/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file store_binlog_fix.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/30 17:44:07
 * @brief 
 **/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <mc_pack.h>

#include "binlog/binlog.h"
#include "util/log.h"
#include "server/event.h"

using namespace store;

enum cmd_t {
	CMD_SHOW = 1,
	CMD_FIX = 2,
};

struct conf_t {
	char *binlog_dir;
	char *binlog_name;
	bool verbose;
	cmd_t cmd;
    int log_level;
} g_conf;

bool g_is_dirty = false;

#define fatal(fmt, arg...) do {\
    printf("++ ERROR "fmt"\n", ##arg); \
    usleep(100*1000); \
    exit(-1); \
} while (0)

#define warning(fmt, arg...) do {\
    printf("++ ERROR "fmt"\n", ##arg); \
} while (0)

#define notice(fmt, arg...) do {\
    printf("++ "fmt"\n", ##arg); \
} while (0)

#define debug(fmt, arg...) do {\
	if (g_conf.verbose) { \
		printf("-- "fmt"\n", ##arg); \
	} \
} while (0)

void print_help()
{
	printf("store-binlog-tool is designed for store to read binlog\n");
    printf("Usage: %s [OPTION]\n", "store-binlog-tool");
    printf("\n");
    printf("  -p binlog_dir     dir of binlog. default: ./data/binlog\n");
    printf("  -b binlog_name    name of binlog. default: binlog\n");
    printf("\n");
    printf("  -c show|fix      show: show binlog info; fix: fix last dirty binlog. default: show\n");
    printf("  -v                verbose\n");
    printf("  -h                display this help and exit\n");
    printf("\n");
    printf("Example: store-binlog-tool -c show\n");
    printf("Example: store-binlog-tool -c fix\n");
    printf("Report bugs to <li_zhe@baidu.com>");
    printf("\n");
    exit(0);
}

void load_opt(int argc, char * argv[])
{
	if (argc <= 1) {
		print_help();
	}

	memset(&g_conf, 0, sizeof(g_conf));
	g_conf.binlog_dir = "./data/binlog";
	g_conf.binlog_name = "binlog";
    g_conf.cmd = CMD_SHOW;
    g_conf.log_level = STORE_LOG_NOTICE;

    char c = '\0';
    while (-1 != (c = getopt(argc, argv, "p:b:c:vh?"))) {
        switch(c) {
		case 'p':
			g_conf.binlog_dir = optarg;
			break;
		case 'b':
			g_conf.binlog_name = optarg;
			break;
        case 'c':
            if (strcmp(optarg, "show") == 0) {
                g_conf.cmd = CMD_SHOW;
            } else if (strcmp(optarg, "fix") == 0) {
                g_conf.cmd = CMD_FIX;
            } else {
				warning("invalid parameter: cmd");
                exit(-1);
            }
            break;
        case 'v':
			g_conf.verbose = true;
			break;
		case 'o':
			g_conf.log_level = strtoul(optarg, NULL, 10);
			break;
        case 'h':
        case '?':
        default:
            print_help();
            break;
        }
    }

	debug("binlog_dir:%s", g_conf.binlog_dir);
	debug("binlog_name:%s", g_conf.binlog_name);

	if (g_conf.binlog_dir == NULL
		|| g_conf.binlog_name == NULL)
    {
		warning("invalid parameter");
		exit(-1);
	}

	if (g_conf.cmd == CMD_SHOW) {
        debug("cmd:%s", "show");
    } else if (g_conf.cmd == CMD_FIX) {
		debug("cmd:%s", "fix last dirty binlog");
	}
}

int open_binlog_with_read_only()
{
    BinLogOptions g_binlog_options(true/*read_only*/);
    BinLog binlog(g_binlog_options);
    BinLogReader *reader = NULL;

	int ret = binlog.open(g_conf.binlog_dir, g_conf.binlog_name);
    if (ret != 0) {
		fatal("binlog.open failed");
	}

	notice("binlog: first_binlog_id:0x%lX, last_binlog_id:0x%lX, checkpoint:0x%lX",
        binlog.first_binlog_id(), binlog.last_binlog_id(),
        binlog.checkpoint_binlog_id());

    uint64_t single_file_limit;
    int first_idx;
    int cur_idx;
    uint64_t first_pos;
    uint64_t cur_pos;
    binlog.debug_info(single_file_limit, first_idx, cur_idx, first_pos, cur_pos);
    notice("big_file: single_file_limit:0x%lX, first_idx:%d, cur_idx:%d, "
        "first_pos:0x%lX, cur_pos:0x%lX",
        single_file_limit, first_idx, cur_idx, first_pos, cur_pos);

    reader = binlog.fetch_reader();
    if (!reader) {
        fatal("binlog.fetch_reader failed");
    }

    // read last binlog id
    if (cur_pos == 0) {
        notice("last_binlog: 0");
    } else {
        ret = reader->read(binlog.last_binlog_id());
        if (ret != 0) {
            fatal("read last binlog failed");
        }
        notice("last_binlog: cur:0x%lX, prev:0x%lX, next:0x%lX, "
            "checksum:0x%X, timestamp_us:%lu, data_len:%u, total_len:%u",
            reader->seq().cur, reader->seq().prev, reader->seq().next,
            reader->head().checksum, reader->head().timestamp_us,
            reader->head().data_len, reader->head().total_len);
        if (reader->seq().next != cur_pos) {
            warning("may have dirty data at the end of file. dirty_pos:0x%lX, cur_pos:0x%lX",
                reader->seq().next, cur_pos);
            g_is_dirty = true;
        } else {
            notice("GOOD. no dirty found at the end");
        }
    }

    if (reader) {
        binlog.put_back_reader(reader);
    }

	return 0;
}

int open_binlog_with_truncate_dirty()
{
    notice("truncate dirty data...");

    BinLogOptions binlog_options;
    BinLog binlog(binlog_options);
    binlog.set_force_truncate_dirty(true);

	int ret = binlog.open(g_conf.binlog_dir, g_conf.binlog_name);
    if (ret != 0) {
		fatal("binlog.open failed");
	}
    notice("DONE. truncate dirty data succeed");

	return 0;
}

int show_binlog()
{
	if (open_binlog_with_read_only() != 0) {
        fatal("open_binlog_with_read_only failed");
		return -1;
	}
    return 0;
}

int fix_binlog()
{
	if (open_binlog_with_read_only() != 0) {
        fatal("open_binlog_with_read_only failed");
		return -1;
	}
    if (!g_is_dirty) {
        notice("binlog is not dirty at the end");
        return 0;
    }

    if (open_binlog_with_truncate_dirty() != 0) {
        fatal("truncate dirty failed");
    }
    return 0;
}

EventLoop g_el(NULL, true);
void run_event()
{
    g_el.run();
}

int main(int argc, char *argv[])
{
    int ret = 0;

	load_opt(argc, argv);

	ret = log_init("./log", "store-binlog-tool", g_conf.log_level);
    if (ret != 0) {
        fatal("log_init failed. ret:%d", ret);
    }
    run_event(); // 为了从libev中获取时间戳
    //notice("%llu\n", EventLoop::current_time());

	debug("");

	if (g_conf.cmd == CMD_SHOW) {
		if (show_binlog() < 0) {
			goto failed;
		}
    } else if (g_conf.cmd == CMD_FIX) {
		if (fix_binlog() < 0) {
			goto failed;
		}
	}

    usleep(100*1000); /*waiting for log thread*/ \
	return 0;
failed:
    usleep(100*1000); /*waiting for log thread*/ \
	return -1;
}

