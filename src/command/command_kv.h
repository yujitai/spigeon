#ifndef __STORE_COMMAND_KV_H_
#define __STORE_COMMAND_KV_H_

#include "command/command.h"

namespace store {

class BinLog;
class Engine;

class CommandGet : public Command {
  public:
    CommandGet(BinLog *binlog, Engine *engine);
    virtual ~CommandGet();

    virtual bool is_write() { return false; }

    static Command* new_instance(BinLog *binlog, Engine *engine) {
        return new CommandGet(binlog, engine);
    }
  private:
    virtual Status extract_params(const Request &req);
    virtual Status process( uint64_t binlog_id, Pack* pack);
};

class CommandSet : public Command {
  public:
    CommandSet(BinLog *binlog, Engine *engine);
    virtual ~CommandSet();

    virtual bool is_write() { return true; }
    static Command* new_instance(BinLog *binlog, Engine *engine) {
        return new CommandSet(binlog, engine);
    }
  private:
    virtual Status extract_params(const Request &req);
    virtual Status process( uint64_t binlog_id, Pack* pack);
    Slice _value;
    int32_t _seconds;
};

class CommandSetEx : public Command {
  public:
    CommandSetEx(BinLog *binlog, Engine *engine);
    virtual ~CommandSetEx();

    virtual bool is_write() { return true; }
    static Command* new_instance(BinLog *binlog, Engine *engine) {
        return new CommandSetEx(binlog, engine);
    }
  private:
    virtual Status extract_params(const Request &req);
    virtual Status process( uint64_t binlog_id, Pack* pack);
    Slice _value;
    int32_t _seconds;
};

class CommandDel : public Command {
  public:
    CommandDel(BinLog *binlog, Engine *engine);
    virtual ~CommandDel();

    virtual bool is_write() { return true; }
    static Command* new_instance(BinLog *binlog, Engine *engine) {
        return new CommandDel(binlog, engine);
    }
  private:
    virtual Status extract_params(const Request &req);
    virtual Status process( uint64_t binlog_id, Pack* pack);
};

class CommandIncrBy: public Command {
  public:
    CommandIncrBy(BinLog *binlog, Engine *engine);
    virtual ~CommandIncrBy();
    virtual bool is_write() { return true; }
    static Command* new_instance(BinLog *binlog, Engine *engine) {
        return new CommandIncrBy(binlog, engine);
    }
  private:
    virtual Status extract_params(const Request &req);
    virtual Status process( uint64_t binlog_id, Pack *pack);
    int64_t increment;
};
}  // namespace store

#endif  // __STORE_COMMAND_KV_H_
