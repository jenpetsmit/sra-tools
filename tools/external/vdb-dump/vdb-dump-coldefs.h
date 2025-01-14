/*===========================================================================
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

#ifndef _h_vdb_dump_coldefs_
#define _h_vdb_dump_coldefs_

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/table.h>
#include <vdb/view.h>
#include <vdb/cursor.h>
#include <vdb/database.h>

#include <klib/vector.h>
#include <klib/text.h>
#include <klib/num-gen.h>

#include "vdb-dump-str.h"
#include "vdb-dump-context.h" /* because of dump_format_t */

#ifdef __cplusplus
extern "C" {
#endif


/* returns a const pointer to a static const string */
typedef const char* (*value_trans_fn_t)( const uint32_t id );

/* returns a return-code, writes to buffer */
/* typedef char* (*dim_trans_fct_t)( const uint8_t *src, dump_format_t fmt ); */

/* returns a return-code, writes to buffer */
typedef rc_t (*dim_trans_fn_t)( char * dst, size_t dst_size, size_t * written,
                                const uint8_t *src, dump_format_t fmt );

/********************************************************************
col-def is the definition of a single column: name/index/type
********************************************************************/
typedef struct col_def
{
    char *name;
    uint32_t idx;
    uint64_t elementsum;
    bool valid;
    bool excluded;
    VTypedecl type_decl;
    VTypedesc type_desc;
    dump_str content;
    value_trans_fn_t value_trans_fn;
    dim_trans_fn_t dim_trans_fn;
    size_t dim_trans_size;
} col_def;
typedef col_def* p_col_def;

/********************************************************************
the col-defs are a vector of single column-definitions
********************************************************************/
typedef struct col_defs
{
    Vector cols;
    uint16_t max_colname_chars;
    size_t str_limit;
} col_defs;
typedef col_defs* p_col_defs;

#define MAX_COL_NAME_LEN 64

const char *vdcd_get_platform_txt( const uint32_t id );

char *vdcd_make_domain_txt( const uint32_t domain );

bool vdcd_init( col_defs** defs, const size_t str_limit );
void vdcd_destroy( col_defs* defs );

uint32_t vdcd_parse_string( col_defs* defs, const char* src, const VTable *tbl, uint32_t * invalid_columns );
uint32_t vdcd_extract_from_table( col_defs* defs, const VTable *tbl, uint32_t * invalid_columns );

uint32_t vdcd_parse_string_view( col_defs* defs, const char* src, const VView *my_view );
uint32_t vdcd_extract_from_view( col_defs* defs, const VView *my_view, uint32_t *invalid_columns );

bool vdcd_table_has_column( const VTable *tbl, const char * to_find );
bool vdcd_extract_from_phys_table( col_defs* defs, const VTable *tbl );
uint32_t vdcd_add_to_cursor( col_defs* defs, const VCursor *curs );
void vdcd_reset_content( col_defs* defs );
void vdcd_ins_trans_fkt( col_defs* defs, const VSchema *schema );
void vdcd_exclude_these_columns( col_defs* defs, const char* column_names );
bool vdcd_get_first_none_static_column_idx( col_defs* defs, const VCursor * curs, uint32_t * idx );

uint32_t vdcd_extract_static_columns( col_defs* defs, const VTable *tbl,
                                      const size_t str_limit, uint32_t * invalid_columns );

rc_t vdcd_collect_spread( const struct num_gen * row_set, col_defs * cols, const VCursor * curs );

#ifdef __cplusplus
}
#endif

#endif
