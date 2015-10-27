#include "inc/env.h"
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "binlog/binlog.h"
#include "db/db.h"
#include "db/db_define.h"
#include "engine/engine.h"
#include "server/db_server.h"
#include "server/db_dispatcher.h"
#include "db/recovery.h"
#include "replication/replication.h"
#include "util/slice.h"
#include "util/stat.h"
#include "util/config_file.h"

namespace store {


Env::Env()
        :engine(NULL),
        binlog(NULL), db(NULL), recovery(NULL), replication(NULL),
        server(NULL){}  

Env::~Env() {
}

static int set_loglevel(int argc, char **argv, command_t *cmd, void *conf) {
    UNUSED(cmd);
    if (argc < 1) {
        return CONFIG_ERROR;
    }
    EnvOptions *options = (EnvOptions*)conf;
    if (!strcasecmp(argv[0], "debug")) {
        options->loglevel = STORE_LOG_DEBUG;
    } else if (!strcasecmp(argv[0], "trace")) {
        options->loglevel = STORE_LOG_TRACE;
    } else if (!strcasecmp(argv[0], "notice")) {
        options->loglevel = STORE_LOG_NOTICE;
    } else if (!strcasecmp(argv[0], "warning")) {
        options->loglevel = STORE_LOG_WARNING;
    } else if (!strcasecmp(argv[0], "fatal")) {
        options->loglevel = STORE_LOG_FATAL;
    } else {
        fprintf(stderr, "loglevel should be one of "
                "debug, trace, notice, warning or fatal\n");
        return CONFIG_ERROR;
    }
    return CONFIG_OK;
}

static command_t env_cmd_table[] = {
    { "logpath",
      conf_set_str_slot,
      offsetof(EnvOptions, logpath) },

    { "logfilename",
      conf_set_str_slot,
      offsetof(EnvOptions, logfilename) },

    { "loglevel",
      set_loglevel,
      0 },

    { "binlogdir",
      conf_set_str_slot,
      offsetof(EnvOptions, binlogdir) },

    { "binlogfilename",
      conf_set_str_slot,
      offsetof(EnvOptions, binlogfilename) },
    
    null_command
};

int Env::init_conf() {
    return ENV_OK;
}
int Env::load_conf(const char *filename) {
    if (load_conf_file(filename, env_cmd_table, &options) != CONFIG_OK) {
        log_fatal("failed to load config file for env module");
        return ENV_ERROR;
    } else {
        fprintf(stderr,
                "Env Options:\n"
                "log_file: %s/%s\n"
                "log_level: %d\n"
                "binlog_file: %s/%s\n"
                "\n",
                options.logpath, options.logfilename,
                options.loglevel,
                options.binlogdir, options.binlogfilename);

        return ENV_OK;
    }
}

int Env::validate_conf() {
    return ENV_OK;
}
int Env::init_log() {
    // init log
    if (log_init(options.logpath, options.logfilename, options.loglevel) != 0) {
        fprintf(stderr, "log init failed!");
        return ENV_ERROR;
    }

    return ENV_OK;
}
int Env::init(const char *filename, Engine *e) {
    // init binlog FIX
    BinLogOptions binlog_options;
    binlog = new BinLog(binlog_options);
    if (binlog->open(options.binlogdir,
                     options.binlogfilename) != 0) {
        log_fatal("binlog open failed: %s/%s", options.binlogdir,
                  options.binlogfilename);
        return ENV_ERROR;
    }
    // init stat
    struct timeval tv;
    gettimeofday(&tv, NULL);
    stat_set("startup_time_s", tv.tv_sec);
    // init db module
    engine = e;
    db = new DB(binlog, engine);
    db->init_conf();
    if (db->load_conf(filename) != DB_OK) {
        return ENV_ERROR;
    }
    if (db->validate_conf() != DB_OK) {
        return ENV_ERROR;
    }
    // init recovery module
    recovery = new Recovery(db, binlog);
    recovery->init_conf();
    if (recovery->load_conf(filename) != RECOVERY_OK) {
        return ENV_ERROR;
    }
    if (recovery->validate_conf() != RECOVERY_OK) {
        return ENV_ERROR;
    }
    // init replication 
    replication = new Replication(db, binlog);
    replication->init_conf();
    if (replication->load_conf(filename) != REPLICATION_OK) {
        return ENV_ERROR;
    }
    if (replication->validate_conf() != REPLICATION_OK) {
        return ENV_ERROR;
    }
    
    // init server
    server = new DBServer;
    server->init_conf();
    if (server->load_conf(filename) != SERVER_MODULE_OK) {
        return ENV_ERROR;
    }
    if (server->init(db, replication) != SERVER_MODULE_OK) {
        return ENV_ERROR;
    }
    if (server->validate_conf() != SERVER_MODULE_OK) {
        return ENV_ERROR;
    }
    return ENV_OK;
}

void Env::run() {
    if (recovery != NULL) {
        int ret = recovery->recover();
        if (ret != 0) {
            log_fatal("recovery failed!");
            return;
        }
    } else {
        log_fatal("recovery init failed!");
        return;
    }
    if (server != NULL)
        server->run();
    else {
        log_fatal("dispatcher has not been initialized");
    }
}

void Env::stop() {
    if (server) {
        server->stop();
    }
}

int Env::cleanup() {
    delete server;
    server = NULL;
    delete replication;
    replication = NULL;
    delete recovery;
    recovery = NULL;
    delete db;
    db = NULL;
    delete binlog;
    binlog = NULL;

    log_close();
    return 0;
}

bool Env::setkillsignalhandler(void (*handler)(int)) {
    signal(SIGPIPE, SIG_IGN);
    int signals[] = {
        SIGHUP, SIGINT, SIGUSR1, SIGUSR2, SIGTERM
    };
    bool err = false;
    for (size_t i = 0; i < sizeof(signals)/sizeof(*signals); i++) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_flags = 0;
        sa.sa_handler = handler;
        sigfillset(&sa.sa_mask);
        if (sigaction(signals[i], &sa, NULL) != 0) err = true;
    }
    return !err;
}

void Env::suicide() {
    raise(SIGTERM);
}

int64_t Env::getpid() {
    return ::getpid();
}
}  // namespace store
