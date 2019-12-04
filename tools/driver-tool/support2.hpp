/* ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 */
#pragma once

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <cstdarg>

#include "../../shared/toolkit.vers.h"
#include "cmdline.hpp"

namespace sratools2
{
    struct Args
    {
        const int _argc;
        char ** _argv;
        bool _testing;

        Args ( int argc, char * argv [], const char * test_imp ) : 
            _argc( argc ),
            _argv( nullptr ),
            _testing( ( test_imp != NULL ) && ( test_imp[ 0 ] != 0 ) )
        {
            if ( _testing )
                copy_argv( argv, test_imp );
            else
                _argv = argv;
        }

        ~ Args ()
        {
            if ( _testing )
            {
                for ( int i = 0; i < _argc; ++i )
                    free( _argv[ i ] );
                free( _argv );
            }
        }

        void print( void )
        {
            std::cout << "main2() ( testing = " << _testing << " )" << std::endl;
            for ( int i = 0; i < _argc; ++i )
                std::cout << "argv[" << i << "] = " << _argv[ i ] << std::endl;
        }

        void populate_entry( int idx, const char * src )
        {
            size_t l = strlen( src ) + 1;
            _argv[ idx ] = ( char * )malloc( l );
            strcpy( _argv[ idx ], src );
        }

        void copy_argv( char *argv[], const char * argv0 )
        {
            _argv = ( char ** )malloc( ( _argc + 1 ) * ( sizeof * _argv ) );
            populate_entry( 0, argv0 );
            for ( int i = 1; i < _argc; ++i )
                populate_entry( i, argv[ i ] );
            _argv[ _argc ] = NULL;
        }

    };

    struct ArgvBuilder
    {
        std::vector < std::string > parameters;
        std::vector < std::string > options;

        void add_parameter( const std::string p ) { parameters.push_back( p ); }
        void clear_parameters( void ) { parameters.clear(); }

        void add_option( const std::string &o ) { options.push_back( o ); }
        template < class T > void add_option( const std::string &o, const T &v )
        {
            options.push_back( o );
            std::stringstream ss;
            ss << v;
            options.push_back( ss.str() );
        }
        void add_option( const std::string &o, std::vector < ncbi::String > &v )
        {
            for ( auto const &value : v )
            {
                options.push_back( o );
                options.push_back( value.toSTLString() );
            }
        }
        
        void add_list_option( const std::string &o, char delim, std::vector< ncbi::String > &v )
        {
            options.push_back( o );
            std::stringstream ss;
            int i = 0;
            for ( auto const &value : v )
            {
                if ( i++ > 0 ) ss << delim;
                ss << value;
            }
            options.push_back( ss.str() );
        }

        char * add_string( const std::string &src )
        {
            size_t l = src.length();
            char * dst = ( char * )malloc( l + 1 );
            if ( dst != nullptr )
            {
                strncpy( dst, src . c_str(), l );
                dst[ l ] = 0;
            }
            return dst;
        }

        char ** generate_argv( int &argc )
        {
            argc = 0;
            int cnt = parameters.size() + options.size() + 1;
            char ** res = ( char ** )malloc( cnt * ( sizeof * res ) );
            if ( res != nullptr )
            {
                for ( auto const &value : parameters )
                    res[ argc++ ] = add_string( value );
                for ( auto const &value : options )
                    res[ argc++ ] = add_string( value );
                res[ argc ] = nullptr;
            }
            return res;
        }
        
        void free_argv( int argc, char ** argv )
        {
            if ( argv != nullptr )
            {
                for ( int i = 0; i < argc; ++i )
                {
                    if ( argv[ i ] != nullptr )
                        free( argv[ i ] );
                }
                free( argv );
            }
        }
    };

    enum class Imposter { SRAPATH, PREFETCH, FASTQ_DUMP, FASTERQ_DUMP, SRA_PILEUP, SAM_DUMP, INVALID };

    struct WhatImposter
    {
        public :
            const std::string _basename;
            const std::string _requested_version;
            const std::string _toolkit_version;
            const Imposter _imposter;
            const bool _version_ok;

        private :
            std::string extract_basename( const char * src )
            {
                std::string res( src );
                auto const t1 = res . find_last_of( '/' );
                if ( t1 != std::string::npos )
                    res.assign( res . substr( t1 + 1 ) );
                auto const t2 = res . find_first_of( '.' );
                if ( t2 != std::string::npos )
                    res.assign( res . substr( 0, t2 ) ); 
                return res;
            }

            std::string extract_version( const char * src )
            {
                std::string res( src );
                auto const t1 = res . find_last_of( '/' );
                if ( t1 != std::string::npos )
                    res.assign( res . substr( t1 + 1 ) );
                auto const t2 = res . find_first_of( '.' );
                if ( t2 != std::string::npos )
                    res.assign( res . substr( t2 + 1 ) );
                else
                    res.clear();
                return res;
            }

            std::string get_toolkit_version( void )
            {
                std::stringstream ss;
                ss << ( ( TOOLKIT_VERS ) >> 24 ) << '.';
                ss << ( ( ( TOOLKIT_VERS ) >> 16 ) & 0xFF ) << '.';
                ss << ( ( TOOLKIT_VERS ) & 0xFFFF );
                return ss.str();
            }

            Imposter detect_imposter( const std::string &src )
            {
                if ( src.compare( "srapath" ) == 0 ) return Imposter::SRAPATH;
                else if ( src.compare( "prefetch" ) == 0 ) return Imposter::PREFETCH;
                else if ( src.compare( "fastq-dump" ) == 0 ) return Imposter::FASTQ_DUMP;
                else if ( src.compare( "fasterq-dump" ) == 0 ) return Imposter::FASTERQ_DUMP;
                else if ( src.compare( "sra-pileup" ) == 0 ) return Imposter::SRA_PILEUP;
                else if ( src.compare( "sam-dump" ) == 0 ) return Imposter::SAM_DUMP;
                return Imposter::INVALID;
            }

            std::string imposter_2_string( const Imposter &value )
            {
                switch( value )
                {
                    case Imposter::INVALID : return "INVALID"; break;
                    case Imposter::SRAPATH : return "SRAPATH"; break;
                    case Imposter::PREFETCH : return "PREFETCH"; break;
                    case Imposter::FASTQ_DUMP : return "FASTQ_DUMP"; break;
                    case Imposter::FASTERQ_DUMP : return "FASTERQ_DUMP"; break;
                    case Imposter::SRA_PILEUP : return "SRA_PILEUP"; break;
                    case Imposter::SAM_DUMP : return "SAM_DUMP"; break;
                    default : return "UNKNOWN";
                }
            }

            bool is_version_ok( void )
            {
                if ( _requested_version.empty() ) return true;
                else if ( _requested_version.compare( _toolkit_version ) == 0 ) return true;
                return false;
            }

        public :
            WhatImposter( const char * argv0 )
                : _basename( extract_basename( argv0 ) )
                , _requested_version( extract_version( argv0 ) )
                , _toolkit_version( get_toolkit_version() )
                , _imposter( detect_imposter( _basename ) )
                , _version_ok( is_version_ok() )
            {
            }

            std::string as_string( void )
            {
                std::stringstream ss;
                ss << imposter_2_string( _imposter );
                ss << " _basename:" << _basename;
                ss << " _requested_version:" << _requested_version;
                ss << " _toolkit_version:" << _toolkit_version;
                ss << " _version_ok: " << ( _version_ok ? "YES" : "NO" );
                return ss.str();
            }

            bool invalid( void )
            {
                return ( _imposter == Imposter::INVALID );
            }
            
            bool invalid_version( void )
            {
                return ( !_version_ok );
            }
    };

    struct OptionBase
    {
        virtual ~OptionBase() {}
        
        virtual std::string as_string() { return std::string( "" ); }
        virtual void populate_argv_builder( ArgvBuilder & builder ) { }
        virtual void add( ncbi::Cmdline &cmdline ) { }
        virtual bool check() { return true; }
        
        void print_vec( std::stringstream &ss, std::vector < ncbi::String > &v, std::string name )
        {
            if ( v.size() > 0 )
            {
                ss << name;
                int i = 0;
                for ( auto const &value : v )
                {
                    if ( i++ > 0 ) ss << ',';
                    ss << value;
                }
                ss << std::endl;
            }
        }

        bool is_one_of( const ncbi::String &value, int count, ... )
        {
            bool res = false;
            int i = 0;
            va_list args;
            va_start( args, count );
            while ( !res && i++ < count )
            {
                ncbi::String s_item( va_arg( args, char * ) );
                res = value . equal( s_item );
            }
            va_end( args );
            return res;
        }
        
    };

    struct CmnOptAndAccessions : OptionBase
    {
        ncbi::String toolname;
        std::vector < ncbi::String > accessions;
        ncbi::String ngc_file;
        ncbi::String kar_file;
        ncbi::String perm_file;
        ncbi::String location;
        bool disable_multithreading, version, quiet;
        std::vector < ncbi::String > debug;
        ncbi::String log_level;
        ncbi::String option_file;

        CmnOptAndAccessions( const ncbi::String & the_toolname )
            : toolname( the_toolname )
            , disable_multithreading( false )
            , version( false )
            , quiet( false )
        {

        }
        
        void add( ncbi::Cmdline &cmdline )
        {
            cmdline . addParam ( accessions, 0, 256, "accessions(s)", "list of accessions to process" );
            cmdline . addOption ( ngc_file, nullptr, "", "ngc", "<path>", "<path> to ngc file" );
            cmdline . addOption ( kar_file, nullptr, "", "kar", "<path>", "<path> to kar file" );
            cmdline . addOption ( perm_file, nullptr, "", "perm", "<path>", "<path> to permission file" );
            cmdline . addOption ( location, nullptr, "", "location", "loc", "location in cloud" );
            
            cmdline . addOption ( disable_multithreading, "", "disable-multithreading", "disable multithreading" );
            cmdline . addOption ( version, "V", "version", "Display the version of the program" );

            /*
            // problem: 'q' could be used by the tool already...
            cmdline . addOption ( quiet, "q", "quiet",
                "Turn off all status messages for the program. Negated by verbose." );
            */

            cmdline . addListOption( debug, ',', 255, "+", "debug", "<Module[-Flag]>",
                "Turn on debug output for module. All flags if not specified." );

            cmdline . addOption ( log_level, nullptr, "L", "log-level", "<level>",
                "Logging level as number or enum string. One of (fatal|sys|int|err|warn|info|debug) or "
                "(0-6) Current/default is warn" );
            cmdline . addOption ( option_file, nullptr, "", "option-file", "file",
                "Read more options and parameters from the file." );
        }

        std::string as_string()
        {
            std::stringstream ss;
            for ( auto const& value : accessions )
                ss << "acc  = " << value << std::endl;
            if ( !ngc_file.isEmpty() )  ss << "ngc-file : " << ngc_file << std::endl;
            if ( !kar_file.isEmpty() )  ss << "kar-file : " << kar_file << std::endl;
            if ( !perm_file.isEmpty() ) ss << "perm-file: " << perm_file << std::endl;
            if ( !location.isEmpty() )  ss << "location : " << location << std::endl;
            if ( disable_multithreading ) ss << "disable multithreading" << std::endl;
            if ( version ) ss << "version" << std::endl;
            print_vec( ss, debug, "debug modules:" );
            if ( !log_level.isEmpty() ) ss << "log-level: " << log_level << std::endl;
            if ( !option_file.isEmpty() ) ss << "option-file: " << option_file << std::endl;
            return ss.str();
        }

        void populate_argv_builder( ArgvBuilder & builder )
        {
            builder . add_option( "-+", debug );
            if ( disable_multithreading ) builder . add_option( "--disable-multithreading" );
            if ( !log_level.isEmpty() ) builder . add_option( "-L", log_level );
            if ( !option_file.isEmpty() ) builder . add_option( "--option-file", option_file );
        }
        
        bool check()
        {
            bool res = true;
            if ( !log_level.isEmpty() )
            {
                res = is_one_of( log_level, 14,
                                  "fatal", "sys", "int", "err", "warn", "info", "debug",
                                  "0", "1", "2", "3", "4", "5", "6" );
            }
            // we could check if ngc/kar/perm-files do actually exist...
            return res;
        }
    };

    int impersonate_fasterq_dump( const Args &args );
    int impersonate_fastq_dump( const Args &args );
    int impersonate_srapath( const Args &args );
    int impersonate_prefetch( const Args &args );
    int impersonate_sra_pileup( const Args &args );
    int impersonate_sam_dump( const Args &args );

    struct Impersonator
    {
        const Args &args;
        const ncbi::String &toolname;
        OptionBase &tool_options;

        Impersonator( const Args &_args, const ncbi::String &_toolname, OptionBase &_tool_options )
            : args( _args ), toolname( _toolname ), tool_options( _tool_options )
        {
        }

        int run( void )
        {
            // Cmdline is a class defined in cmdline.hpp
            ncbi::Cmdline cmdline( args . _argc, args . _argv );
            
            // CmnOptAndAccessions is defined in support2.hpp
            CmnOptAndAccessions cmn_options( toolname );

            // add all the tool-specific options to the parser ( first )
            tool_options . add( cmdline );

            // add all common options and the parameters to the parser
            cmn_options . add( cmdline );

            try
            {
                // let the parser parse the original args,
                // and let the parser handle help,
                // and let the parser write all values into cmn and params
                
                // preparsing...
                cmdline . parse ( true );

                // full parsing
                cmdline . parse ();

                // pre-check the options, after the input has been parsed!
                if ( !tool_options . check() )
                    return 3;

                if ( !cmn_options . check() )
                    return 3;

                // just to see what we got
                std::cout << tool_options . as_string() << cmn_options . as_string() << std::endl;

                // create an argv-builder 
                ArgvBuilder builder;
                // add all options from both to the builder
                tool_options . populate_argv_builder( builder );
                cmn_options . populate_argv_builder( builder );

                // what should happen before executing the tool
                int argc;
                char ** argv = builder . generate_argv( argc );
                if ( argv != nullptr )
                {
                    for ( int i = 0; i < argc; ++i )
                        std::cout << "argv[" << i << "] = '" << argv[ i ] << "'" << std::endl;

                    // at this point we have everything in place to execute
                    // the tool on all accessions:
                    // cmn_options . accessions has to be expanded ( resolve containers )
                    // ( ngs/kar/perm/location is available in cmn_options )
                    // cmn_options . toolname is the name of the tool to execute
                    // argv is the sanitized new argument-vector to be passed on to the tool
                    
                    builder . free_argv( argc, argv );
                }

            }
            catch ( ncbi::Exception const &e )
            {
                std::cerr << "An error occured: " << e.what() << std::endl;
            }

            return 0;
        }
    };
    
} // namespace...