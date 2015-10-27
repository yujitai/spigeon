#ifndef __STORE_COMMAND_STAT_H_
#define __STORE_COMMAND_STAT_H_

#include "command/command.h"
#include "util/store_define.h"

namespace store {

class Pack;
class Request;

class CommandStat : public Command {
  public:
    CommandStat(BinLog *binlog, Engine *engine);
    virtual ~CommandStat();

    virtual bool is_write() { return false; }
    virtual Status extract_params(const Request &req) {
        UNUSED(req);
        return Status::OK();
    }
    virtual Status process(uint64_t binlog_id, Pack* pack);
    static Command* new_instance(BinLog *binlog, Engine *engine) {
        return new CommandStat(binlog, engine);
    }
};

}  // namespace store

#endif  // __STORE_COMMAND_STAT_H_
