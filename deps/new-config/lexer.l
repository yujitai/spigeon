%{
#include <stdio.h>
#include "config_lexer.h"
typedef struct _yyuser_data_t {
    int group_indent;
    int is_array;
    char *lbuf;
    char sbuf[64];
    int len;
    int lbuf_len;
    int i;
} yyuser_data_t;
#define YY_DECL int yylex (yyscan_t yyscanner, lexer_output_t *output)
int parse_quote(char *buf, int len) {
    int cur = 0;
    int i;
    for (i = 0; i < len; cur++, i++) {
        if (buf[i] == '\\') {
            if (i+1 < len) {
                i++;
                switch(buf[i]) {
                    case 'n':
                        buf[cur] = '\n'; break;
                    case 'r':
                        buf[cur] = '\r'; break;
                    case 't':
                        buf[cur] = '\t'; break;
                    case 'v':
                        buf[cur] = '\v'; break;
                    case 'a':
                        buf[cur] = '\x07'; break;
                    case 'b':
                        buf[cur] = '\x08'; break;
                    case 'f':
                        buf[cur] = '\x0c'; break;
                    case '\'':
                        buf[cur] = '\''; break;
                    case '"':
                        buf[cur] = '"'; break;
                    case '\\':
                        buf[cur] = '\\'; break;
                    case 'x':
                        if (i+2 < len
                                && ((buf[i+1] >= '0' && buf[i+1] <= '9')
                                 || (buf[i+1] >= 'a' && buf[i+1] <= 'f')
                                 || (buf[i+1] >= 'A' && buf[i+1] <= 'F'))
                                && ((buf[i+2] >= '0' && buf[i+2] <= '9')
                                 || (buf[i+2] >= 'a' && buf[i+2] <= 'f')
                                 || (buf[i+2] >= 'A' && buf[i+2] <= 'F')))
                        {
                        #define ___hval(x)          \
                            (((x)<='9')?            \
                                (x) - '0' :         \
                                (((x)>='a')?        \
                                    (x) - 'a' + 10 :\
                                    (x) - 'A' + 10))
                            buf[cur] =
                                (___hval(buf[i+1])<<4) |
                                ___hval(buf[i+2]);
                            i += 2;
                        } else {
                            buf[cur] = buf[i];
                        }
                        break;
                    case '0':
                    case '1':
                    case '2':
                        if (i+2 < len
                                && (buf[i+1] >= '0' && buf[i+1] <= '7')
                                && (buf[i+2] >= '0' && buf[i+2] <= '7'))
                        {
                        #define ___oval(x) ((x) - '0')
                            buf[cur] =
                                (___oval(buf[i]) << 6) | 
                                (___oval(buf[i+1]) << 3) | 
                                ___oval(buf[i]);
                            i += 2;
                        } else if (buf[i] == '0') {
                            buf[cur] = '\0';
                        } else {
                            buf[cur] = buf[i];
                        }
                        break;
                    default:
                        buf[cur] = buf[i];
                }
            }
        } else {
            buf[cur] = buf[i];
        }
    }
    buf[cur] = '\0';
    return cur;
}
%}

%option noyywrap fast reentrant yylineno 8bit
%option extra-type="yyuser_data_t*"
%option outfile="lexer.c"
%x VALUE
%x AFTER_KEY
%x QUOTED_VALUE
%x COMMENT
%x GROUP
%x GROUP_FINISH

key [a-zA-Z0-9_]+
pure_value [^"#\n\t ][^#\n]*
quoted_value ([^"\\\n]|(\\[\x00-\xff])|(\\x[0-9a-fA-F][0-9a-fA-F])|(\\[0-3][0-7][0-7]))*
ANY [\x00-\xff]

%%

"[" { yyextra->is_array = 0; yyextra->group_indent = 0; BEGIN(GROUP); }
"@" { yyextra->is_array = 1; }
"#" { BEGIN(COMMENT); }

{key} {
    if ((size_t) yyleng < sizeof(yyextra->sbuf)) {
        /* copy to small buf */
        memcpy(yyextra->sbuf, yytext, yyleng);
        yyextra->sbuf[yyleng] = 0;
        output->key = yyextra->sbuf;
    } else {
        if (yyleng >= yyextra->lbuf_len) {
            yyextra->lbuf_len = yyleng + 1;
            if (yyextra->lbuf) free(yyextra->lbuf);
            yyextra->lbuf = (char*) malloc(yyextra->lbuf_len);
        }
        /* copy to large buf */
        memcpy(yyextra->lbuf, yytext, yyleng);
        yyextra->lbuf[yyleng] = 0;
        output->key = yyextra->lbuf;
    }
    output->key_len = yyleng;
    output->is_array = yyextra->is_array;
    BEGIN(AFTER_KEY);
}

<AFTER_KEY>[ \t] ;
<AFTER_KEY>":" { yyextra->is_array = 0; BEGIN(VALUE); }
<AFTER_KEY>\n { BEGIN(INITIAL); output->lineno = yylineno; return -1; }

<GROUP,GROUP_FINISH>"]" { yyextra->is_array = 0; BEGIN(INITIAL); }
<GROUP>"." { yyextra->group_indent++; }
<GROUP>"@" { yyextra->is_array = 1; }
<GROUP>{key} {
    output->key = yytext;
    output->key_len = yyleng;
    output->type = TOKEN_TYPE_GROUP;
    output->level = yyextra->group_indent;
    output->is_array = yyextra->is_array;
    output->lineno = yylineno;
    if (output->is_array) {
        BEGIN(GROUP_FINISH);
    }
    return 1;
}

<COMMENT>\n { BEGIN(INITIAL); }
<COMMENT>{ANY} ;

<VALUE>{pure_value} {
    output->value = yytext;
    output->value_len = yyleng;
    output->type = TOKEN_TYPE_ITEM;
    output->lineno = yylineno;
    BEGIN(COMMENT);
    return 2;
}
<VALUE>"\"" { BEGIN(QUOTED_VALUE); }
<VALUE>[\n] {
    output->value = NULL;
    output->value_len = 0;
    output->type = TOKEN_TYPE_ITEM;
    output->lineno = yylineno;
    BEGIN(INITIAL);
    return 2;
}
<VALUE>"#" { BEGIN(COMMENT); }
<VALUE>[ \t] ;

<QUOTED_VALUE>{quoted_value} {
    /* convert quoted value */
    output->value = yytext;
    output->value_len = parse_quote(yytext, yyleng);
    output->type = TOKEN_TYPE_ITEM;
    output->lineno = yylineno;
    BEGIN(COMMENT);
    return 2;
}

[ \t\r\n] ;
<INITIAL,QUOTED_VALUE,GROUP,AFTER_KEY>{ANY} {
    output->lineno = yylineno;
    return -1;
}

%%

typedef struct lexer_internal {
    yyscan_t s;
    yyuser_data_t extra;
} lexer_internal_t;

int lexer_init(lexer_t *l) {
    lexer_internal_t *internal;
    int ret;

    if (l == NULL) {
        errno = EINVAL;
        return 1;
    }

    internal = (lexer_internal_t*) yyalloc(sizeof(lexer_internal_t), NULL);

    if (internal == NULL) {
        errno = ENOMEM;
        return 1;
    }

    memset(&internal->extra, 0, sizeof(internal->extra));

    ret = yylex_init_extra(&internal->extra, &internal->s);
    if (ret) {
        yylex_destroy(internal->s);
        yyfree(internal, NULL);
        return ret;
    }

    *l = (lexer_t) internal;

    return 0;
}

int lexer_setfile(lexer_t l, const char *filename) {
    FILE *f;
    lexer_internal_t *internal = (lexer_internal_t*) l;
    f = fopen(filename, "r");
    yyset_in(f, internal->s);
    if (f == NULL) {
        return -1;
    }
    return 0;
}

int lexer_destroy(lexer_t l) {
    lexer_internal_t *internal = (lexer_internal_t*) l;
    yylex_destroy(internal->s);
    yyfree(internal, NULL);
    return 0;
}

int lexer_next(lexer_t l, lexer_output_t *output) {
    lexer_internal_t *internal = (lexer_internal_t*) l;
    return yylex(internal->s, output);
}
