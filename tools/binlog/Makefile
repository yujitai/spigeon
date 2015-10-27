#COMAKE2 edit-mode: -*- Makefile -*-
####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
.PHONY:all
all:comake2_makefile_check ../.. store-binlog-tool store-binlog-fix 
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:comake2_makefile_check
comake2_makefile_check:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mcomake2_makefile_check[0m']"
	@if [ "COMAKE" -nt "Makefile" ] ; then echo "[ERROR]PLEASE UPDATE 'Makefile' BY 'comake2'" ; exit 250 ;  fi

.PHONY:clean
clean:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	$(MAKE) -C ../.. clean
	rm -rf store-binlog-tool
	rm -rf ./output/bin/store-binlog-tool
	rm -rf store-binlog-fix
	rm -rf ./output/bin/store-binlog-fix
	rm -rf store-binlog-tool_store_binlog_tool.o
	rm -rf store-binlog-tool_mcpack_format.o
	rm -rf store-binlog-fix_store_binlog_fix.o

.PHONY:dist
dist:clean
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdist[0m']"
	cd ..;tar czvf binlog.tar.gz binlog;cd binlog

.PHONY:love
love:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mlove[0m']"
	@echo "make love done"

.PHONY:../..
../..:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40m../..[0m']"
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Entering directory:'[1;32;40m../..[0m']"
	$(MAKE) -C ../..
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Leaving directory:'[1;32;40m../..[0m']"

store-binlog-tool:store-binlog-tool_store_binlog_tool.o \
  store-binlog-tool_mcpack_format.o \
  ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mstore-binlog-tool[0m']"
	g++ store-binlog-tool_store_binlog_tool.o \
  store-binlog-tool_mcpack_format.o -Xlinker "-(" ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a ../../../../../../../lib2-64/bsl/lib/libbsl.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_ResourcePool.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_archive.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_check_cast.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_exception.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_pool.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_utils.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_var.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_var_implement.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_var_utils.a \
  ../../../../../../../lib2-64/ullib/lib/libullib.a \
  ../../../../../../../public/configure/libconfig.a \
  ../../../../../../../public/mcpack/libmcpack.a \
  ../../../../../../../public/spreg/libspreg.a \
  ../../../../../../../third-64/gtest/lib/libgtest.a \
  ../../../../../../../third-64/gtest/lib/libgtest_main.a \
  ../../../../../../../third-64/pcre/lib/libpcre.a \
  ../../../../../../../third-64/pcre/lib/libpcrecpp.a \
  ../../../../../../../third-64/pcre/lib/libpcreposix.a -lpthread \
  -lcrypto \
  -lrt -Xlinker "-)" -o store-binlog-tool
	mkdir -p ./output/bin
	cp -f --link store-binlog-tool ./output/bin

store-binlog-fix:store-binlog-fix_store_binlog_fix.o \
  ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mstore-binlog-fix[0m']"
	g++ store-binlog-fix_store_binlog_fix.o -Xlinker "-(" ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a ../../../../../../../lib2-64/bsl/lib/libbsl.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_ResourcePool.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_archive.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_check_cast.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_exception.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_pool.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_utils.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_var.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_var_implement.a \
  ../../../../../../../lib2-64/bsl/lib/libbsl_var_utils.a \
  ../../../../../../../lib2-64/ullib/lib/libullib.a \
  ../../../../../../../public/configure/libconfig.a \
  ../../../../../../../public/mcpack/libmcpack.a \
  ../../../../../../../public/spreg/libspreg.a \
  ../../../../../../../third-64/gtest/lib/libgtest.a \
  ../../../../../../../third-64/gtest/lib/libgtest_main.a \
  ../../../../../../../third-64/pcre/lib/libpcre.a \
  ../../../../../../../third-64/pcre/lib/libpcrecpp.a \
  ../../../../../../../third-64/pcre/lib/libpcreposix.a -lpthread \
  -lcrypto \
  -lrt -Xlinker "-)" -o store-binlog-fix
	mkdir -p ./output/bin
	cp -f --link store-binlog-fix ./output/bin

store-binlog-tool_store_binlog_tool.o:store_binlog_tool.cpp \
  store_binlog_tool.cpp \
  mcpack_format.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mstore-binlog-tool_store_binlog_tool.o[0m']"
	g++ -c -I. \
  -I../../src \
  -I../../deps/libev-4.11 -I../../../../../../../lib2-64/bsl \
  -I../../../../../../../lib2-64/bsl/include \
  -I../../../../../../../lib2-64/bsl/output \
  -I../../../../../../../lib2-64/bsl/output/include \
  -I../../../../../../../lib2-64/ullib \
  -I../../../../../../../lib2-64/ullib/include \
  -I../../../../../../../lib2-64/ullib/output \
  -I../../../../../../../lib2-64/ullib/output/include \
  -I../../../../../../../public/configure \
  -I../../../../../../../public/configure/include \
  -I../../../../../../../public/configure/output \
  -I../../../../../../../public/configure/output/include \
  -I../../../../../../../public/mcpack \
  -I../../../../../../../public/mcpack/include \
  -I../../../../../../../public/mcpack/output \
  -I../../../../../../../public/mcpack/output/include \
  -I../../../../../../../public/spreg \
  -I../../../../../../../public/spreg/include \
  -I../../../../../../../public/spreg/output \
  -I../../../../../../../public/spreg/output/include \
  -I../../../../../../../third-64/gtest \
  -I../../../../../../../third-64/gtest/include \
  -I../../../../../../../third-64/gtest/output \
  -I../../../../../../../third-64/gtest/output/include \
  -I../../../../../../../third-64/pcre \
  -I../../../../../../../third-64/pcre/include \
  -I../../../../../../../third-64/pcre/output \
  -I../../../../../../../third-64/pcre/output/include -D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -DVERSION=\"1.9.8.7\" -g \
  -pipe \
  -W \
  -Wall  -o store-binlog-tool_store_binlog_tool.o store_binlog_tool.cpp

store-binlog-tool_mcpack_format.o:mcpack_format.cpp \
  mcpack_format.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mstore-binlog-tool_mcpack_format.o[0m']"
	g++ -c -I. \
  -I../../src \
  -I../../deps/libev-4.11 -I../../../../../../../lib2-64/bsl \
  -I../../../../../../../lib2-64/bsl/include \
  -I../../../../../../../lib2-64/bsl/output \
  -I../../../../../../../lib2-64/bsl/output/include \
  -I../../../../../../../lib2-64/ullib \
  -I../../../../../../../lib2-64/ullib/include \
  -I../../../../../../../lib2-64/ullib/output \
  -I../../../../../../../lib2-64/ullib/output/include \
  -I../../../../../../../public/configure \
  -I../../../../../../../public/configure/include \
  -I../../../../../../../public/configure/output \
  -I../../../../../../../public/configure/output/include \
  -I../../../../../../../public/mcpack \
  -I../../../../../../../public/mcpack/include \
  -I../../../../../../../public/mcpack/output \
  -I../../../../../../../public/mcpack/output/include \
  -I../../../../../../../public/spreg \
  -I../../../../../../../public/spreg/include \
  -I../../../../../../../public/spreg/output \
  -I../../../../../../../public/spreg/output/include \
  -I../../../../../../../third-64/gtest \
  -I../../../../../../../third-64/gtest/include \
  -I../../../../../../../third-64/gtest/output \
  -I../../../../../../../third-64/gtest/output/include \
  -I../../../../../../../third-64/pcre \
  -I../../../../../../../third-64/pcre/include \
  -I../../../../../../../third-64/pcre/output \
  -I../../../../../../../third-64/pcre/output/include -D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -DVERSION=\"1.9.8.7\" -g \
  -pipe \
  -W \
  -Wall  -o store-binlog-tool_mcpack_format.o mcpack_format.cpp

store-binlog-fix_store_binlog_fix.o:store_binlog_fix.cpp \
  store_binlog_fix.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mstore-binlog-fix_store_binlog_fix.o[0m']"
	g++ -c -I. \
  -I../../src \
  -I../../deps/libev-4.11 -I../../../../../../../lib2-64/bsl \
  -I../../../../../../../lib2-64/bsl/include \
  -I../../../../../../../lib2-64/bsl/output \
  -I../../../../../../../lib2-64/bsl/output/include \
  -I../../../../../../../lib2-64/ullib \
  -I../../../../../../../lib2-64/ullib/include \
  -I../../../../../../../lib2-64/ullib/output \
  -I../../../../../../../lib2-64/ullib/output/include \
  -I../../../../../../../public/configure \
  -I../../../../../../../public/configure/include \
  -I../../../../../../../public/configure/output \
  -I../../../../../../../public/configure/output/include \
  -I../../../../../../../public/mcpack \
  -I../../../../../../../public/mcpack/include \
  -I../../../../../../../public/mcpack/output \
  -I../../../../../../../public/mcpack/output/include \
  -I../../../../../../../public/spreg \
  -I../../../../../../../public/spreg/include \
  -I../../../../../../../public/spreg/output \
  -I../../../../../../../public/spreg/output/include \
  -I../../../../../../../third-64/gtest \
  -I../../../../../../../third-64/gtest/include \
  -I../../../../../../../third-64/gtest/output \
  -I../../../../../../../third-64/gtest/output/include \
  -I../../../../../../../third-64/pcre \
  -I../../../../../../../third-64/pcre/include \
  -I../../../../../../../third-64/pcre/output \
  -I../../../../../../../third-64/pcre/output/include -D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -DVERSION=\"1.9.8.7\" -g \
  -pipe \
  -W \
  -Wall  -o store-binlog-fix_store_binlog_fix.o store_binlog_fix.cpp

endif #ifeq ($(shell uname -m),x86_64)


