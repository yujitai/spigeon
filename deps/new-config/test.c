/**
 * @file test.c
 * @author liuqingjun(com@baidu.com)
 * @date 2013/01/07 14:45:00
 * @brief 
 *  
 **/

#include <stdio.h>
#include "config.h"

int main(int argc, char **argv) {
    config_t *c;
    config_node_t *node;
    if (argc < 3) {
        printf("Usage: %s file path\n", argv[0]);
        return 1;
    }
    config_init(&c);
    if (config_read_file(c, argv[1])) {
        printf("%s\n", config_error(c));
        return 0;
    }
    node = config_lookup(c, argv[2]);
    if (node) {
        printf("value = %s\n", config_node_value(node));
    } else {
        printf("not found\n");
    }
    config_destroy(c);
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
