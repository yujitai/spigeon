/***************************************************************************
 *
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 *
 **************************************************************************/

/**
 * @file mcpack_print.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/06/19 22:49:01
 * @brief modified from MCPackHelper
 **/

#include <stdio.h>
#include <assert.h>

#include <mc_pack.h>

static const char *type2string(int type)
{
	switch (type) {
	case MC_PT_BAD:
		return "bad";
	case MC_PT_PCK:
		return "pack";
	case MC_PT_OBJ:
		return "object";
	case MC_PT_ARR:
		return "array";
	case MC_IT_BIN:
		return "raw";
	case MC_IT_TXT:
		return "string";
	case MC_IT_SGN:
		return "signed";
	case MC_IT_UNS:
		return "unsigned";
	case MC_IT_32B:
		return "32b";
	case MC_IT_64B:
		return "64b";
	case MC_IT_I32:
		return "int32";
	case MC_IT_U32:
		return "uint32";
	case MC_IT_I64:
		return "int64";
	case MC_IT_U64:
		return "uint64";
	default:
		return "undefined";
	}
}

#define STR_TAB "    "
#define PRINT(fmt, arg...) do {\
	snprintf_ret = snprintf(cur, left, fmt, ##arg); \
	if (snprintf_ret < 0) { \
		return -1; \
	} else if (snprintf_ret >= left) { \
		return MC_PE_NO_SPACE; \
	} \
	cur += snprintf_ret; \
	left -= snprintf_ret; \
} while (0)

static int mcpack_print(char *buf, size_t buf_len, const mc_pack_t *pack, int tabs);

static int mcpack_print(char *buf, size_t buf_len, const mc_pack_item_t &item, int tabs)
{
	int ret = 0;
	int snprintf_ret = 0;
	char *cur = buf;
	int left = buf_len;

	for (int i = 0; i < tabs; ++i) {
		PRINT(STR_TAB);
	}
	const mc_pack_t *pack;
	switch (item.type) {
	case MC_PT_OBJ:
		PRINT("%s : (%s)\n", item.key, type2string(item.type));
		if (ret = mc_pack_get_pack_from_item(&item, &pack) < 0) {
			return -1;
		}
		if ((ret = mcpack_print(cur, left, pack, tabs)) < 0) {
			return ret;
		}
		cur += ret;
		left -= ret;
		break;
	case MC_PT_ARR:
		PRINT("%s : (%s)\n", item.key, type2string(item.type));
		if (ret = mc_pack_get_array_from_item(&item, &pack) < 0) {
			return -1;
		}
		if ((ret = mcpack_print(cur, left, pack, tabs)) < 0) {
			return ret;
		}
		cur += ret;
		left -= ret;
		break;
	case MC_IT_BIN:
		PRINT("%s : (%s)\n", item.key, type2string(item.type));
		for (int i = 0; i < (int)item.value_size; ++i) {
			PRINT("%3hhX", item.value[i]);
		}
		PRINT("\n");
		break;
	case MC_IT_TXT:
		PRINT("%s : (%s)%s\n", item.key, type2string(item.type), item.value);
		break;
	case MC_IT_I32:
		PRINT("%s : (%s)%d\n", item.key, type2string(item.type), *(int32_t *)item.value);
		break;
	case MC_IT_U32:
		PRINT("%s : (%s)%u\n", item.key, type2string(item.type), *(u_int32_t *)item.value);
		break;
	case MC_IT_I64:
		PRINT("%s : (%s)%ld\n", item.key, type2string(item.type), *(int64_t *)item.value);
		break;
	case MC_IT_U64:
		PRINT("%s : (%s)%ld\n", item.key, type2string(item.type), *(u_int64_t *)item.value);
		break;
	default:
		PRINT("TYPE NOT SUPPORTED.");
		return -1;
	}
	return (cur - buf);
}

static int mcpack_print(char *buf, size_t buf_len, const mc_pack_t *pack, int tabs)
{
	int ret = 0;
	int snprintf_ret = 0;
	char *cur = buf;
	int left = buf_len;

	for (int i = 0; i < tabs; ++i) {
		PRINT("%s", STR_TAB);
	}
	PRINT("{\n");

	int err = 0;
	mc_pack_item_t item;
	err = mc_pack_first_item(pack, &item);
	while (MC_PE_SUCCESS == err) {
		if ((ret = mcpack_print(cur, left, item, tabs + 1)) < 0) {
			return ret;
		}
		cur += ret;
		left -= ret;
		err = mc_pack_next_item(&item, &item);
	}

	for (int i = 0; i < tabs; ++i) {
		PRINT("%s", STR_TAB);
	}
	PRINT("}\n");

	return (cur - buf);
}

int mcpack_format(const mc_pack_t *pack, char *buf, size_t buf_len)
{
	assert(NULL != buf && NULL != pack);

	int ret = 0;
	int snprintf_ret = 0;
	char *cur = buf;
	int left = buf_len;

	if ((ret = mcpack_print(buf, buf_len, pack, 0)) < 0) {
		PRINT("invalid pack\n");
		return ret;
	} else {
		//PRINT("\n");
		return (cur - buf);
	}
}


