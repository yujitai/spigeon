#include "config_file.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util/utils.h"
#include "util/sds.h"
#include "util/zmalloc.h"
#include "util/log.h"
#include "new-config/config.h"


namespace store {
#define CONFIGLINE_MAX 1024

command_t *find_command(command_t *cmd_table, const char *name)
{
    command_t *cp = cmd_table;
    while (!is_null_command(*cp)) {
        if (strcasecmp(name, cp->name) == 0) {
            return cp;
        }
        cp++;
    }
    return NULL;
}

int parse_conf(config_node_t *node, command_t *cmd_table, void *conf) {
    const char *cmd_name = config_node_key(node);
    const char *cmd_args = config_node_value(node);

    if (!cmd_name || !cmd_args) {
        return CONFIG_ERROR;
    }
    
    int argc;
    sds *argv = sdssplitargs(cmd_args, &argc);

    command_t *cmd = find_command(cmd_table, cmd_name);
    if (cmd) {
        if (cmd->handler(argc, argv, cmd, conf) != CONFIG_OK) {
            zfree(argv);
            return CONFIG_ERROR;
        }
    }
    zfree(argv);
    return CONFIG_OK;
}

int load_conf_file(const char *filename, command_t *cmd_table, void *conf) {
    config_t *c;
    config_node_t *root, *node;
    config_init(&c);
    if (config_read_file(c, filename)) {
        fprintf(stderr, "can not read config file\n");
        return CONFIG_ERROR;
    }
    root = config_root(c);
    if (!root) {
        return CONFIG_ERROR;
    }
    node = config_node_first_child(root);
    if (!node) {
        return CONFIG_ERROR;
    }
    if (parse_conf(node, cmd_table, conf) != CONFIG_OK) {
        return CONFIG_ERROR;
    }
    while ((node = config_next(node)) != NULL) {
        if (parse_conf(node, cmd_table, conf) != CONFIG_OK) {
            return CONFIG_ERROR;
        }
    }
    config_destroy(c);
    return CONFIG_OK;
}

int conf_set_flag_slot(int argc, char **argv, command_t *cmd, void *conf)
{
    if (argc < 1) {
        return CONFIG_ERROR;
    }
  
    char *p = (char*)conf;
    flag_t *fp = (flag_t*)(p + cmd->offset);
  
    if (!strcasecmp(argv[0], "yes")) {
        *fp = 1;
    } else if (!strcasecmp(argv[0], "no")) {
        *fp = 0;
    } else {
        fprintf(stderr, "invalid flag in %s, should be yes or no\n", cmd->name);
        return CONFIG_ERROR;
    }
    return CONFIG_OK;
}

int conf_set_usec_slot(int argc, char **argv, command_t *cmd, void *conf)
{
    if (argc < 1) {
        return CONFIG_ERROR;
    }
  
    char *p = (char*)conf;
    long long *lp = (long long*)(p + cmd->offset);
    *lp = timetoll(argv[0], NULL);
    return CONFIG_OK;
}

int conf_set_str_slot(int argc, char **argv, command_t *cmd, void *conf)
{
    if (argc < 1) {
        return CONFIG_ERROR;
    }
  
    char *p = (char*)conf;
    char **sp = (char**)(p + cmd->offset);
    *sp = zstrdup(argv[0]);
    return CONFIG_OK;
}

int conf_set_num_slot(int argc, char **argv, command_t *cmd, void *conf)
{
    if (argc < 1) {
        return CONFIG_ERROR;
    }
  
    char *p = (char*)conf;
    int *np = (int*)(p + cmd->offset);
    *np = atoi(argv[0]);
    return CONFIG_OK;
}

int conf_set_mem_slot(int argc, char **argv, command_t *cmd, void *conf)
{
    if (argc < 1) {
        return CONFIG_ERROR;
    }
  
    char *p = (char*)conf;
    long long *np = (long long*)(p + cmd->offset);
    *np = memtoll(argv[0], NULL);
    return CONFIG_OK;
}

}
