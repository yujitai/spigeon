/**
 * @file config.h
 * @author liuqingjun(com@baidu.com)
 * @date 2013/01/05 23:36:09
 * @brief 
 *  
 **/


#ifndef  __CONFIG_H_
#define  __CONFIG_H_

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif
    
typedef enum config_node_type_t {
    CONFIG_TYPE_GROUP = 1,
    CONFIG_TYPE_ARRAY = 2,
    CONFIG_TYPE_ITEM = 3
} config_node_type_t;

typedef struct _config_node_t config_node_t;
struct _config_node_t {
    config_node_type_t type;
    config_node_t *parent;
    config_node_t *prev_sibling;
    config_node_t *next_sibling;
    config_node_t *first_child;
    config_node_t *last_child;
    char *key;
    char *value;
    size_t key_len;
    size_t value_len;
    int lineno;
    int length;
    int index;
};

typedef struct _config_t config_t;

int config_init(config_t **config);
int config_destroy(config_t *config);
int config_read_file(config_t *config, const char *filename);
const char* config_error(config_t *config);
config_node_t* config_root(config_t *config);
config_node_t* config_lookup(config_t *config, const char *path);

const char* config_node_key(config_node_t *node);
size_t config_node_key_len(config_node_t *node);
const char* config_node_value(config_node_t *node);
size_t config_node_value_len(config_node_t *node);
int config_node_lineno(config_node_t *node);
int config_node_length(config_node_t *node);
int config_node_index(config_node_t *node);

config_node_t* config_node_parent(config_node_t *node);
config_node_t* config_node_first_child(config_node_t *node);
config_node_t* config_next(config_node_t *node);

const char* config_node_get_string(config_node_t *node);
long config_node_get_int(config_node_t *node);
double config_node_get_float(config_node_t *node);
int config_node_get_bool(config_node_t *node);

int config_lookup_string(config_t *config, const char *path, const char **value);
int config_lookup_int(config_t *config, const char *path, long *value);
int config_lookup_float(config_t *config, const char *path, double *value);
int config_lookup_bool(config_t *config, const char *path, int *value);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
