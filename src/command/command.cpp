#include "command/command.h"

#include <pthread.h>
#include <string>
#include "command/command_kv.h"
#include "command/command_list.h"
#include "command/command_stat.h"
#include "db/db_define.h"
#include "db/request.h"
#include "db/response.h"
#include "engine/engine.h"
#include "util/pack.h"
#include "util/slice.h"
#include "util/log.h"
#include "util/stat.h"
#include "util/store_define.h"
#include "util/zmalloc.h"
#include "util/key_lock.h"

namespace store {

Command::Command(BinLog *binlog, Engine *engine) :
   _binlog(binlog), _engine(engine), _key("") { }

Command::~Command() { }


// modify here if you want to add new command
static cmd_t commands[] =  {
    {"GET",     CommandGet::new_instance},
    {"SET",     CommandSet::new_instance},
    {"SETEX",   CommandSetEx::new_instance},
    {"DEL",     CommandDel::new_instance},
    {"INCRBY",  CommandIncrBy::new_instance},
   

    {"LPUSH",   CommandLpush::new_instance},
    {"RPUSH",   CommandRpush::new_instance},

    {"LSETBYMEMBER",   CommandLsetbymember::new_instance}, 
    {"LSET",   CommandLset::new_instance},

    {"LPOP",   CommandLpop::new_instance}, 
    {"RPOP",   CommandRpop::new_instance}, 
    {"LREM",   CommandLrem::new_instance}, 
    {"LRIM",   CommandLrim::new_instance}, 
   
    {"LLEN",   CommandLlen::new_instance}, 
    {"LGETBYMEMBER",   CommandLgetbymember::new_instance}, 
    {"LINDEX",   CommandLindex::new_instance}, 
    {"LRANGE",CommandLrange::new_instance},

    //{"EXPIRE",  CommandExpire::new_instance},
    //{"PERSIST", CommandPersist::new_instance},
    //{"TTL",     CommandTtl::new_instance},

    {"STAT",    CommandStat::new_instance}
};

void CommandFactory::load_command_table() {
    int command_num = sizeof(commands) / sizeof(commands[0]);
    for (int i = 0; i < command_num; ++i) {
        command_table[commands[i].method] = commands[i].factory_method;
    }
    log_debug("load command table success! table size[%d]",
              (int)command_table.size());
}

CommandFactory::CommandFactory(BinLog *binlog, Engine *engine) : 
        _binlog(binlog), _engine(engine) {
    load_command_table();
}

Command* CommandFactory::create(const Slice& cmd_name) {
    cmd_map_t::iterator it = command_table.find(cmd_name.to_string());
    if (it == command_table.end()) {
        return NULL;
    }
    return (it->second)(_binlog, _engine);
}

}  // namespace store
