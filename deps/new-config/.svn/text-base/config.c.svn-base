/**
 * @file config.c
 * @author liuqingjun(com@baidu.com)
 * @date 2013/01/05 09:45:19
 * @brief 
 *  
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "config_lexer.h"
#include "config.h"

struct _config_t {
    config_node_t *root;
    char *errmsg;
};

int asprintf(char **ret, const char *format, ...) {
    int count;
    char *buffer;
    va_list ap;
    *ret = NULL;

    va_start(ap, format);
    count = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    if (count >= 0) {
        buffer = malloc(count + 1);
        if (buffer == NULL)
            return -1;

        va_start(ap, format);
        count = vsnprintf(buffer, count + 1, format, ap);
        va_end(ap);

        if (count < 0) {
            free(buffer);
            return count;
        }
        *ret = buffer;
    }

    return count;
}

config_node_t* new_config_node() {
    return (config_node_t*) calloc(1, sizeof(config_node_t));
}

void add_child(config_node_t *parent, config_node_t *child) {
    child->parent = parent;
    if (parent->first_child == NULL) {
        parent->first_child = parent->last_child = child;
        child->prev_sibling = child->next_sibling = NULL;
    } else {
        child->prev_sibling = parent->last_child;
        child->next_sibling = NULL;
        child->prev_sibling->next_sibling = child;
        parent->last_child = child;
    }
    child->index = parent->length;
    parent->length++;
}

config_node_t* find_child(config_node_t *parent, char *key, size_t key_len) {
    config_node_t *cur;
    if (parent == NULL) return NULL;
    cur = parent->first_child;
    for(; cur; cur = cur -> next_sibling) {
        if (key_len == cur->key_len && memcmp(key, cur->key, key_len) == 0) {
            break;
        }
    }
    return cur;
}

void print_tree(config_node_t *node, int indent) {
    config_node_t *cur;
    int i;
    for(cur = node->first_child; cur; cur = cur->next_sibling) {
        if (cur->type == CONFIG_TYPE_GROUP) {
            for (i = 0; i < indent; i++) {
                printf("  ");
            }
            printf("[");
            for (i = 0; i < indent; i++) {
                printf(".");
            }
            if (node->type == CONFIG_TYPE_ARRAY) {
                printf("@%s]\n", node->key);
            } else {
                printf("%s]\n", cur->key);
            }
            print_tree(cur, indent + 1);
        } else if (cur->type == CONFIG_TYPE_ARRAY) {
            print_tree(cur, indent);
        } else {
            for (i = 0; i < indent; i++) {
                printf("  ");
            }
            if (node->type == CONFIG_TYPE_ARRAY) {
                printf("@%s:%s\n", node->key, cur->value);
            } else {
                printf("%s:%s\n", cur->key, cur->value);
            }
        }
    }
}

void copy_output_node(config_node_t *node, lexer_output_t *output) {
    char *buf;
    size_t len;
    if (node->type == CONFIG_TYPE_ITEM) {
        len = output->key_len + output->value_len + 2;
    } else {
        len = output->key_len + 1;
    }
    buf = (char*) malloc(len);
    node->key = buf;
    memcpy(node->key, output->key, output->key_len);
    node->key_len = output->key_len;
    node->key[node->key_len] = 0;

    if (node->type == CONFIG_TYPE_ITEM) {
        node->value = buf + node->key_len + 1;
        memcpy(node->value, output->value, output->value_len);
        node->value_len = output->value_len;
        node->value[node->value_len] = 0;
    }

    node->lineno = output->lineno;
}

void config_node_destroy(config_node_t *node) {
    config_node_t *last, *tmp;
    last = node;
    while (node) {
        if (node->first_child) {
            last->next_sibling = node->first_child;
            last = node->last_child;
        }
        if (node->key) free(node->key);
        tmp = node->next_sibling;
        free(node);
        node = tmp;
    }
}

config_node_t* config_parse_file(const char *filename, char **errmsg) {
    lexer_t l;
    lexer_output_t output;
    config_node_t *root, *cur, *tmp, *parent, *stack_top;
    int cur_level, top_level, ret;

    stack_top = root = cur = new_config_node();
    cur_level = -1;
    top_level = -1;

    if (root == NULL) {
        errno = ENOMEM;
        asprintf(errmsg, "%s", strerror(errno));
        return NULL;
    }

    root->type = CONFIG_TYPE_GROUP;

    ret = lexer_init(&l);
    if (ret) {
        asprintf(errmsg, "%s", strerror(errno));
        free(root);
        return NULL;
    }
    if (lexer_setfile(l, filename)) {
        asprintf(errmsg, "%s", strerror(errno));
        goto error;
    }
	while ((ret = lexer_next(l, &output))) {
        if (ret == -1) {
            asprintf(errmsg, "Error in file %s:%d, syntax error", filename, output.lineno);
            goto error;
        }
        if (output.type == TOKEN_TYPE_ITEM) {
            tmp = find_child(cur, output.key, output.key_len);
            if (output.is_array) {
                if (tmp == NULL) {
                    tmp = new_config_node();
                    if (tmp == NULL) {
                        errno = ENOMEM;
                        asprintf(errmsg, "%s", strerror(errno));
                        goto error;
                    }
                    tmp->type = CONFIG_TYPE_ARRAY;
                    copy_output_node(tmp, &output);
                    add_child(cur, tmp);
                } else if (tmp->type != CONFIG_TYPE_ARRAY) {
                    asprintf(errmsg, "Error in file %s:%d, key '%s' already defined at line %d", filename, output.lineno, output.key, tmp->lineno);
                    goto error;
                }
                parent = tmp;
                output.key_len = 0;
            } else {
                if (tmp != NULL) {
                    asprintf(errmsg, "Error in file %s:%d, key '%s' already defined at line %d", filename, output.lineno, output.key, tmp->lineno);
                    goto error;
                }
                parent = cur;
            }
            /* TODO check for same name node existence */
            tmp = new_config_node();
            if (tmp == NULL) {
                errno = ENOMEM;
                asprintf(errmsg, "%s", strerror(errno));
                goto error;
            }
            tmp->type = CONFIG_TYPE_ITEM;
            copy_output_node(tmp, &output);
            add_child(parent, tmp);
        } else if (output.type == TOKEN_TYPE_GROUP) {
            if (output.level > top_level + 1) {
                asprintf(errmsg, "Error in file %s:%d, group level error", filename, output.lineno);
                goto error;
            }
            /* move cur to current level */
            cur = stack_top;
            cur_level = top_level;
            while (cur_level > output.level) {
                cur = cur->parent;
                if (cur->type == CONFIG_TYPE_ARRAY) {
                    cur = cur->parent;
                }
                cur_level--;
            }
            /* is it a different node? */
            if (
                    cur_level != output.level
                    || output.is_array
                    || cur->key_len != output.key_len
                    || memcmp(cur->key, output.key, cur->key_len))
            {
                if (cur_level == output.level) {
                    cur = cur->parent;
                    if (cur->type == CONFIG_TYPE_ARRAY) {
                        cur = cur->parent;
                    }
                }
                tmp = find_child(cur, output.key, output.key_len);
                if (output.is_array) {
                    if (tmp == NULL) {
                        /* create a group node first */
                        tmp = new_config_node();
                        if (tmp == NULL) {
                            errno = ENOMEM;
                            asprintf(errmsg, "%s", strerror(errno));
                            goto error;
                        }
                        tmp->type = CONFIG_TYPE_ARRAY;
                        copy_output_node(tmp, &output);
                        add_child(cur, tmp);
                    }
                    parent = tmp;
                    output.key_len = 0;
                } else {
                    parent = cur;
                }
                if (output.is_array || tmp == NULL) {
                    tmp = new_config_node();
                    if (tmp == NULL) {
                        errno = ENOMEM;
                        asprintf(errmsg, "%s", strerror(errno));
                        goto error;
                    }
                    tmp->type = CONFIG_TYPE_GROUP;
                    copy_output_node(tmp, &output);
                    add_child(parent, tmp);
                    top_level = output.level;
                    stack_top = cur = tmp;
                } else if (tmp->type != CONFIG_TYPE_GROUP) {
                    asprintf(errmsg, "Error in file %s:%d, key '%s' already defined at line %d", filename, output.lineno, output.key, tmp->lineno);
                    goto error;
                }
            }
        }
    }
    lexer_destroy(l);
    return root;

    error:
    if (root) config_node_destroy(root);
    lexer_destroy(l);
    return NULL;
}

int config_init(config_t **config) {
    *config = malloc(sizeof(config_t));
    (*config)->root = NULL;
    (*config)->errmsg = NULL;
    return 0;
}

int config_destroy(config_t *config) {
    if (config->root)
        config_node_destroy(config->root);
    if (config->errmsg)
        free(config->errmsg);
    free(config);
    return 0;
}

int config_read_file(config_t *config, const char *filename) {
    config_node_t *ret;
    if (config->root) {
        config_node_destroy(config->root);
    }
    ret = config_parse_file(filename, &config->errmsg);
    if (ret) {
        config->root = ret;
        return 0;
    }

    return -1;
}

const char* config_error(config_t *config) {
    return config->errmsg;
}

config_node_t* config_root(config_t *config) {
    return config->root;
}

config_node_t* config_lookup(config_t *config, const char *cpath) {
    char *path, *key, *r;
    config_node_t *node;
    int index;
    node = config->root;
    path = strdup(cpath);
    key = strtok_r(path, "/", &r);
    while (node && key) {
        if (node->type == CONFIG_TYPE_ARRAY) {
            index = atoi(key);
            node = node->first_child;
            for(; index > 0 && node; index--, node = node->next_sibling);
        } else if (node->type == CONFIG_TYPE_GROUP) {
            node = find_child(node, key, strlen(key));
        } else {
            node = NULL;
        }
        key = strtok_r(NULL, "/", &r);
    }
    free(path);
    return node;
}

const char* config_node_key(config_node_t *node) {
    return node->key;
}

size_t config_node_key_len(config_node_t *node) {
    return node->key_len;
}

const char* config_node_value(config_node_t *node) {
    return node->value;
}

size_t config_node_value_len(config_node_t *node) {
    return node->value_len;
}

int config_node_lineno(config_node_t *node) {
    return node->lineno;
}

int config_node_length(config_node_t *node) {
    return node->length;
}

int config_node_index(config_node_t *node) {
    return node->index;
}

config_node_t* config_node_parent(config_node_t *node) {
    return node->parent;
}

config_node_t* config_node_first_child(config_node_t *node) {
    return node->first_child;
}

config_node_t* config_next(config_node_t *node) {
    return node->next_sibling;
}

const char* config_node_get_string(config_node_t *node) {
    return node->value;
}

long config_node_get_int(config_node_t *node) {
    return strtol(node->value, NULL, 0);
}

double config_node_get_float(config_node_t *node) {
    return strtod(node->value, NULL);
}

int config_node_get_bool(config_node_t *node) {
    if (atol(node->value) != 0
            || strcasecmp(node->value, "y") == 0
            || strcasecmp(node->value, "yes") == 0
            || strcasecmp(node->value, "true") == 0
            || strcasecmp(node->value, "t") == 0)
        return 1;
    return 0;
}

int config_lookup_string(config_t *config, const char *path, const char **value) {
    config_node_t *node;
    node = config_lookup(config, path);
    if (node == NULL) return -1;
    *value = config_node_get_string(node);
    return 0;
}

int config_lookup_int(config_t *config, const char *path, long *value) {
    config_node_t *node;
    node = config_lookup(config, path);
    if (node == NULL) return -1;
    *value = config_node_get_int(node);
    return 0;
}

int config_lookup_float(config_t *config, const char *path, double *value) {
    config_node_t *node;
    node = config_lookup(config, path);
    if (node == NULL) return -1;
    *value = config_node_get_float(node);
    return 0;
}

int config_lookup_bool(config_t *config, const char *path, int *value) {
    config_node_t *node;
    node = config_lookup(config, path);
    if (node == NULL) return -1;
    *value = config_node_get_bool(node);
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
