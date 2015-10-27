#include "command/command_stat.h"
#include <string>
#include <list>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "server/event.h"
#include "binlog/binlog.h"
#include "db/db_define.h"
#include "util/log.h"
#include "util/stat.h"
#include "util/store_define.h"
#include "util/pack.h"
#include "util/string.h"

namespace store {

CommandStat::CommandStat(BinLog *binlog, Engine *engine) :
   Command(binlog, engine) { }

CommandStat::~CommandStat() { }

Status CommandStat::process(uint64_t binlog_id, Pack* pack) {
    UNUSED(binlog_id);

    stat_container_t sum_result;
    stat_container_t max_result;
    stat_get_all(&sum_result, &max_result);

    std::string str;

    unsigned long now = EventLoop::current_time() / 1000000;
    struct rusage self_ru, c_ru;
    getrusage(RUSAGE_SELF, &self_ru);
    getrusage(RUSAGE_CHILDREN, &c_ru);

    str.append(string_format("now:%d\n", getpid()));
    str.append(string_format("process_id:%d\n", getpid()));
    str.append(string_format("startup_time_s:%ld\n", max_result["startup_time_s"]));
    str.append(string_format("uptime_in_seconds:%ld\n", 
        now - max_result["startup_time_s"]));
    str.append(string_format("uptime_in_days:%ld\n",
        (now - max_result["startup_time_s"]) / 3600 / 24));
    str.append(string_format("used_cpu_sys:%.2f\n", 
        (float)self_ru.ru_stime.tv_sec+(float)self_ru.ru_stime.tv_usec/1000000));
    str.append(string_format("used_cpu_user:%.2f\n", 
        (float)self_ru.ru_utime.tv_sec+(float)self_ru.ru_utime.tv_usec/1000000));
    str.append(string_format("used_cpu_sys_children:%.2f\n", 
        (float)c_ru.ru_stime.tv_sec+(float)c_ru.ru_stime.tv_usec/1000000));
    str.append(string_format("used_cpu_user_children:%.2f\n", 
        (float)c_ru.ru_utime.tv_sec+(float)c_ru.ru_utime.tv_usec/1000000));

    str.append(string_format("max_binlog_id:0x%lX\n", max_result["max_binlog_id"]));
    str.append(string_format("max_query_buffer_size:%ld\n", max_result["max_query_buffer_size"]));
    str.append(string_format("max_reply_list_size:%ld\n", max_result["max_reply_list_size"]));
    str.append(string_format("max_reply_size:%ld\n", max_result["max_reply_size"]));
    str.append(string_format("last_rep_time_s:%ld\n", max_result["last_rep_time"]));
    str.append(string_format("current_binlog_delay:%ld\n", max_result["current_binlog_delay"]));
    str.append(string_format("max_binlog_delay:%ld\n", max_result["max_binlog_delay"]));

    str.append(string_format("current_connections:%ld\n", sum_result["current_connections"]));
    str.append(string_format("total_connections:%ld\n", sum_result["total_connections"]));
    str.append(string_format("cmds_all:%ld\n", sum_result["cmds_all"]));
    str.append(string_format("total_cmd_request:%ld\n", sum_result["total_cmd_request"]));
    str.append(string_format("total_sync_request:%ld\n", sum_result["total_sync_request"]));

    str.append(string_format("cmds_STAT:%ld\n", sum_result["cmds_STAT"]));

    str.append(string_format("cmds_GET:%ld\n", sum_result["cmds_GET"]));
    str.append(string_format("cmds_SET:%ld\n", sum_result["cmds_SET"]));
    str.append(string_format("cmds_DEL:%ld\n", sum_result["cmds_DEL"]));
    str.append(string_format("cmds_INCRBY:%ld\n", sum_result["cmds_INCRBY"]));

    str.append(string_format("cmds_LPUSH:%ld\n", sum_result["cmds_LPUSH"]));
    str.append(string_format("cmds_RPUSH:%ld\n", sum_result["cmds_RPUSH"]));
    str.append(string_format("cmds_LVALUESET:%ld\n", sum_result["cmds_LSETBYMEMBER"]));
    str.append(string_format("cmds_LSET:%ld\n", sum_result["cmds_LSET"]));
    str.append(string_format("cmds_LPOP:%ld\n", sum_result["cmds_LPOP"]));
    str.append(string_format("cmds_RPOP:%ld\n", sum_result["cmds_RPOP"]));
    str.append(string_format("cmds_LREM:%ld\n", sum_result["cmds_LREM"]));
    str.append(string_format("cmds_LRIM:%ld\n", sum_result["cmds_LRIM"]));
    str.append(string_format("cmds_LLEN:%ld\n", sum_result["cmds_LLEN"]));
    str.append(string_format("cmds_LGETBYVALUE:%ld\n", sum_result["cmds_LGETBYMEMBER"]));
    str.append(string_format("cmds_LINDEX:%ld\n", sum_result["cmds_LINDEX"]));
    str.append(string_format("cmds_LRANGE:%ld\n", sum_result["cmds_LRANGE"]));

    str.append(string_format("active_list_size:%d\n", _binlog->active_binlog_list_size()));
    str.append(string_format("active_list_max_size:%d\n", _binlog->active_binlog_list_max_size()));

    str.append(_engine->stat());

    int ret = pack->put_str(STORE_RESP_STAT, str);
    if (ret != 0) {
        log_warning("pack put str failed! ret[%d]", ret);
        return Status::InternalError("packing error");;
    }

    log_debug("process stat success");
    return Status::OK();
}

}  // namespace store
