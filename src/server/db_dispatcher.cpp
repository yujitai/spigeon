#include "server/db_dispatcher.h"
#include "server/db_worker.h"
#include "server/server.h"

namespace store {
DBDispatcher::DBDispatcher(DBServerOptions &o, DB *db, Replication *rep)
    : GenericDispatcher((GenericServerOptions&)o),
    db_options(o), db_handler(db), rep_handler(rep), rep_worker(NULL)
{
}

DBDispatcher::~DBDispatcher() {
    delete rep_worker;
}

int DBDispatcher::init() {
    int ret = GenericDispatcher::init();
    if (ret) {
        return ret;
    }
    if (spawn_rep_worker() == DISPATCHER_ERROR) {
        return DISPATCHER_ERROR;
    }
    return DISPATCHER_OK;
}

int DBDispatcher::spawn_worker() {
    GenericWorker *new_worker = new DBWorker(db_options, this, db_handler, rep_handler);
    new_worker->init();
    if (create_thread(new_worker) == THREAD_ERROR) {
        log_fatal("failed to create worker thread");
        return DISPATCHER_ERROR;
    }
    workers.push_back(new_worker);
    return DISPATCHER_OK;
}

int DBDispatcher::spawn_rep_worker() {
    GenericWorker *new_worker;
    if (db_options.master) {
        new_worker = new RepMasterWorker(db_options, rep_handler);
    } else {
        new_worker = new RepSlaveWorker(db_options, rep_handler);
    }
    if (new_worker->init() != WORKER_OK) {
        log_fatal("failed to init rep worker");
        return DISPATCHER_ERROR;
    }
    if (create_thread(new_worker) == THREAD_ERROR) {
        log_fatal("failed to create rep worker");
        return DISPATCHER_ERROR;
    }
    rep_worker = new_worker;
    return DISPATCHER_OK;
}

int DBDispatcher::join_workers() {
    for (size_t i = 0; i < workers.size(); i++) {
        if (workers[i] != NULL) {
            workers[i]->notify(GenericWorker::QUIT);
            if (join_thread(workers[i]) == THREAD_ERROR) {
                log_fatal("failed to join worker thread");
            }
        }
    }
    if (rep_worker != NULL) {
        rep_worker->notify(GenericWorker::QUIT);
        if (join_thread(rep_worker) == THREAD_ERROR) {
            log_fatal("failed to join rep worker");
        }
    }
    return DISPATCHER_OK;
}

int DBDispatcher::new_slave(RepMsg *msg) {
    if (db_options.master) {
        if (rep_worker) {
            lock.lock();
            rep_worker->mq_push((void*)msg);
            rep_worker->notify(GenericWorker::NEWCONNECTION);
            lock.unlock();
            return DISPATCHER_OK;
        } else {
            log_warning("no replication worker available");
            return DISPATCHER_ERROR;
        }
    } else {
        return DISPATCHER_ERROR;
    }
}

}

