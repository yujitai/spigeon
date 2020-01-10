/*******************************************************************
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file worker.h
 * @author yujitai
 * @date 2019/10/29 19:27:09
 ******************************************************************/

#ifndef  __GSDM_WORKER_H_
#define  __GSDM_WORKER_H_

#include <stdint.h>
#include <stddef.h>
#include <unordered_map>

#include "gsdm/basecmd.h"
#include "gsdm/stlwrapper.h"
#include "server/event.h"
#include "server/worker.h"
#include "server/socket_include.h"
#include "util/log.h"

namespace zf {

class NewGenericWorker : public Runnable {
public:
    NewGenericWorker(int worker_id);
    ~NewGenericWorker();

    int initialize();
    void run() override;

    int notify_unix(const uint8_t* buffer, size_t len);
    int process_unix_notify(CmdInfo* info);

    void setup_queue(uint32_t src_worker_id, int cmd_type);
    //BaseCmd* get_cmd(NewGenericWorker* from, int cmd_type);
    void send_cmd(NewGenericWorker* src, int cmd_type, BaseCmd* cmd);

    int get_worker_id() const { return _worker_id; }

public:
    char _unix_wait_buf[16];

private:
    int _worker_id;

    int _unix_sock_post;
    int _unix_sock_wait;
    sockaddr_un _address_wait;
    socklen_t _address_len;
    IOWatcher* _unix_watcher = nullptr;
    EventLoop* _el = nullptr;

    // K:command type 
    // V:queue of the cmd type
    typedef std::unordered_map<int, LockFreeQueue<BaseCmd*>*> QueueHash;
    // K:worker id
    // V:queues map
    typedef std::unordered_map<uint32_t, QueueHash> WorkerQueue; 
    WorkerQueue _worker_queue;
};

} // namespace zf

#endif  //__GSDM_WORKER_H_


