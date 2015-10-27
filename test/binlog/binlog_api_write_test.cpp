/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file binlog_api_write_test.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/20 15:51:22
 * @brief 
 **/

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

#include <deque>

#include "binlog/binlog.h"
#include "util/log.h"
#include "server/event.h"

using namespace store;

#define notice(...) printf(__VA_ARGS__)
#define fatal(...) \
do { \
    printf(__VA_ARGS__); \
    usleep(10*1000); /*waiting for log thread*/ \
    abort(); \
    /*raise(SIGKILL);*/ \
} while (0) \

const int g_writer_thread_num = 10;
uint64_t g_single_file_limit = 1 << 20;
const uint64_t g_max_data_len = 10 * 1024 * 1024;
const std::string g_binlog1_dir = "./data/binlog";
const std::string g_binlog2_dir = "./data/binlog2";

int g_count = 100;
uint64_t g_start_binlog_id = 0;
bool g_reader_end = false;

std::deque<char*> g_task_list;
Mutex g_mutex;
CondVar g_cond(g_mutex);

void do_read(BinLogReader *reader)
{
    uint64_t binlog_id = g_start_binlog_id;
    for (int i = 0; i < g_count; ++i) {
        while (reader->eof(binlog_id)) {
            notice("reach the end of binlog. %lu. sleep for a while\n", binlog_id);
            usleep(1000*1000);
        }

        int ret = reader->read(binlog_id);
        if (ret != 0) {
            fatal("reader.read failed. binlog_id:%lu, ret:%d\n", binlog_id, ret);
        }

        char *buf = new char[reader->head().total_len];
        ASSERT(buf);
        memcpy(buf, reader->buf(), reader->head().total_len);

        {
            MutexHolder holder(g_mutex);
            g_task_list.push_back(buf);
            notice("push task into list. size:%lu\n", g_task_list.size());
        }
        g_cond.broadcast();

        notice("READ finished. binlog_id:%lu, ts:%lu, total_len:%u, data_len:%u\n",
            binlog_id, reader->head().timestamp_us, reader->head().total_len, reader->head().data_len);
        binlog_id = reader->next_binlog_id();
    }
    g_reader_end = true;
    g_cond.broadcast();
}

void do_write(BinLogWriter *writer)
{
    while (true) {
        char *buf = NULL;
        while (true) {
            MutexHolder holder(g_mutex);
            if (!g_task_list.empty()) {
                buf = g_task_list[0];
                ASSERT(buf);
                g_task_list.pop_front();
                notice("got task from list. size:%lu\n", g_task_list.size());
                break;
            } else if (g_reader_end) {
                return;
            }
            //notice("waiting for task...\n");
            g_cond.wait();
        }

        notice("simulate busy...\n");
        usleep((rand() % 100 + 10) * 1000);

        ASSERT(buf);
        binlog_head_t *head = (binlog_head_t*)buf;
        binlog_id_seq_t seq;
        binlog_head_t out_head;
        int ret = writer->write(head->binlog_id, head->timestamp_us,
            head->data, head->data_len, &seq, &out_head);
        if (ret != 0) {
            fatal("writer write failed. binlog_id:0x%lX, len:%d, ret:%d\n",
                head->binlog_id, head->data_len, ret);
        }
        if (out_head.checksum != head->checksum) {
            fatal("out_head->checksum not match");
        }
        notice("WRITE finished. binlog_id:0x%lX, len:%d, ret:%d\n",
            head->binlog_id, head->data_len, ret);

        writer->active_binlog_mark_done(seq);

        delete [] buf;
    }
}

void *reader_thread_fun(void *ptr)
{
    assert(ptr);
    BinLog *binlog = (BinLog*)ptr;

    BinLogReader *reader = binlog->fetch_reader();
    if (!reader) {
        fatal("binlog.fetch_reader failed\n");
    }

    do_read(reader);

    binlog->put_back_reader(reader);

    return 0;
}

void *writer_thread_fun(void *ptr)
{
    assert(ptr);
    BinLog *binlog = (BinLog*)ptr;

    BinLogWriter *writer = binlog->writer();
    if (!writer) {
        fatal("binlog.writer failed\n");
    }

    do_write(writer);

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

    ret = log_init("./log", "binlog_api_write_test", STORE_LOG_NOTICE);
    //ret = log_init("./log", "binlog_api_write_test", STORE_LOG_DEBUG);
    if (ret != 0) {
        fatal("log_init failed\n");
    }
    run_event(); // 为了从libev中获取时间戳
    //notice("%llu\n", EventLoop::current_time());

    log_debug("-------------------- START log --------------------");
    log_warning("-------------------- START log --------------------");

    BinLogOptions binlog_options1;
    binlog_options1.read_only = true;

    BinLog binlog1(binlog_options1);
    ret = binlog1.open(g_binlog1_dir);
    if (ret != 0) {
        fatal("binlog1 open failed. ret:%d\n", ret);
    }

    BinLogOptions binlog_options2;
    binlog_options2.read_only = false;
    binlog_options2.dump_interval_ms = 10*1000;
    binlog_options2.use_active_list = true;
    binlog_options2.single_file_limit = g_single_file_limit;

    BinLog binlog2(binlog_options2);
    ret = binlog2.open(g_binlog2_dir);
    if (ret != 0) {
        fatal("binlog2 open failed. ret:%d\n", ret);
    }

    g_start_binlog_id = binlog2.next_binlog_id();
    notice("binlog2.next_binlog_id: 0x%lX\n", binlog2.next_binlog_id());

    log_debug("-------------------- START test --------------------");
    log_warning("-------------------- START test --------------------");

    pthread_t reader_thread;
    ret = pthread_create(&reader_thread, NULL, reader_thread_fun, &binlog1);
    assert(ret == 0);

    pthread_t writer_thread[g_writer_thread_num];
    for (int i = 0; i < g_writer_thread_num; ++i) {
        ret = pthread_create(&writer_thread[i], NULL, writer_thread_fun, &binlog2);
        assert(ret == 0);
    }

    ret = pthread_join(reader_thread, NULL);
    assert(ret == 0);

    for (int i = 0; i < g_writer_thread_num; ++i) {
        ret = pthread_join(writer_thread[i], NULL);
        assert(ret == 0);
    }

    usleep(100*1000); /*waiting for log thread*/
    return 0;
}

