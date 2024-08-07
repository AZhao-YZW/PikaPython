/*
 * MIT License
 *
 * Copyright (c) 2024 azhao 余钊炜 yuzhaowei2002@outlook.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "lexer.h"
#include "PikaPlatform.h"
#include "PikaParser.h"

static Arg* lexer_set_token(Arg* token_stream, enum TokenType type, char* op_str)
{
    Args buffs = {0};
    char token_type_buff[3] = {0};
    char* token_stream_str = arg_getStr(token_stream);
    Arg* new_token_stream;

    token_type_buff[0] = 0x1F;
    token_type_buff[1] = type;
    token_stream_str = strsAppend(&buffs, token_stream_str, token_type_buff);
    token_stream_str = strsAppend(&buffs, token_stream_str, op_str);
    new_token_stream = arg_newStr(token_stream_str);
    arg_deinit(token_stream);
    strsDeinit(&buffs);
    return new_token_stream;
}

static Arg* lexer_set_symbol(Arg* token_stream, char* stmt, int32_t i, int32_t* symbol_start_index)
{
    Args buffs = {0};
    char* symbol_buff = NULL;

    if (-1 == *symbol_start_index) {
        /* no found symbol start index */
        goto __exit;
    }
    /* nothing to add symbel */
    if (i == *symbol_start_index) {
        goto __exit;
    }
    symbol_buff = args_getBuff(&buffs, i - *symbol_start_index);
    pika_platform_memcpy(symbol_buff, stmt + *symbol_start_index, i - *symbol_start_index);
    /* literal */
    if ((symbol_buff[0] == '\'' || symbol_buff[0] == '"') ||    // "" or ''
        (symbol_buff[0] >= '0' && symbol_buff[0] <= '9') ||     // number
        (symbol_buff[0] == 'b' && (symbol_buff[1] == '\'' || symbol_buff[1] == '"'))) { // b"" or b''
        token_stream = lexer_set_token(token_stream, TOKEN_literal, symbol_buff);
        goto __exit;
    }

    /* symbol */
    token_stream = lexer_set_token(token_stream, TOKEN_symbol, symbol_buff);

__exit:
    *symbol_start_index = -1;
    strsDeinit(&buffs);
    return token_stream;
}

// return: is continue or not
static bool lexer_solve_string(Arg** token_stream, uint8_t *in_quotes, char* stmt, int32_t i,
                               int32_t *symbol_start_index)
{
    char cn1 = (i >= 1) ? stmt[i - 1] : 0;
    char c0 = stmt[i];

    if (0 == *in_quotes) {
        /* in '' or "" */
        if (('\'' == c0 || '"' == c0) && '\\' != cn1) {
            *in_quotes = ('\'' == c0) ? 1 : 2;
            return true;
        }
        return false;
    } else {
        /* out '' or "" */
        if (((1 == *in_quotes && '\'' == c0) || (2 == *in_quotes && '"' == c0)) && '\\' != cn1) {
            *in_quotes = 0;
            *token_stream = lexer_set_symbol(*token_stream, stmt, i + 1, symbol_start_index);
        }
        return true;
    }
}

// return: is match charactor in list or not
static bool lexer_match_char(char * list, int size, char c)
{
    int i;
    for (i = 0; i < size; i++) {
        if (c == list[i]) {
            return true;
        }
    }
    return false;
}

// return: is continue or not
static bool lexer_match_devider(Arg** token_stream, char* stmt, int32_t i, uint8_t *bracket_depth,
                                int32_t *symbol_start_index)
{
    char c0 = stmt[i];
    char content[2] = {0};
    char support_devider[] = {
        '(', ')', ',', '[', ']', ':', '{', '}', ';'
    };

    if (lexer_match_char(support_devider, sizeof(support_devider), c0)) {
        content[0] = c0;
        *token_stream = lexer_set_symbol(*token_stream, stmt, i, symbol_start_index);
        *token_stream = lexer_set_token(*token_stream, TOKEN_devider, content);
        if (c0 == '(') {
            (*bracket_depth)++;
        }
        if (c0 == ')') {
            (*bracket_depth)--;
        }
        return true;
    }
    return false;
}

// return: is continue or not
static bool lexer_match_operater(Arg** token_stream, char* stmt, int32_t *i, int32_t size,
                                 int32_t *symbol_start_index, bool is_number)
{
    char cn1 = (*i >= 1) ? stmt[*i - 1] : 0;
    char c0 = stmt[*i];
    char c1 = (*i + 1 < size) ? stmt[*i + 1] : 0;
    char c2 = (*i + 2 < size) ? stmt[*i + 2] : 0;
    char content[4] = {0};
    char first_char[] = {
        '>', '<', '*', '/', '+', '-', '!', '=', '%', '&', '|', '^', '~'
    };
    char *multi_ops[] = {
        ">=", "<=", "*=", "/=", "+=", "-=", "!=", "==", "%=", "|=", "^=", "&=",
        "**=", "//=", "<<=", ">>=", "**", "//", "<<", ">>",
    };
    char *op;
    int op_index;
    uint8_t op_len;

    if (lexer_match_char(first_char, sizeof(first_char), c0)) {
        if ('-' == c0 && is_number) {
            if ((cn1 == 'e') || (cn1 == 'E')) {
                return true;
            }
        }
        /* multi operaters */
        for (op_index = 0; op_index < sizeof(multi_ops) / sizeof(multi_ops[0]); op_index++) {
            op = multi_ops[op_index];
            op_len = strlen(op);
            if ((op_len == 2 && c0 == op[0] && c1 == op[1]) ||
                (op_len == 3 && c0 == op[0] && c1 == op[1] && c2 == op[2])) {
                content[0] = c0;
                content[1] = c1;
                content[2] = (op_len == 3) ? c2 : 0;
                *token_stream = lexer_set_symbol(*token_stream, stmt, *i, symbol_start_index);
                *token_stream = lexer_set_token(*token_stream, TOKEN_operator, content);
                *i = *i + op_len - 1;
                return true;
            }
        }

        /* single operators: +, -, *, ... */
        content[0] = c0;
        *token_stream = lexer_set_symbol(*token_stream, stmt, *i, symbol_start_index);
        *token_stream = lexer_set_token(*token_stream, TOKEN_operator, content);
        return true;
    }

    return false;
}

// return: is continue or not
static bool lexer_match_key_word(Arg** token_stream, char* stmt, int32_t *i, int32_t size,
                                 int32_t *symbol_start_index)
{
    static struct {
        char *key_word;
        enum TokenType token_type;
    } kw_map[] = {
        { "not",    TOKEN_operator },
        { "and",    TOKEN_operator },
        { "or",     TOKEN_operator },
        { "is",     TOKEN_operator },
        { "in",     TOKEN_operator },
        { "if",     TOKEN_keyword  },
        { "as",     TOKEN_operator },
        { "for",    TOKEN_keyword  },
        { "import", TOKEN_operator },
        { "@inh",   TOKEN_operator },
    };
    char kw_str_buff[sizeof(" import ")] = {0};
    bool is_match = false;
    int c, kw_index;
    uint8_t kw_len;

    for (kw_index = 0; kw_index < sizeof(kw_map) / sizeof(kw_map[0]); kw_index++) {
        kw_len = strlen(kw_map[kw_index].key_word);
        if (size - *i - 1 < kw_len) {    // size include ' '
            continue;
        }
        for (c = 0; c < kw_len; c++) {
            if ((stmt + *i)[c] != kw_map[kw_index].key_word[c]) {
                break;
            }
        }
        if (c == kw_len && (stmt + *i)[c] == ' ') {
            is_match = true;
            break;
        }
    }

    if (is_match) {
        *token_stream = lexer_set_symbol(*token_stream, stmt, *i, symbol_start_index);
        kw_str_buff[0] = ' ';
        pika_platform_memcpy(kw_str_buff + 1, kw_map[kw_index].key_word, kw_len);
        kw_str_buff[kw_len + 1] = ' ';
        *token_stream = lexer_set_token(*token_stream, kw_map[kw_index].token_type, kw_str_buff);
        *i = *i + kw_len;
        return true;
    }

    return false;
}

/* tokenStream is devided by space */
/* a token is [TOKENTYPE|(CONTENT)] */
char* lexer_get_token_stream(Args* out_buff, char* stmt)
{
    /* init */
    Arg* token_stream = New_arg(NULL);
    token_stream = arg_setStr(token_stream, "", "");
    int32_t size = strGetSize(stmt);
    uint8_t bracket_depth = 0;
    char cn2, cn1, c0, c1, c2, c3, c4, c5, c6;
    int32_t symbol_start_index = -1;
    uint8_t in_quotes = 0;
    bool is_number = false;
    char* token_stream_str;

    /* process */
    for (int32_t i = 0; i < size; i++) {
        /* update char */
        cn2 = (i >= 2) ? stmt[i - 2] : 0;
        cn1 = (i >= 1) ? stmt[i - 1] : 0;
        c0 = stmt[i];
        c1 = (i + 1 < size) ? stmt[i + 1] : 0;
        c2 = (i + 2 < size) ? stmt[i + 2] : 0;
        c3 = (i + 3 < size) ? stmt[i + 3] : 0;
        c4 = (i + 4 < size) ? stmt[i + 4] : 0;
        c5 = (i + 5 < size) ? stmt[i + 5] : 0;
        c6 = (i + 6 < size) ? stmt[i + 6] : 0;
        if (-1 == symbol_start_index) {
            is_number = ((c0 >= '0') && (c0 <= '9'));
            symbol_start_index = i;
        }

        /* solve string */
        if (lexer_solve_string(&token_stream, &in_quotes, stmt, i, &symbol_start_index)) {
            continue;
        }

        /* match annotation */
        if ('#' == c0) {
            break;
        }

        /* match devider*/
        if (lexer_match_devider(&token_stream, stmt, i, &bracket_depth, &symbol_start_index)) {
            continue;
        }
        
        /* match operator */
        if (lexer_match_operater(&token_stream, stmt, &i, size, &symbol_start_index, is_number)) {
            continue;
        }

        // not the string operator
        if ((cn1 >= 'a' && cn1 <= 'z') || (cn1 >= 'A' && cn1 <= 'Z') ||
            (cn1 >= '0' && cn1 <= '9') || cn1 == '_' || cn1 == '.') {
            goto __after_match_string_operator;
        }

        if (lexer_match_key_word(&token_stream, stmt, &i, size, &symbol_start_index)) {
            continue;
        }

    __after_match_string_operator:
        /* skip spaces */
        if (' ' == c0) {
            /* not get symbal */
            if (i == symbol_start_index) {
                symbol_start_index = -1;
            } else {
                /* already get symbal */
                token_stream = lexer_set_symbol(token_stream, stmt, i, &symbol_start_index);
            }
        }
        if (i == size - 1) {
            /* last check symbel */
            token_stream = lexer_set_symbol(token_stream, stmt, size, &symbol_start_index);
        }
    }
    /* output */
    token_stream_str = arg_getStr(token_stream);
    token_stream_str = strsCopy(out_buff, token_stream_str);
    arg_deinit(token_stream);
    return token_stream_str;
}