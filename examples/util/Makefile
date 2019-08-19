#COMAKE2 edit-mode: -*- Makefile -*-
####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -pipe \
  -W \
  -Wall
CFLAGS=-g \
  -pipe \
  -W \
  -Wall
CPPFLAGS=-D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -DVERSION=\"1.9.8.7\" \
  -DDEBUG
INCPATH=-I. \
  -I../../src \
  -I../../deps/libev-4.11
DEP_INCPATH=-I../../../../../../../lib2-64/bsl \
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
  -I../../../../../../../third-64/pcre/output/include

#============ CCP vars ============
CCHECK=@ccheck.py
CCHECK_FLAGS=
PCLINT=@pclint
PCLINT_FLAGS=
CCP=@ccp.py
CCP_FLAGS=


#COMAKE UUID
COMAKE_MD5=8443dfcc402b197087b9c9716b2a4f28  COMAKE


.PHONY:all
all:comake2_makefile_check ../.. test_stat test_key_lock test_time test_file_util test_log 
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:comake2_makefile_check
comake2_makefile_check:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mcomake2_makefile_check[0m']"
	#in case of error, update 'Makefile' by 'comake2'
	@echo "$(COMAKE_MD5)">comake2.md5
	@md5sum -c --status comake2.md5
	@rm -f comake2.md5

.PHONY:ccpclean
ccpclean:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mccpclean[0m']"
	@echo "make ccpclean done"

.PHONY:clean
clean:ccpclean
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	$(MAKE) -C ../.. clean
	rm -rf test_stat
	rm -rf ./output/bin/test_stat
	rm -rf test_key_lock
	rm -rf ./output/bin/test_key_lock
	rm -rf test_time
	rm -rf ./output/bin/test_time
	rm -rf test_file_util
	rm -rf ./output/bin/test_file_util
	rm -rf test_log
	rm -rf ./output/bin/test_log
	rm -rf test_stat_test_stat.o
	rm -rf test_key_lock_test_key_lock.o
	rm -rf test_time_test_time.o
	rm -rf test_file_util_test_file_util.o
	rm -rf test_log_test_log.o

.PHONY:dist
dist:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdist[0m']"
	tar czvf output.tar.gz output
	@echo "make dist done"

.PHONY:distclean
distclean:clean
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdistclean[0m']"
	rm -f output.tar.gz
	@echo "make distclean done"

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

test_stat:test_stat_test_stat.o \
  ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_stat[0m']"
	$(CXX) test_stat_test_stat.o -Xlinker "-(" ../../deps/libev-4.11/.libs/libev.a \
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
  -lrt -Xlinker "-)" -o test_stat
	mkdir -p ./output/bin
	cp -f --link test_stat ./output/bin

test_key_lock:test_key_lock_test_key_lock.o \
  ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_key_lock[0m']"
	$(CXX) test_key_lock_test_key_lock.o -Xlinker "-(" ../../deps/libev-4.11/.libs/libev.a \
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
  -lrt -Xlinker "-)" -o test_key_lock
	mkdir -p ./output/bin
	cp -f --link test_key_lock ./output/bin

test_time:test_time_test_time.o \
  ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_time[0m']"
	$(CXX) test_time_test_time.o -Xlinker "-(" ../../deps/libev-4.11/.libs/libev.a \
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
  -lrt -Xlinker "-)" -o test_time
	mkdir -p ./output/bin
	cp -f --link test_time ./output/bin

test_file_util:test_file_util_test_file_util.o \
  ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_file_util[0m']"
	$(CXX) test_file_util_test_file_util.o -Xlinker "-(" ../../deps/libev-4.11/.libs/libev.a \
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
  -lrt -Xlinker "-)" -o test_file_util
	mkdir -p ./output/bin
	cp -f --link test_file_util ./output/bin

test_log:test_log_test_log.o \
  ../../deps/libev-4.11/.libs/libev.a \
  ../../output/lib/libstoreframework.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_log[0m']"
	$(CXX) test_log_test_log.o -Xlinker "-(" ../../deps/libev-4.11/.libs/libev.a \
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
  -lrt -Xlinker "-)" -o test_log
	mkdir -p ./output/bin
	cp -f --link test_log ./output/bin

test_stat_test_stat.o:test_stat.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_stat_test_stat.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o test_stat_test_stat.o test_stat.cpp

test_key_lock_test_key_lock.o:test_key_lock.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_key_lock_test_key_lock.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o test_key_lock_test_key_lock.o test_key_lock.cpp

test_time_test_time.o:test_time.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_time_test_time.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o test_time_test_time.o test_time.cpp

test_file_util_test_file_util.o:test_file_util.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_file_util_test_file_util.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o test_file_util_test_file_util.o test_file_util.cpp

test_log_test_log.o:test_log.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest_log_test_log.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o test_log_test_log.o test_log.cpp

endif #ifeq ($(shell uname -m),x86_64)


