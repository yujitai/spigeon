#ifndef __STORE_COMMAND_H_
#define __STORE_COMMAND_H_

#include <string>
#include <map>

#include "util/status.h"
#include "util/slice.h"
#include "engine/engine.h"

namespace store {

class BinLog;
class Engine;
class Request;
class Response;
class Pack;

class Command {
public:
    Command(BinLog *binlog, Engine *engine);
    virtual ~Command();

    virtual bool is_write() = 0;
    virtual Status extract_params(const Request &req) = 0;
    // the Pack *pack can be NULL when response is not needed
    virtual Status process(uint64_t binlog_id, Pack* pack) = 0;
    const Slice& get_key() const { return _key; }
    int64_t _slowlog_threshold;
protected:

    BinLog *_binlog;
    Engine *_engine;
    Slice _key;
private:
    // No coping allowed
    Command(const Command&);
    void operator=(const Command&);
};

typedef Command* (*cmd_factory_method_t)(BinLog*, Engine*);

typedef struct cmd_t {
    const char* method;
    cmd_factory_method_t factory_method;
} cmd_t;

typedef std::map<std::string, cmd_factory_method_t> cmd_map_t;


class CommandFactory {
public:
    CommandFactory(BinLog *binlog, Engine *engine);
    ~CommandFactory() { }
    Command* create(const Slice& command_name);

private:
    void load_command_table();
    BinLog *_binlog;
    Engine *_engine;
    cmd_map_t command_table;
};

}  // namespace store

#endif  // __STORE_COMMAND_H_
