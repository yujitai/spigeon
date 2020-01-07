/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file worker.cpp
 * @author yujitai
 * @date 2020/01/03 15:47:58
 ******************************************************************/

#include "gsdm/worker.h"

#include "iostream"
using namespace std;

namespace zf {

static void recv_unix_notify(EventLoop* el, IOWatcher* w, int fd, int revents, void* data) {
    NewGenericWorker* wk = (NewGenericWorker*)data;
    ssize_t recv = recvfrom(fd, wk->_unix_wait_buf, sizeof(wk->_unix_wait_buf), 0, NULL, NULL);
    cout << "recv_unix_notify:recvfrom:" << recv << endl;
    wk->process_unix_notify((CmdInfo*)(wk->_unix_wait_buf));
}

NewGenericWorker::NewGenericWorker(int worker_id) 
    : _worker_id(worker_id),
      _unix_sock_post(-1),
      _unix_sock_wait(-1)
{
    _el = new EventLoop((void*)this, false);
    _address_len = sizeof(struct sockaddr_un);
    memset(&_address_wait, 0, _address_len);
    memset(_unix_wait_buf, 0, sizeof(_unix_wait_buf));
}

NewGenericWorker::~NewGenericWorker() {
    delete _el;

    if (_unix_sock_post != -1) {
        close(_unix_sock_post);
        _unix_sock_post = -1;
    }

    if (_unix_sock_wait != -1) {
        close(_unix_sock_wait);
        _unix_sock_wait = -1;
    }
}

int NewGenericWorker::initialize() {
    int unix_sock_fd[2];
    if (-1 == socketpair(AF_UNIX, SOCK_DGRAM, 0, unix_sock_fd)) {
        // log_fatal("unix sock fd create failed. error[%s]", strerror(errno));
        cout << "socketpair failed" << endl;
        return -1;
    }
    _unix_sock_post = unix_sock_fd[0];
    _unix_sock_wait = unix_sock_fd[1];

    std::stringstream unix_path;
    unix_path << "gsdm_" << get_worker_id();
    _address_wait.sun_family = AF_UNIX;
    // _address_wait.sun_path[0] = '\0';
    strncpy(_address_wait.sun_path + 1, unix_path.str().c_str(), sizeof(_address_wait.sun_path) - 2);
    // remove(_address_wait.sun_path);
    // unlink(_address_wait.sun_path);
    if (-1 == bind(_unix_sock_wait, (sockaddr*)&_address_wait, _address_len)) {
        // log_fatal("[Bind failed] errno[%d] err[%s]", errno, strerror(errno));
        cout << strerror(errno) << endl;
        return -1;
    }

    _unix_watcher = _el->create_io_event(recv_unix_notify, (void*)this);
    if (_unix_watcher == nullptr) 
        return -1;
    _el->start_io_event(_unix_watcher, _unix_sock_wait, EventLoop::READ);

    return 0;
}

void NewGenericWorker::run() {
    cout << "el run" << endl;
    _el->run();
}

void NewGenericWorker::setup_queue(uint32_t src_worker_id, int cmd_type) {
    if (!MAP_HAS2(_worker_queue, src_worker_id, cmd_type)) {
        cout << "setup_queue:src_worker_id=" << src_worker_id << " cmd_type=" << cmd_type << endl;
        _worker_queue[src_worker_id][cmd_type] = new LockFreeQueue<BaseCmd*>();
    }

    /*
    FOR_UNORDERED_MAP(_worker_queue, uint32_t, QueueHash, i) {
        cout << MAP_KEY(i) << endl; 
    }
    */
}

void NewGenericWorker::send_cmd(NewGenericWorker* src, int cmd_type, BaseCmd* cmd) {
    QueueHash& qh = _worker_queue[src->get_worker_id()];
    if (!MAP_HAS1(qh, cmd_type)) 
        return;

    qh[cmd_type]->produce(cmd);

    CmdInfo info = { src->get_worker_id(), cmd_type };
    notify_unix((uint8_t*)&info, sizeof(CmdInfo));
}

int NewGenericWorker::notify_unix(const uint8_t* buffer, size_t len) {
    ssize_t sent = sendto(_unix_sock_post, buffer, len, 0, (struct sockaddr*)&_address_wait, _address_len);
    cout << "notify unix:sendto:" << sent << endl;
}

int NewGenericWorker::process_unix_notify(CmdInfo* info) {
    cout << "worker_id=" << get_worker_id() << " process_unix_notify from worker_id=" << info->worker_id << endl;
    int src_worker_id = info->worker_id;
    int cmd_type = info->cmd_type;
    if (!MAP_HAS2(_worker_queue, src_worker_id, cmd_type)) 
        return -1;

    BaseCmd* cmd = nullptr;
    _worker_queue[src_worker_id][cmd_type]->consume(&cmd);
    if (!cmd)
        return -1;

    cout << "basecmd process:cmdtype=" << cmd_type << endl;
    cmd->process(this);
}

} // namespace zf


