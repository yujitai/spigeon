/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file binlog_performance.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/30 09:51:22
 * @brief 
 **/

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

#include <pthread.h>

#include "binlog/binlog.h"
#include "util/log.h"
#include "server/event.h"

using namespace store;

const std::string g_binlog_dir = "./data/binlog";
uint64_t g_single_file_limit = 1 << 30;
const uint64_t g_max_data_len = 10 * 1024 * 1024;

bool g_verbose = false;

int g_writer_thread_num = 10;
int g_reader_thread_num = 10;
int g_write_count = 10;
int g_read_count = g_writer_thread_num * g_write_count;
uint32_t g_len = 1024;
uint64_t g_reader_start_binlog_id = 0;

#define notice(...) printf(__VA_ARGS__)
#define fatal(...) \
do { \
    printf(__VA_ARGS__); \
    usleep(10*1000); /*waiting for log thread*/ \
    abort(); \
    /*raise(SIGKILL);*/ \
} while (0) \

uint64_t get_timestamp_us()
{
	struct timeval tv;
	int ret = 0;
	if( 0 != (ret = gettimeofday(&tv, NULL))) {
		fatal("gettimeofday failed. [ret:%i] [errno:%i] [%m]", ret, errno);
	}   
	return tv.tv_sec * 1000000LU + tv.tv_usec;
}

void test_append(BinLogWriter *writer)
{
    char *buf = new char[g_max_data_len];
    assert(buf);
    memset(buf, 0xEE, g_max_data_len);

	uint64_t run_start = get_timestamp_us();

    uint64_t sample_time_sum = 0;
    const uint64_t sample_count = 10000;
    for (int i = 0; i < g_write_count; ++i) {
        int len = 0;
        if (g_len == 0) {
            len = rand() % g_max_data_len + 1;
        } else {
            len = g_len;
        }

        binlog_id_seq_t seq;

		uint64_t fun_start = get_timestamp_us();
        int ret = writer->append(buf, len, &seq);
		uint64_t fun_end = get_timestamp_us();
        sample_time_sum += (fun_end - fun_start);
		if (i%sample_count == (sample_count-1)) { //(g_write_count/100) == 0) {
			printf("sampling. count:%lu, avg timespan: %lu us\n",
                sample_count, sample_time_sum / sample_count);
            sample_time_sum = 0;
        }

        if (ret != 0) {
            fatal("writer.append failed. len:%d, ret:%d\n", len, ret);
        }
        if (g_verbose) {
            notice("writer.append finished. len:%d, ret:%d, cur:0x%lX, prev:0x%lX, next:0x%lX\n",
                len, ret, seq.cur, seq.prev, seq.next);
        }

        writer->active_binlog_mark_done(seq);
    }

	uint64_t run_end = get_timestamp_us();
	printf("test_append: run timespan: %lu s, avg: %lu us\n",
        (run_end-run_start)/1000000, (run_end-run_start)/g_write_count);

    delete [] buf;
}

void test_read(BinLogReader *reader, uint64_t binlog_id)
{
	uint64_t run_start = get_timestamp_us();

    for (int i = 0; i < g_read_count; ++i) {
        while (reader->eof(binlog_id)) {
            notice("reach the end of binlog. 0x%lX. sleep for a while\n", binlog_id);
            usleep(1000*1000);
        }

		uint64_t fun_start = get_timestamp_us();
        int ret = reader->read(binlog_id);
		uint64_t fun_end = get_timestamp_us();
		if (i%(g_read_count/100) == 0) {
			printf("sampling. fun timespan: %lu us\n", (fun_end-fun_start));
		}

        if (ret != 0) {
            fatal("reader.read failed. binlog_id:0x%lX, ret:%d\n", binlog_id, ret);
        }

        if (g_verbose) {
            notice("reader.read finished. ret:%d, binlog_id:0x%lX, binlog_id_in_head:0x%lX, "
                "ts:%lu, total_len:%u, data_len:%u, prev:0x%lX, next:0x%lX\n",
                ret, binlog_id, reader->head().binlog_id,
                reader->head().timestamp_us, reader->head().total_len, reader->head().data_len,
                reader->seq().prev, reader->seq().next);
        }
        binlog_id = reader->next_binlog_id();
    }

	uint64_t run_end = get_timestamp_us();
	printf("test_read: run timespan: %lu s, avg: %lu us\n",
        (run_end-run_start)/1000000, (run_end-run_start)/g_write_count);
}

void *writer_thread_fun(void *ptr)
{
    assert(ptr);
    BinLog *binlog = (BinLog*)ptr;

    BinLogWriter *writer = binlog->writer();
    if (!writer) {
        fatal("binlog.writer failed\n");
    }

    test_append(writer);

    return 0;
}

void *reader_thread_fun(void *ptr)
{
    assert(ptr);
    BinLog *binlog = (BinLog*)ptr;

    BinLogReader *reader = binlog->fetch_reader();
    if (!reader) {
        fatal("binlog.fetch_reader failed\n");
    }

    //g_reader_start_binlog_id = binlog->first_binlog_id();
    test_read(reader, g_reader_start_binlog_id);

    binlog->put_back_reader(reader);

    return 0;
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
    while (-1 != (c = getopt(argc, argv, "l:r:w:c:s:vh?"))) {
        switch(c) {
        case 'l':
			g_len = strtoul(optarg, NULL, 10);
            break;
        case 'r':
			g_reader_thread_num = strtoul(optarg, NULL, 10);
            break;
        case 'w':
			g_writer_thread_num = strtoul(optarg, NULL, 10);
            break;
        case 'c':
			g_write_count = strtoul(optarg, NULL, 10);
            break;
        case 's':
			g_single_file_limit = strtoull(optarg, NULL, 10);
            break;
        case 'v':
            g_verbose = true;
            break;
        case 'h':
        case '?':
        default:
            //print_help();
            break;
        }
    }

    g_read_count = g_write_count * (g_writer_thread_num == 0 ? 1 : g_writer_thread_num);

    notice("read_thread_num: %d\n", g_reader_thread_num);
    notice("write_thread_num: %d\n", g_writer_thread_num);
    notice("read_count: %d\n", g_read_count);
    notice("write_count: %d\n", g_write_count);
    notice("len: %u\n", g_len);
    notice("single_file_limit: 0x%lX\n", g_single_file_limit);

    notice("starting...\n");
    sleep(1);
}

int main(int argc, char *argv[])
{
    int ret = 0;

    load_opt(argc, argv);

    ret = log_init("./log", "binlog_performance", STORE_LOG_NOTICE);
    if (ret != 0) {
        fatal("log_init failed\n");
    }

    run_event(); // 为了从libev中获取时间戳
    //notice("%llu\n", EventLoop::current_time());

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

    if (g_writer_thread_num != 0) {
        g_reader_start_binlog_id = binlog.next_binlog_id();
    } else {
        g_reader_start_binlog_id = binlog.first_binlog_id();
    }

    pthread_t writer_thread[g_writer_thread_num];
    for (int i = 0; i < g_writer_thread_num; ++i) {
        ret = pthread_create(&writer_thread[i], NULL, writer_thread_fun, &binlog);
        assert(ret == 0);
    }

    pthread_t reader_thread[g_reader_thread_num];
    for (int i = 0; i < g_reader_thread_num; ++i) {
        ret = pthread_create(&reader_thread[i], NULL, reader_thread_fun, &binlog);
        assert(ret == 0);
    }

    for (int i = 0; i < g_writer_thread_num; ++i) {
        ret = pthread_join(writer_thread[i], NULL);
        assert(ret == 0);
    }
    for (int i = 0; i < g_reader_thread_num; ++i) {
        ret = pthread_join(reader_thread[i], NULL);
        assert(ret == 0);
    }

    usleep(100*1000); /*waiting for log thread*/
    return 0;
}

