#!/bin/bash -ex

D=/home/avi/scylla

clang++ -MD -MT sstable_datafile_test.o -MF sstable_datafile_test.o.d -I/home/avi/scylla/seastar/include -I/home/avi/scylla/build/release/seastar/gen/include -U_FORTIFY_SOURCE -DSEASTAR_SSTRING -march=westmere  -DSEASTAR_API_LEVEL=6 -DSEASTAR_SCHEDULING_GROUPS_COUNT=16 -DSEASTAR_HAVE_DPDK -DFMT_SHARED -I/usr/include/p11-kit-1 -Wl,--gc-sections  -ffile-prefix-map=/home/avi/scylla=. -I$D  -I$D/build/release/gen -I$D/mutation -I$D/schema -I$D/cql3 -I$D/utils -I$D/utils/gz -I$D/dht -march=westmere -ffunction-sections -fdata-sections  -DDEBUG -DSANITIZE -DDEBUG_LSA_SANITIZER -DSCYLLA_ENABLE_ERROR_INJECTION -O1 -fno-slp-vectorize -DSCYLLA_BUILD_MODE=release -g -gz -iquote. -iquote build/release/gen --std=gnu++20  -ffile-prefix-map=/home/avi/scylla=. -Imutation -Ischema -Icql3 -Iutils -Iutils/gz -Idht -march=westmere  -DBOOST_TEST_DYN_LINK   -DNOMINMAX -DNOMINMAX -fvisibility=hidden   -Wfatal-errors -Wno-mismatched-tags -Wno-tautological-compare -Wno-c++11-narrowing -Wno-ignored-attributes -Wno-overloaded-virtual -Wno-unused-command-line-argument -Wno-unsupported-friend -Wno-delete-non-abstract-non-virtual-dtor -Wno-braced-scalar-init -Wno-implicit-int-float-conversion -Wno-delete-abstract-non-virtual-dtor -Wno-psabi -Wno-narrowing -Wno-nonnull -Wno-uninitialized -Wno-deprecated-declarations -DXXH_PRIVATE_API -DSEASTAR_TESTING_MAIN -DFMT_DEPRECATED_OSTREAM  -c -o sstable_datafile_test.o sstable_datafile_test.cc


clang++  -Wl,--gc-sections -Wl,--build-id=sha1,--dynamic-linker=/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////lib64/ld-linux-x86-64.so.2 -fuse-ld=lld -fuse-ld=lld -Wl,--build-id=sha1,--dynamic-linker=/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////lib64/ld-linux-x86-64.so.2  -fvisibility=hidden  -o sstable_datafile_test_g sstable_datafile_test.o  /home/avi/scylla/build/release/seastar/libseastar.a /usr/lib64/libboost_program_options.so /usr/lib64/libboost_thread.so /usr/lib64/libcares.so /usr/lib64/libcryptopp.so /usr/lib64/libfmt.so.9.1.0 -ldl /usr/lib64/libboost_thread.so /usr/lib64/libsctp.so /usr/lib64/libnuma.so -latomic -llz4 -lgnutls -lgmp -lunistring -lnettle -lhogweed -lgmp -lnettle -ltasn1 -lidn2 -lunistring -lp11-kit -lhwloc -lm -lpthread -lyaml-cpp -lyaml-cpp -latomic -llz4 -lz -lsnappy -ljsoncpp  -lstdc++fs  -lcrypt  -lcryptopp  -lpthread -Wl,-Bstatic -lzstd -Wl,-Bdynamic -lboost_date_time -lboost_regex -licuuc -licui18n -lxxhash -ldeflate -llua -lm -ldl -lsystemd -labsl_raw_hash_set -labsl_bad_optional_access -labsl_hashtablez_sampler -labsl_exponential_biased -labsl_synchronization -labsl_graphcycles_internal -labsl_stacktrace -labsl_symbolize -labsl_debugging_internal -labsl_demangle_internal -labsl_malloc_internal -labsl_time -labsl_civil_time -labsl_strings -labsl_strings_internal -lrt -labsl_base -labsl_spinlock_wait -labsl_int128 -labsl_throw_delegate -labsl_raw_logging_internal -labsl_log_severity -labsl_time_zone -labsl_hash -labsl_city -labsl_strings -labsl_strings_internal -labsl_throw_delegate -labsl_bad_optional_access -labsl_bad_variant_access -labsl_low_level_hash -lrt -labsl_base -labsl_raw_logging_internal -labsl_log_severity -labsl_spinlock_wait -labsl_int128 /usr/lib64/libboost_unit_test_framework.so /home/avi/scylla/build/release/seastar/libseastar_testing.a /home/avi/scylla/build/release/seastar/libseastar.a /usr/lib64/libboost_program_options.so /usr/lib64/libboost_thread.so /usr/lib64/libcares.so /usr/lib64/libcryptopp.so /usr/lib64/libfmt.so.9.1.0 -ldl /usr/lib64/libboost_thread.so /usr/lib64/libsctp.so /usr/lib64/libnuma.so -latomic -llz4 -lgnutls -lgmp -lunistring -lnettle -lhogweed -lgmp -lnettle -ltasn1 -lidn2 -lunistring -lp11-kit -lhwloc -lm -lpthread -lyaml-cpp -lfmt

expect() {
    grep -q "$1" gdb.txt || exit 1
}

fails=0
# allow 10% fail rate
for (( i = 0; i < 10 && fails <= 1; ++i )); do
    rm -f gdb.txt
    (ulimit -t 20; gdb -batch -ex 'handle SIGTERM pass noprint' -ex 'set logging on' -ex run -ex bt -ex quit ./sstable_datafile_test_g) || exit 1

    expect ^seastar::current_tasktrace
    expect my_coroutine
    expect migrators::add
done

(( fails <= 1 )) || exit 1

exit 0
