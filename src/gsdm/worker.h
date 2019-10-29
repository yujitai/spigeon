/*******************************************************************
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file worker.h
 * @author yujitai
 * @date 2019/10/29 19:27:09
 ******************************************************************/

#ifndef  __WORKER_H_
#define  __WORKER_H_

namespace zf {

class NewGenericWorker : public GenericWorker {
public:
    void notify_unix(const uint8_t* v, size_t len);
    void process_unix_notify(CmdInfo)
private:
    int _unix_socket_post;
    int _unix_socket_wait;
    IOWatcher* _unix_watcher;
};

} // namespace zf

#endif  //__WORKER_H_


