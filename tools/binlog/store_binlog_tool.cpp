/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file store_binlog_tool.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/25 13:15:23
 * @brief binlog工具
 **/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <mc_pack.h>

#include "binlog/binlog.h"
#include "util/log.h"
#include "server/event.h"

#include "mcpack_format.h"

using namespace store;

enum cmd_t {
	CMD_SHOW = 1,
	CMD_READ_BINLOG = 2,
	//CMD_FIND_BINLOG_ID, //not supported yet
};

struct conf_t {
	char *binlog_dir;
	char *binlog_name;
	bool verbose;
	uint32_t buf_len;
	cmd_t cmd;

	uint64_t start_binlog_id;
	int num;
	bool print_mcpack;
	bool print_mcpack_beauty;

    int log_level;
} g_conf;

BinLogOptions g_binlog_options(true/*read_only*/);
BinLog g_binlog(g_binlog_options);
BinLogReader *g_reader = NULL;
char *g_tmp_buf = NULL;
char *g_text_buf = NULL;
time_t g_time;
struct tm g_tm;

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
    printf("  -c show|read      show: show binlog info; read: read binlog. default: read\n");
    printf("  -i binlog_id      start binlog_id\n");
	printf("  -n num            num of binlog. default:1\n");
	//printf("  -t time           time of the binlog to find, format:1339139233 or '2012-01-01 12:00:00'\n");
    printf("\n");
    printf("  -m                print mcpack.\n");
    printf("  -e                print mcpack with ident.\n");
    printf("  -v                verbose\n");
    printf("  -l length         buffer length. default:1024*1024*50\n");
    printf("  -o log_level      log level. default:4\n");
    printf("  -h                display this help and exit\n");
    printf("\n");
    printf("Example: store-binlog-tool -v -e -i 10 -n 3\n");
    //printf("Example: store-binlog-tool -v -m -c find -t 1339139233\n");
    //printf("Example: store-binlog-tool -v -m -c find -t '2012-01-02 12:10:20'\n");
    printf("Report bugs to <li_zhe@baidu.com>");
    printf("\n");
    exit(0);
}

uint64_t smart_parse_int(const char *data)
{
    if (strlen(data) > 2 && data[0] == '0' && (data[1] == 'x' || data[1] == 'X')) {
        return strtoull(data+2, NULL, 16);
    } else {
        return strtoull(data, NULL, 10);
    }
}

void load_opt(int argc, char * argv[])
{
	if (argc <= 1) {
		print_help();
	}

	memset(&g_conf, 0, sizeof(g_conf));
	g_conf.binlog_dir = "./data/binlog";
	g_conf.binlog_name = "binlog";
	g_conf.buf_len = 1024 * 1024 * 50;
    g_conf.cmd = CMD_READ_BINLOG;
	g_conf.num = 1;
    g_conf.log_level = STORE_LOG_NOTICE;

	g_tm.tm_year = 0;

    char c = '\0';
    while (-1 != (c = getopt(argc, argv, "p:b:c:i:n:mevl:o:h?"))) {
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
            } else if (strcmp(optarg, "read") == 0) {
                g_conf.cmd = CMD_READ_BINLOG;
            //} else if (strcmp(optarg, "find") == 0) {
                //g_conf.cmd = CMD_FIND_BINLOG_ID;
            } else {
				warning("invalid parameter: cmd");
                exit(-1);
            }
            break;
		case 'i':
			g_conf.start_binlog_id = smart_parse_int(optarg);
			break;
		case 'n':
			g_conf.num = strtoul(optarg, NULL, 10);
			if (g_conf.num < 1) {
				warning("invalid parameter: num");
				exit(-1);
			}
			break;
		//case 't':
			//g_time = strtoul(optarg, NULL, 10);
			//if (g_time >= 946656000 [>'2000-01-01 00:00:00'<]
				//&& g_time <= 4102416000[>'2100-01-01 00:00:00'<]) {
				//// 数字模式
				//localtime_r(&g_time, &g_tm);
			//} else {
				//// 字符串模式
				//if (strptime(optarg, "%Y-%m-%d %H:%M:%S", &g_tm) == NULL) {
					//warning("输入参数time非法");
					//exit(-1);
				//}
				//g_time = mktime(&g_tm);
			//}
			//break;
		case 'm':
			g_conf.print_mcpack = true;
			break;
		case 'e':
			g_conf.print_mcpack_beauty = true;
			break;
        case 'v':
			g_conf.verbose = true;
			break;
		case 'l':
			g_conf.buf_len = strtoul(optarg, NULL, 10);
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
	debug("buf_len:%u", g_conf.buf_len);

	if (g_conf.binlog_dir == NULL
		|| g_conf.binlog_name == NULL)
    {
		warning("invalid parameter");
		exit(-1);
	}

	if (g_conf.cmd == CMD_SHOW) {
        debug("cmd:%s", "show");
    } else if (g_conf.cmd == CMD_READ_BINLOG) {
		debug("cmd:%s", "read binlog");
		debug("start_binlog_id:0x%lX", g_conf.start_binlog_id);
		debug("num:%u", g_conf.num);
	//} else {
		//debug("cmd:%s", "find binlog_id");
		//char time_buf[25];
		//strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &g_tm);
		//debug("time:%lu, %s",
			//g_time, time_buf);
		////debug("time:%lu year:%d, month:%d, day:%d, hour:%d, min:%d, sec:%d",
			////g_time, g_tm.tm_year+1900, g_tm.tm_mon+1, g_tm.tm_mday, g_tm.tm_hour, g_tm.tm_min, g_tm.tm_sec);
	}
}

int open_binlog()
{
	g_tmp_buf = new (std::nothrow) char[g_conf.buf_len];
	g_text_buf = new (std::nothrow) char[g_conf.buf_len];
	if (g_tmp_buf == NULL || g_text_buf == NULL) {
		fatal("new buf failed");
	}

	int ret = g_binlog.open(g_conf.binlog_dir, g_conf.binlog_name);
    if (ret != 0) {
		fatal("binlog.open failed");
	}

    uint32_t page_size;
    uint32_t map_size;
    MmapSeqWritableFile::debug_info(page_size, map_size);
    notice("page_size:0x%X, map_size:0x%X", page_size, map_size);

    uint64_t single_file_limit;
    int first_idx;
    int cur_idx;
    uint64_t first_pos;
    uint64_t cur_pos;
    g_binlog.debug_info(single_file_limit, first_idx, cur_idx, first_pos, cur_pos);
    notice("big_file: single_file_limit:0x%lX, first_idx:%d, cur_idx:%d, "
        "first_pos:0x%lX, cur_pos:0x%lX",
        single_file_limit, first_idx, cur_idx, first_pos, cur_pos);

	notice("binlog: first_binlog_id:0x%lX, last_binlog_id:0x%lX, "
        "next_binlog_id:0x%lX, checkpoint:0x%lX",
        g_binlog.first_binlog_id(), g_binlog.last_binlog_id(),
        g_binlog.next_binlog_id(), g_binlog.checkpoint_binlog_id());

    g_reader = g_binlog.fetch_reader();
    if (!g_reader) {
        fatal("binlog.fetch_reader failed");
    }

    // read last binlog id
    if (cur_pos == 0) {
        notice("last_binlog: non exist");
    } else if (g_binlog.next_binlog_id() == 0) {
        notice("last_binlog: non exist");
        if (0 != cur_pos) {
            warning("may have dirty data at the end of file. dirty_pos:0x%u, cur_pos:0x%lX",
                0, cur_pos);
        }
    } else {
        ret = g_reader->read(g_binlog.last_binlog_id());
        if (ret != 0) {
            fatal("read last binlog failed");
        }
        notice("last_binlog: cur:0x%lX, prev:0x%lX, next:0x%lX, "
            "checksum:0x%X, timestamp_us:%lu, data_len:%u, total_len:%u",
            g_reader->seq().cur, g_reader->seq().prev, g_reader->seq().next,
            g_reader->head().checksum, g_reader->head().timestamp_us,
            g_reader->head().data_len, g_reader->head().total_len);
        if (g_reader->seq().next != cur_pos) {
            warning("may have dirty data at the end of file. dirty_pos:0x%lX, cur_pos:0x%lX",
                g_reader->seq().next, cur_pos);
        }
    }

	return 0;
}

int print_pack(const char *data_buf, int data_len, uint64_t binlog_id) {
    int ret = 0;

	// mcpack
	mc_pack_t *pack = mc_pack_open_r(data_buf, data_len, g_tmp_buf, g_conf.buf_len);
    if (mc_pack_valid(pack) == 0) {
        warning("binlog_id:0x%lX, open mcpack falied", binlog_id);
        return -1;
    }

	if (g_conf.print_mcpack) {
		ret = mc_pack_pack2text(pack, g_text_buf, g_conf.buf_len, 0);
		if (ret == MC_PE_NO_SPACE) {
			warning("binlog_id:0x%lX, buf for text not enough", binlog_id);
            goto failed;
		} else if (ret != 0) {
			warning("binlog_id:0x%lX, pack2text failed", binlog_id);
            goto failed;
		}
		notice("binlog_id:0x%lX, PACK, version:%u, text:%s",
			binlog_id, mc_pack_get_version(pack), g_text_buf);
	}
	if (g_conf.print_mcpack_beauty) {
		ret = mcpack_format(pack, g_text_buf, g_conf.buf_len);
		if (ret == MC_PE_NO_SPACE) {
			warning("binlog_id:0x%lX, buf for text not enough", binlog_id);
            goto failed;
		} else if (ret != 0) {
			warning("binlog_id:0x%lX, mcpack_format failed", binlog_id);
            goto failed;
		}
		notice("binlog_id:0x%lX, PACK, version:%u, format:%s",
			binlog_id, mc_pack_get_version(pack), g_text_buf);
	}
	mc_pack_close(pack);
    return 0;
failed:
	mc_pack_close(pack);
	return -1;
}

int do_read_binlog(uint64_t binlog_id)
{
	int ret = 0;

    // binlog
    ret = g_reader->read(binlog_id);
    if (ret != 0) {
        warning("read binlog_id failed. binlog_id:0x%lX, ret:%d", binlog_id, ret);
        return -1;
    }
    notice("binlog_id:0x%lX, binlog_id_in_head:0x%lX, "
        "ts:%lu, total_len:%u, data_len:%u, prev:0x%lX, next:0x%lX",
        binlog_id, g_reader->head().binlog_id,
        g_reader->head().timestamp_us, g_reader->head().total_len, g_reader->head().data_len,
        g_reader->seq().prev, g_reader->seq().next);

    if (g_conf.print_mcpack || g_conf.print_mcpack_beauty) {
        print_pack(g_reader->data(), g_reader->data_len(), binlog_id);
    }

    return 0;
}

int read_binlog()
{
    uint64_t current = g_conf.start_binlog_id;
	for (int i = 0; i < g_conf.num; ++i) {
        if (g_reader->eof(current)) {
			warning("binlog_id:0x%lX, reach the end of binlog:0x%lX", current, g_binlog.last_binlog_id());
			return -1;
		}
		if (do_read_binlog(current) < 0) {
			return -1;
		}
        current = g_reader->next_binlog_id();
	}
	return 0;
}

int show_binlog()
{
    //nothing
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

	if (open_binlog() < 0) {
		return -1;
	}

	if (g_conf.cmd == CMD_SHOW) {
		if (show_binlog() < 0) {
			goto failed;
		}
    } else if (g_conf.cmd == CMD_READ_BINLOG) {
		if (read_binlog() < 0) {
			goto failed;
		}
	//} else if (g_conf.cmd == CMD_FIND_BINLOG_ID) {
		//if (find_binlog_id() < 0) {
			//goto failed;
		//}
	}

    if (g_reader) {
        g_binlog.put_back_reader(g_reader);
    }
    usleep(100*1000); /*waiting for log thread*/ \
	return 0;
failed:
    usleep(100*1000); /*waiting for log thread*/ \
	return -1;
}

// not tested
/*
uint32_t read_binlog_time_s(uint64_t binlog_id)
{
	int ret = g_reader->read(binlog_id);
    if (ret != 0) {
        warning("read binlog_id failed. binlog_id:0x%lX, ret:%d", binlog_id, ret);
		return 0; // 表示错误
    }
    return g_reader->head().timestamp_us / 1000000;
}

void result_found(uint64_t binlog_id, time_t binlog_time)
{
	struct tm tmp_tm;
	char time_buf[25];
	localtime_r(&binlog_time, &tmp_tm);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tmp_tm);

	notice("next binlog after time[%lu]. binlog_id:0x%lX, write_time:%lu, %s",
		g_time, binlog_id, binlog_time, time_buf);

	do_read_binlog(binlog_id);
}

int find_binlog_id()
{
	if (g_time == 0 || g_tm.tm_year == 0
		|| g_time < 946656000 //'2000-01-01 00:00:00'
		|| g_time > 4102416000 //'3000-01-01 00:00:00'
    ) {
		fatal("输入参数time非法");
	}

	uint64_t min_binlog_id = g_binlog.first_binlog_id();
    uint64_t max_binlog_id = g_binlog.last_binlog_id();
	debug("min_binlog_id:0x%lX", min_binlog_id);
	debug("max_binlog_id:0x%lX", max_binlog_id);

	time_t binlog_time;
	binlog_time = read_binlog_time_s(min_binlog_id);
	if (binlog_time == 0) {
		return -1; // 失败
	}
	if (g_time <= binlog_time) {
		result_found(min_binlog_id, binlog_time);
		return 0;
	}

	binlog_time = read_binlog_time_s(max_binlog_id);
	if (binlog_time == 0) {
		return -1; // 失败
	}
	if (g_time > binlog_time) {
		warning("time[%lu] exceed the max binlog time[%lu]", g_time, binlog_time);
		return -1; // 失败
	}
	
	// 顺序查找
    for (uint64_t current = min_binlog_id;
        current <= max_binlog_id; 
        current = g_reader->next_binlog_id())
    {
		binlog_time = read_binlog_time_s(current);
        if (g_time <= binlog_time) {
            result_found(min_binlog_id, binlog_time);
            return 0;
        }
    }
    fatal("should not happen");
	return -1;
}
*/

