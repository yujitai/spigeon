#use lower case 'makefile', 'Makefile' will ignored in .git2svnignore
LIBEV=libev-4.11/.libs/libev.a
LIBJEMALLOC=jemalloc_stable4/lib/libjemalloc.a
LIBCONFIG=new-config/libconfig.a
LIBPROTOBUF=protobuf/src/.libs/libprotobuf.a
LIBEVHTTPCLIENT=evhttpclient/libevhttpclient.a

basic: $(LIBEV) $(LIBJEMALLOC) $(LIBCONFIG) $(LIBEVHTTPCLIENT)

#build some third-party lib cost too much time, especillay protobuf. make it separately
all: $(LIBEV) $(LIBJEMALLOC) $(LIBCONFIG) $(LIBEVHTTPCLIENT) $(LIBPROTOBUF)

clean:
	echo "clean"
	# sometimes make clean not working if configure is not ok
	cd libev-4.11 && ./configure && make clean
	cd jemalloc_stable4 && ./configure && make clean
	cd new-config && make clean
	cd evhttpclient && make clean

third-clean:
	#cd protobuf && ./configure && make clean

$(LIBEV):
	cd libev-4.11 && ./configure && make

$(LIBJEMALLOC):
	cd jemalloc_stable4 && ./autogen.sh && ./configure && make

$(LIBPROTOBUF):
	cd protobuf && ./configure && make
	# use static verion of protoc, othervise you should include the lib path in LD_LIBRARY_PATH
	#cd protobuf && LDFLAGS=-static ./configure && make

$(LIBCONFIG):
	cd new-config && make

$(LIBEVHTTPCLIENT):
	cd evhttpclient && make
