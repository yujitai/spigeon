# zframework 
一个 `C/C++` 轻量级服务器异步网络编程框架, 追求更简洁的实现

# 基本架构
```
                      +---------------------+
                      |    GenericServer    |
                      +---------------------+
                      +---------------------+
                      |  GenericDispatcher  |
                      +---------------------+
                      /          |           \
+--------------------+ +-------------------+ +-------------------+
|   GenericWorker    | |   GenericWorker   | |   GenericWorker   |
+--------------------+ +-------------------+ +-------------------+
                       +-------------------+
                       |   NetworkManager  |
                       +-------------------+
                       +-------------------+
                       |     Connection    |
                       +-------------------+
                       +-------------------+
                       |       Socket      |
                       +-------------------+
                       +-------------------+
                       |       libev       |
                       +-------------------+

```

## 特性
* 支持 `TCP` 并发服务编程
* 支持 `UDP` 并发服务编程
* 支持 `HTTP` 客户端编程  
* 支持百度 `mcpack` 私有协议, `HTTP` 协议
* 使用 `pipe` 进行线程间同步
* libev 事件驱动模型
* 强大的异步日志系统
* 完善的配置文件系统
* 无锁队列

## 开发历程
* v1.0, 2015-10-25, 从百度 ksarch 迁移 store-framework
* v1.0, 2019-01-17, 为满足 `zrtc` 的需求, 开始优化 store-framework
* v1.0, 2019-08-19, 正式更名为 `zframework`, 开始开发 1.0 版本
* v1.0, 2019-09-11, 从代码的布局、逻辑以及底层库的服务角度完成第一次精简和优化
* v1.0, 2019-09-18, 增加基础 UDP 服务模型
* v1.0, 2019-09-24, 底层网络层 Socket 抽象完成

## 构建
./build.sh

## 作者
yujitai<yujitai@zuoyebang.com>


