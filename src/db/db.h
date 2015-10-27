#ifndef __STORE_DB_H_
#define __STORE_DB_H_

#include "db/db_define.h"
#include "util/zmalloc.h"
#include "util/key_lock.h"
#include "inc/module.h"
#include "util/status.h"

namespace store {

class Env;
class Pack;
class Slice;
class Request;
class Response;
class BinLog;
class Engine;
class Config;
class CommandFactory;

struct nshead_t;
struct binlog_id_seq_t;

// [-2000, -2999] db
enum db_error_t {
    DB_OK                 = 0,

    DB_PACK_ERROR         = -2000,
    
    DB_ERROR              = -2999,
};

struct DBOptions {
    long long response_buf_size;  
    bool read_only;
    long long slowlog_threshold;
    long long cmd_slowlog_threshold;
};

class DB : public Module{
public:
    DB(BinLog *binlog, Engine *engine);
    virtual ~DB();

    int init_conf();
    int load_conf(const char *filename);
    int validate_conf();
    // interface for normal requests
    Status process_request(const Request& request, Slice* resp);
    // interface for replication slave
    Status process_binlog(const Slice& data, uint64_t binlog_id);
    // interface for recovery
    Status process_idempotent_request(const Request &request, uint64_t binlog_id);
    // interface for semantic monitoring request
    Status process_monitor_request(Slice *resp);

    Status build_error_response(const Status& status, Slice *resp);
private:
    Status process_sub_request(const Request& request, Pack* pack);

    Status dispatch_request(const Request& request, Response* resp);

    Status write_binlog(const Request& request, binlog_id_seq_t* seq);

    Status mark_request_done(const binlog_id_seq_t& seq);
    
    int set_error_msg(Pack *pack, const Status& status);
    
    BinLog *_binlog;
    Engine *_engine;
    CommandFactory* _cmd_factory;
    KeyLock _key_lock;
    
    DBOptions _options;
};

}  // namespace store

#endif  // __STORE_DB_H_
