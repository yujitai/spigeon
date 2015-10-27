#ifndef __STORE_RECOVERY_H_
#define __STORE_RECOVERY_H_

#include <stddef.h>
#include "db/db_define.h"
#include "inc/module.h"

namespace store {

class DB;
class BinLog;

enum recovery_errot_t {
    RECOVERY_OK           = 0,
    // [-4500, -4599] recovery
    RECOVERY_BINLOG_ERROR = -4500,
    RECOVERY_MALLOC_ERROR = -4501,
    RECOVERY_MCPACK_ERROR = -4502,
    RECOVERY_ENGINE_ERROR = -4503,

    RECOVERY_ERROR        = -4599
};

struct RecoveryOptions {
};

class Recovery : public Module{
public:
    Recovery(DB* db, BinLog *binlog);
    virtual ~Recovery();

    int init_conf();
    int load_conf(const char *filename);
    int validate_conf();
    
    int recover();
private:

    DB* _db;
    BinLog* _binlog;
};

}  // namespace store

#endif  // __STORE_RECOVERY_H_
