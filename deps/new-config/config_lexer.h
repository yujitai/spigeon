/**
 * @file config_lexer.h
 * @author liuqingjun(com@baidu.com)
 * @date 2013/01/05 11:14:29
 * @brief 
 *  
 **/


#ifndef  __CONFIG_LEXER_H_
#define  __CONFIG_LEXER_H_

typedef void* lexer_t;

typedef struct _lexer_output_t {
    enum { TOKEN_TYPE_GROUP, TOKEN_TYPE_ITEM } type;
    int is_array;
    int level;
    char *key;
    size_t key_len;
    char *value;
    size_t value_len;
    int lineno;
} lexer_output_t;

int lexer_init(lexer_t *l);
int lexer_setfile(lexer_t l, const char *filename);
int lexer_destroy(lexer_t l);
int lexer_next(lexer_t l, lexer_output_t *output);

#endif

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
