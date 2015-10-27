#ifndef __STORE_COMMAND_LIST_H_
#define __STORE_COMMAND_LIST_H_

#include "command/command.h"

namespace store{

    class BinLog;
    class Engine;
    class CommandLpush : public Command{
        public:
            CommandLpush(BinLog *binlog, Engine *engine);
            virtual ~CommandLpush();

            virtual bool is_write(){ return true;}
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLpush(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process( uint64_t binlog_id, Pack* pack);
            Slice _member;
            Slice _extra;
            int32_t _seconds;
    };

    class CommandRpush : public Command {
        public:
            CommandRpush(BinLog *binlog, Engine *engine);
            virtual ~CommandRpush();

            virtual bool is_write(){ return true;}
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandRpush(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process( uint64_t binlog_id, Pack* pack);
            Slice _member;
            Slice _extra;
            int32_t _seconds;
    };

    class CommandLsetbymember : public Command {
        public:
            CommandLsetbymember(BinLog *binlog, Engine *engine);
            virtual ~CommandLsetbymember();

            virtual bool is_write() { return true; }
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLsetbymember(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process( uint64_t binlog_id, Pack* pack);
            Slice _member;
            Slice _extra;
            int32_t _seconds;
    };

    class CommandLset : public Command {
        public:
            CommandLset(BinLog *binlog, Engine *engine);
            virtual ~CommandLset();

            virtual bool is_write() { return true; }
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLset(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process(uint64_t binlog_id, Pack* pack);
            Slice _member;
            Slice _extra;
            int32_t _seconds;
            int32_t _index;     
    };

    class CommandLpop : public Command {
        public:
            CommandLpop(BinLog *binlog, Engine *engine);
            virtual ~CommandLpop();

            virtual bool is_write() { return true; } 
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLpop(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process(uint64_t binlog_id, Pack* pack);
            Slice _member;
            Slice _extra;
            int32_t _seconds;
    };

    class CommandRpop : public Command {
        public:
            CommandRpop(BinLog *binlog, Engine *engine);
            virtual ~CommandRpop();

            virtual bool is_write() { return true; } 
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandRpop(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process(uint64_t binlog_id, Pack* pack);
            Slice _member;
            Slice _extra;
            int32_t _seconds; 
    };

    class CommandLrem : public Command{
        public:
            CommandLrem(BinLog *binlog, Engine *engine);
            virtual ~CommandLrem();

            virtual bool is_write() { return true; }
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLrem(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process(uint64_t binlog_id, Pack* pack);
            Slice _member;
            Slice _extra;
            int32_t _seconds;
            int32_t _count;
    };

    class CommandLrim : public Command{
        public:
            CommandLrim(BinLog *binlog, Engine *engine);
            virtual ~CommandLrim(); 

            virtual bool is_write() { return true; } 
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLrim(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process( uint64_t binlog_id, Pack* pack);

            int32_t _start;
            int32_t _stop;
            int32_t _seconds;
    };

    class CommandLlen : public Command{
        public:
            CommandLlen(BinLog *binlog, Engine *engine);
            virtual ~CommandLlen();

            virtual bool is_write() { return false; }
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLlen(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process( uint64_t binlog_id, Pack* pack);
    };

    class CommandLgetbymember : public Command{
        public:
            CommandLgetbymember(BinLog *binlog, Engine *engine);
            virtual ~CommandLgetbymember();

            virtual bool is_write() { return false; } 
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLgetbymember(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process(uint64_t binlog_id, Pack* pack);
            Slice _member;
    };

    class CommandLindex : public Command{
        public:
            CommandLindex(BinLog *binlog, Engine *engine);
            virtual ~CommandLindex();

            virtual bool is_write() { return false; }
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLindex(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process(uint64_t binlog_id, Pack* pack);
            int32_t _index;
    };

    class CommandLrange : public Command{
        public:
            CommandLrange (BinLog *binlog, Engine *engine);
            virtual ~CommandLrange();

            virtual bool is_write() { return false;}
            static Command* new_instance(BinLog *binlog, Engine *engine){
                return new CommandLrange(binlog, engine);
            }
        private:
            virtual Status extract_params(const Request &req);
            virtual Status process(uint64_t binlog_id, Pack* pack);
            int32_t _pos;
            int32_t _count;
    };

}



#endif
