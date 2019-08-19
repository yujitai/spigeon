topdir = ../..
srcdir = $(topdir)/src
testdir = $(topdir)/test
depsdir = $(topdir)/deps

uname_S := $(shell sh -c "uname -s 2>/dev/null || echo not")
OPTIMIZATION ?= -O2

WARN = -Wall
OPT = $(OPTIMIZATION)
DEBUG = -g -ggdb -DDEBUG -DUSE_SYS_TIME

CXXFLAGS := -c $(OPT) $(WARN) $(DEBUG)
CXXFLAGS += -I$(srcdir) -I$(depsdir)/libev-4.11
CXXFLAGS += -I$(topdir)/test/gtest/include
CXXFLAGS += -I$(topdir)/baidu/public/mcpack/output/include

LDFLAGS := $(DEBUG)
LDFLAGS += -lpthread

LIBS =  $(depsdir)/libev-4.11/.libs/libev.a $(topdir)/test/gtest/lib/gtest.a

ifeq ($(uname_S), Linux)
  CXX=g++
  LD=g++
else
  CXX=c++
  LD=c++
endif

PROG_NAME = test_log
OBJS = test_log.o \
       log.o url_snprintf.o \
       dispatcher.o worker.o event.o network.o thread.o sds.o zmalloc.o

all: $(PROG_NAME)
	@echo "Done :)"

.PHONY: all

log.o: $(srcdir)/util/log.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

dispatcher.o: $(srcdir)/event/dispatcher.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

worker.o: $(srcdir)/event/worker.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

event.o: $(srcdir)/event/event.cpp
	$(CXX) $(CXXFLAGS) $?
network.o: $(srcdir)/util/network.cpp
	$(CXX) $(CXXFLAGS) $?	
thread.o: $(srcdir)/event/thread.cpp
	$(CXX) $(CXXFLAGS) $?
sds.o: $(srcdir)/util/sds.cpp
	$(CXX) $(CXXFLAGS) $?
zmalloc.o: $(srcdir)/util/zmalloc.cpp
	$(CXX) $(CXXFLAGS) $?
url_snprintf.o: $(srcdir)/util/url_snprintf.cpp
	$(CXX) $(CXXFLAGS) $?

$(PROG_NAME): $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	-(rm $(PROG_NAME) $(OBJS))

.PHONY: clean

#log.o:
#	cd util && g++ -c log.cpp && cd ..
#	cd util && g++ -o test_log -lpthread test_log.cpp log.o ./gtest/lib/gtest.a -I./libevent/include -L./libevent/lib -Wl,-Bstatic -levent -Wl,-Bdynamic -lrt -I./gtest/include 

