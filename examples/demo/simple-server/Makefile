#function@makefile
#author@yujitai
#date@20190905

header_path = -I/home/yujitai/dev/zrtc/voip/zframework/output/include/zframework \
			  -I/home/yujitai/dev/zrtc/newcommon/mcpack/output/include

lib_path = -L/home/yujitai/dev/zrtc/voip/zframework/output/lib \
		   -L/home/yujitai/dev/zrtc/newcommon/mcpack/output/lib \
		   -lzframework -lpthread -lrt -lssl -std=c++11 


objects = simple_server.o \
		  simple_worker.o
simple_server : $(objects)
	g++ -o simple_server $(objects) $(header_path) $(lib_path) 
simple_server.o : simple_server.cpp
	g++ -c simple_server.cpp $(header_path) 
simple_worker.o : simple_worker.h
	g++ -c simple_worker.cpp $(header_path) 


.PHONY : clean
clean :
	rm simple_server $(objects)


