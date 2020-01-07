# gsdm framework-通用服务开发模型

# Feature
* cmd-process 处理模型
* 使用 unix 域进行多线程同步
* 支持多线程间数据及指令的转发

# Compile demo
g++ demo.cpp -std=c++11 -I/home/yujitai/dev/zrtc/voip/zframework/output/include/zframework /home/yujitai/dev/zrtc/voip/zframework/output/lib/libzframework.a -lpthread

