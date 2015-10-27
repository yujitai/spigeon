#ifndef __STORE_REPLICATION_H_
#define __STORE_REPLICATION_H_

#include "binlog/binlog.h"
#include "db/db_define.h"
#include "util/slice.h"
#include "util/network.h"
#include "inc/module.h"

namespace store {

// [-4000, -4999] replication
enum replication_error_t {
    REPLICATION_OK,
    
    // [0 1000] some error that sync process need cope with
    REPLICATION_BINLOG_NOT_AVAIL   = 1,
    REPLICATION_READ_BINLOG_FAIL   = 2,
    REPLICATION_SLAVE_DATA_TOO_OLD = 3,

    // [-4000 -4099] client error
    REPLICATION_METHOD_ERROR     = -4000,

    // [-4100 -4199] replication inner error
    REPLICATION_MALLOC_ERROR      = -4101,
    REPLICATION_SYNC_ID_NOT_EXIST = -4102,
    REPLICATION_BINLOG_CHECH_FAIL = -4103,
    REPLICATION_DB_PROCESS_ERROR  = -4104,
    REPLICATION_GET_COMMAND_ERROR = -4105,
    REPLICATION_LOG_PARAM_ERROR   = -4106,
    REPLICATION_MCPACK_ERROR      = -4107,
    REPLICATION_BINLOG_ERROR      = -4108,

    REPLICATION_ERROR             = -4999,
    REPLICATION_INTERNAL_ERROR    = -1
};

#define STORE_REPL_PING "REPL_PING"
#define STORE_REPL_DATA "REPL_DATA"
#define STORE_REPL_HANDSHAKE "REPL_HANDSHAKE"

#define STORE_COMMAND_SYNC      "SYNC"

class DB;
class Env;
class Config;
class BinLog;
class Engine;
class Response;
class CommandFactory;
class BinLogReaderHolder;
class Request;
struct BinLogMessage;
struct SyncMessage;
class RepStatus {
public:
    RepStatus() : reader(NULL), binlog(NULL) { }
    ~RepStatus() {
        if (binlog) {
            binlog->put_back_reader(reader);
        }
    }
    BinLogReader* reader;
    BinLog *binlog;
};

struct ReplOptions {
    long long slowlog_threshold;
};

class Replication : public Module{
public:
    Replication(DB *db, BinLog *binlog);
    virtual ~Replication();

    int init_conf();
    int load_conf(const char *filename);
    int validate_conf();

    int build_sync(Slice *sync);

    int process_sync(const Request& req, RepStatus *status);

    int get_binlog(RepStatus *status, Slice *log);
    int process_binlog(const Request& binlog);
    
    int build_ping(Slice *ping);
    int build_handshake_msg(Slice *msg);
  private:
    int get_last_binlog(uint64_t *log_id, uint32_t *checksum);
    int build_binlog_message(const BinLogMessage& msg, size_t len, Slice *log);
    int parse_binlog_message(const Request& req, BinLogMessage *msg);
    
    int build_sync_message(const SyncMessage& msg, Slice *sync);
    int parse_sync_message(const Request& req, SyncMessage *msg);
    
    DB *_db;
    BinLog* _binlog;
    ReplOptions _options;
};

}  // namespace store

#endif  // __STORE_REPLICATION_H_
