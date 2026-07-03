#!/bin/bash
SRCS=$(find src/ -name '*.c' | sort)
for harness in ingest_fuzzer query_fuzzer admin_fuzzer migrate_fuzzer; do
    $CC $CFLAGS -I include fuzz/${harness}.c $SRCS $LIB_FUZZING_ENGINE -o $OUT/${harness}
done
