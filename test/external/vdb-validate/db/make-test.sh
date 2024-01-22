#!/bin/sh

SRR=SRR26020762
SCRATCH=`mktemp -d` || exit 1
(
    cd "${SCRATCH}"
    NCBI_VDB_QUALITY="R" prefetch -o "${SRR}.sra" "${SRR}" || { echo "prefetch failed"; exit $?; }
    cp "${SRR}.sra" corrupt.sra
    printf '\xa1' | dd of=corrupt.sra bs=1 seek=1000000 conv=notrunc
    kar -x corrupt.sra -d corrupt
    vdb-unlock corrupt
)
