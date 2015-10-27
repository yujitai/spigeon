/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file binlog_api_basic_test.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/20 15:51:22
 * @brief 
 **/

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

#include "binlog/binlog.h"
#include "util/log.h"
#include "server/event.h"

using namespace store;

uint64_t g_single_file_limit = 1 << 20;
const uint64_t g_max_data_len = 10 * 1024 * 1024;
std::string g_binlog_dir = "./data/binlog";

int g_count = 1;

#define notice(...) printf(__VA_ARGS__)
#define fatal(...) \
do { \
    printf(__VA_ARGS__); \
    usleep(10*1000); /*waiting for log thread*/ \
    abort(); \
    /*raise(SIGKILL);*/ \
} while (0) \

void test_append(BinLogWriter *writer)
{
    notice("append test count:%d\n", g_count);

    char *buf = new (std::nothrow) char[g_max_data_len];
    assert(buf);

    for (int i = 0; i < g_count; ++i) {
        int len = rand() % g_max_data_len + 1;
        memset(buf, (len%256), len);

        binlog_id_seq_t seq;
        int ret = writer->append(buf, len, &seq);
        if (ret != 0) {
            fatal("writer.append failed. len:%d, ret:%d\n", len, ret);
        }
        notice("writer.append finished. len:%d, ret:%d, cur:0x%lX, prev:0x%lX, next:0x%lX\n",
            len, ret, seq.cur, seq.prev, seq.next);

        writer->active_binlog_mark_done(seq);
    }

    delete [] buf;
}

void test_read(BinLogReader *reader)
{
    notice("read test count:%d\n", g_count);

    uint64_t binlog_id = 0;
    for (int i = 0; i < g_count; ++i) {
        if (!reader->sendable(binlog_id)) {
            notice("not sendable binlog_id:0x%lX", binlog_id);
            break;
        }
        if (reader->eof(binlog_id)) {
            notice("reach the end of binlog. 0x%lX\n", binlog_id);
            break;
        }

        int ret = reader->read(binlog_id);
        if (ret != 0) {
            fatal("reader.read failed. binlog_id:0x%lX, ret:%d\n", binlog_id, ret);
        }

        notice("reader.read finished. ret:%d, binlog_id:0x%lX, binlog_id_in_head:0x%lX, "
            "ts:%lu, total_len:%u, data_len:%u, prev:0x%lX, next:0x%lX\n",
            ret, binlog_id, reader->head().binlog_id,
            reader->head().timestamp_us, reader->head().total_len, reader->head().data_len,
            reader->seq().prev, reader->seq().next);
        binlog_id = reader->next_binlog_id();
    }
}

EventLoop g_el(NULL, true);
void run_event()
{
    g_el.run();
}

void load_opt(int argc, char *argv[])
{
	if (argc <= 1) {
		//print_help();
	}

    char c = '\0';
    while (-1 != (c = getopt(argc, argv, "c:s:h?"))) {
        switch(c) {
        case 'c':
			g_count = atoi(optarg);
            if (g_count < 0) {
                fatal("invalid parameter: count");
            }
            break;
        case 's':
			g_single_file_limit = strtoull(optarg, NULL, 10);
            break;
        case 'h':
        case '?':
        default:
            //print_help();
            break;
        }
    }
    notice("count: %d\n", g_count);
    notice("single_file_limit: %lu\n", g_single_file_limit);
}

int main(int argc, char *argv[])
{
    int ret = 0;

    load_opt(argc, argv);

    ret = log_init("./log", "binlog_api_basic_test", STORE_LOG_TRACE);
    //ret = log_init("./log", "binlog_api_basic_test", STORE_LOG_DEBUG);
    if (ret != 0) {
        fatal("log_init failed\n");
    }
    run_event(); // 为了从libev中获取时间戳
    //notice("%llu\n", EventLoop::current_time());

    log_debug("-------------------- START log --------------------");
    log_warning("-------------------- START log --------------------");

    BinLogOptions binlog_options;
    binlog_options.read_only = false;
    binlog_options.dump_interval_ms = 1000;
    binlog_options.use_active_list = true;
    binlog_options.single_file_limit = g_single_file_limit;

    BinLog binlog(binlog_options);
    ret = binlog.open(g_binlog_dir);
    if (ret != 0) {
        fatal("binlog.open failed. ret:%d\n", ret);
    }

    uint64_t t = binlog.next_binlog_id();

    BinLogWriter *writer = binlog.writer();
    if (!writer) {
        fatal("binlog.writer failed\n");
    }
    BinLogReader *reader = binlog.fetch_reader();
    if (!reader) {
        fatal("binlog.fetch_reader failed\n");
    }

    log_debug("-------------------- START test --------------------");
    log_warning("-------------------- START test --------------------");

    test_append(writer);
    test_read(reader);

    binlog.put_back_reader(reader);

    usleep(100*1000); /*waiting for log thread*/
    return 0;
}

