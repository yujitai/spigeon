#ifndef __STORE_ENV_H_
#define __STORE_ENV_H_

#include <string>
#include <stdint.h>
#include "util/log.h"
#include "util/store_define.h"
#include "inc/module.h"

namespace store {

class Slice;
class DB;
class Config;
class BinLog;
class Engine;
class DBServer;
class Recovery;
class Replication;
class CommandFactory;

enum {
    ENV_OK = 0,
    ENV_ERROR = -1
};

struct EnvOptions {
    char *logpath;
    char *logfilename;
    log_level_t loglevel;

    char *binlogdir;
    char *binlogfilename;
};

class Env : public Module{
  public:
    Env();
    virtual ~Env();
    int init_conf();
    int load_conf(const char *filename);
    int validate_conf();
    int init_log();
    
    int init(const char *filename, Engine *e);
    void run();
    void stop();
    int cleanup();
    static bool setkillsignalhandler(void (*handler)(int));
    static void suicide();
    static int64_t getpid();
  private:
    EnvOptions options;
    
    Engine *engine;
    BinLog *binlog;
    DB *db;
    Recovery *recovery;
    Replication *replication;
    DBServer *server;
};


}  // namespace store

#endif
