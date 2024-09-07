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
#include "cursor.h"
#include "token.h"

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
        token_stream = lexer_set_token(token_stream, TOKEN_LITERAL, symbol_buff);
        goto __exit;
    }

    /* symbol */
    token_stream = lexer_set_token(token_stream, TOKEN_SYMBOL, symbol_buff);

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
        *token_stream = lexer_set_token(*token_stream, TOKEN_DEVIDER, content);
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
                *token_stream = lexer_set_token(*token_stream, TOKEN_OPERATOR, content);
                *i = *i + op_len - 1;
                return true;
            }
        }

        /* single operators: +, -, *, ... */
        content[0] = c0;
        *token_stream = lexer_set_symbol(*token_stream, stmt, *i, symbol_start_index);
        *token_stream = lexer_set_token(*token_stream, TOKEN_OPERATOR, content);
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
        { "not",    TOKEN_OPERATOR },
        { "and",    TOKEN_OPERATOR },
        { "or",     TOKEN_OPERATOR },
        { "is",     TOKEN_OPERATOR },
        { "in",     TOKEN_OPERATOR },
        { "if",     TOKEN_KEYWORD  },
        { "as",     TOKEN_OPERATOR },
        { "for",    TOKEN_KEYWORD  },
        { "import", TOKEN_OPERATOR },
        { "@inh",   TOKEN_OPERATOR },
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

/* token_stream is devided by space */
/* a token is [TOKENTYPE|(CONTENT)] */
char* lexer_get_token_stream(Args* out_buff, char* stmt)
{
    /* init */
    Arg* token_stream = New_arg(NULL);
    token_stream = arg_setStr(token_stream, "", "");
    int32_t size = strGetSize(stmt);
    uint8_t bracket_depth = 0;
    char cn1, c0;
    int32_t symbol_start_index = -1;
    uint8_t in_quotes = 0;
    bool is_number = false;
    char* token_stream_str;

    /* process */
    for (int32_t i = 0; i < size; i++) {
        /* update char */
        cn1 = (i >= 1) ? stmt[i - 1] : 0;
        c0 = stmt[i];
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

StmtType lexer_match_stmt_type(char *right)
{
    Args buffs = {0};
    StmtType eStmtType = STMT_NONE;
    char* top_stmt = _remove_sub_stmt(&buffs, right);
    bool operator = false;
    bool method = false;
    bool string = false;
    bool bytes = false;
    bool number = false;
    bool symbol = false;
    bool list = false;
    bool slice = false;
    bool dict = false;
    bool import = false;
    bool inherit = false;
    bool chain = false;

    Cursor_forEach(cs, top_stmt) {
        Cursor_iterStart(&cs);
        /* collect type */
        if (strEqu(cs.token1.pyload, " import ")) {
            import = true;
            goto __iter_continue;
        }
        if (strEqu(cs.token1.pyload, "@inh ")) {
            inherit = true;
            goto __iter_continue;
        }
        if (strEqu(cs.token2.pyload, "[")) {
            /* (symble | iteral | <]> | <)>) + <[> */
            if (TOKEN_SYMBOL == cs.token1.type ||
                TOKEN_LITERAL == cs.token1.type ||
                strEqu(cs.token1.pyload, "]") ||
                strEqu(cs.token1.pyload, ")")) {
                /* keep the last one of the chain or slice */
                slice = true;
                chain = false;
                goto __iter_continue;
            }
            /* ( <,> | <=> ) + <[> */
            list = true;
        }
        if (strEqu(cs.token1.pyload, "[") && cs.iter_index == 1) {
            /* VOID + <[> */
            list = true;
            method = false;
            goto __iter_continue;
        }
        if (strEqu(cs.token1.pyload, "...")) {
            goto __iter_continue;
        }

        if (strEqu(cs.token1.pyload, "pass")) {
            goto __iter_continue;
        }

        if (strIsStartWith(cs.token1.pyload, ".")) {
            if (cs.iter_index != 1) {
                /* keep the last one of the chain or slice */
                chain = true;
                slice = false;
                goto __iter_continue;
            }
        }
        if (strEqu(cs.token1.pyload, "{")) {
            dict = true;
            goto __iter_continue;
        }
        if (cs.token1.type == TOKEN_OPERATOR) {
            operator = true;
            goto __iter_continue;
        }
        /* <(> */
        if (strEqu(cs.token1.pyload, "(")) {
            method = true;
            slice = false;
            goto __iter_continue;
        }
        if (cs.token1.type == TOKEN_LITERAL) {
            if (cs.token1.pyload[0] == '\'' || cs.token1.pyload[0] == '"') {
                string = true;
                goto __iter_continue;
            }
            if (cs.token1.pyload[1] == '\'' || cs.token1.pyload[1] == '"') {
                if (cs.token1.pyload[0] == 'b') {
                    bytes = true;
                    goto __iter_continue;
                }
            }
            number = true;
            goto __iter_continue;
        }
        if (cs.token1.type == TOKEN_SYMBOL) {
            symbol = true;
            goto __iter_continue;
        }
    __iter_continue:
        Cursor_iterEnd(&cs);
    }
    if (inherit) {
        eStmtType = STMT_INHERIT;
        goto __exit;
    }
    if (import) {
        eStmtType = STMT_IMPORT;
        goto __exit;
    }
    if (operator) {
        eStmtType = STMT_OPERATOR;
        goto __exit;
    }
    if (slice) {
        eStmtType = STMT_SLICE;
        goto __exit;
    }
    if (chain) {
        eStmtType = STMT_CHAIN;
        goto __exit;
    }
    if (list) {
        eStmtType = STMT_LIST;
        goto __exit;
    }
    if (dict) {
        eStmtType = STMT_DICT;
        goto __exit;
    }
    if (method) {
        eStmtType = STMT_METHOD;
        goto __exit;
    }
    if (string) {
        /* support multi assign */
        if (Cursor_isContain(right, TOKEN_DEVIDER, ",")) {
            eStmtType = STMT_TUPLE;
            goto __exit;
        }
        eStmtType = STMT_STRING;
        goto __exit;
    }
    if (bytes) {
        /* support multi assign */
        if (Cursor_isContain(right, TOKEN_DEVIDER, ",")) {
            eStmtType = STMT_TUPLE;
            goto __exit;
        }
        eStmtType = STMT_BYTES;
        goto __exit;
    }
    if (number) {
        /* support multi assign */
        if (Cursor_isContain(right, TOKEN_DEVIDER, ",")) {
            eStmtType = STMT_TUPLE;
            goto __exit;
        }
        eStmtType = STMT_NUMBER;
        goto __exit;
    }
    if (symbol) {
        /* support multi assign */
        if (Cursor_isContain(right, TOKEN_DEVIDER, ",")) {
            eStmtType = STMT_TUPLE;
            goto __exit;
        }
        eStmtType = STMT_REFERENCE;
        goto __exit;
    }
__exit:
    Cursor_deinit(&cs);
    strsDeinit(&buffs);
    return eStmtType;
}

// debug
char* lexer_print_token_stream(Args* out_buffs, char* token_stream)
{
    pika_assert(token_stream);
    /* init */
    Args buffs = {0};
    char* print_out = strsCopy(&buffs, "");

    /* process */
    uint16_t token_size = TokenStream_getSize(token_stream);
    for (uint16_t i = 0; i < token_size; i++) {
        char* sToken = TokenStream_pop(&buffs, &token_stream);
        if (sToken[0] == TOKEN_OPERATOR) {
            print_out = strsAppend(&buffs, print_out, "{opt}");
            print_out = strsAppend(&buffs, print_out, sToken + 1);
        }
        if (sToken[0] == TOKEN_DEVIDER) {
            print_out = strsAppend(&buffs, print_out, "{dvd}");
            print_out = strsAppend(&buffs, print_out, sToken + 1);
        }
        if (sToken[0] == TOKEN_SYMBOL) {
            print_out = strsAppend(&buffs, print_out, "{sym}");
            print_out = strsAppend(&buffs, print_out, sToken + 1);
        }
        if (sToken[0] == TOKEN_LITERAL) {
            print_out = strsAppend(&buffs, print_out, "{lit}");
            print_out = strsAppend(&buffs, print_out, sToken + 1);
        }
    }
    /* out put */
    print_out = strsCopy(out_buffs, print_out);
    strsDeinit(&buffs);
    return print_out;
}

static char* _solveEqualLevelOperator(Args* buffs,
                                      char* sOperator,
                                      char* sOp1,
                                      char* sOp2,
                                      char* sStmt) {
    if ((strEqu(sOperator, sOp1)) || (strEqu(sOperator, sOp2))) {
        Cursor_forEach(cs, sStmt) {
            Cursor_iterStart(&cs);
            if (strEqu(cs.token1.pyload, sOp1)) {
                sOperator = strsCopy(buffs, sOp1);
            }
            if (strEqu(cs.token1.pyload, sOp2)) {
                sOperator = strsCopy(buffs, sOp2);
            }
            Cursor_iterEnd(&cs);
        }
        Cursor_deinit(&cs);
    }
    return sOperator;
}

static const char operators[][9] = {
    "**",  "~",    "*",     "/",     "%",    "//",      "+",  "-",  ">>",
    "<<",  "&",    "^",     "|",     "<",    "<=",      ">",  ">=", "!=",
    "==",  " is ", " in ",  "%=",    "/=",   "//=",     "-=", "+=", "*=",
    "**=", "^=",   " not ", " and ", " or ", " import "};

char* Lexer_getOperator(Args* outBuffs, char* sStmt) {
    Args buffs = {0};
    char* sOperator = NULL;

    // use parse state foreach to get operator
    for (uint32_t i = 0; i < sizeof(operators) / 9; i++) {
        Cursor_forEach(cs, sStmt) {
            Cursor_iterStart(&cs);
            // get operator
            if (strEqu(cs.token1.pyload, (char*)operators[i])) {
                // solve the iuuse of "~-1"
                sOperator = strsCopy(&buffs, (char*)operators[i]);
                Cursor_iterEnd(&cs);
                break;
            }
            Cursor_iterEnd(&cs);
        };
        Cursor_deinit(&cs);
    }

    /* solve the iuuse of "~-1" */
    if (strEqu(sOperator, "-")) {
        Cursor_forEach(cs, sStmt) {
            Cursor_iterStart(&cs);
            if (strEqu(cs.token2.pyload, "-")) {
                if (cs.token1.type == TOKEN_OPERATOR) {
                    sOperator = strsCopy(&buffs, cs.token1.pyload);
                    Cursor_iterEnd(&cs);
                    break;
                }
            }
            Cursor_iterEnd(&cs);
        };
        Cursor_deinit(&cs);
    }

    /* match the last operator in equal level */
    sOperator = _solveEqualLevelOperator(&buffs, sOperator, "+", "-", sStmt);
    sOperator = _solveEqualLevelOperator(&buffs, sOperator, "*", "/", sStmt);
    /* out put */
    if (NULL == sOperator) {
        return NULL;
    }
    sOperator = strsCopy(outBuffs, sOperator);
    strsDeinit(&buffs);
    return sOperator;
}

Arg* arg_strAddIndent(Arg* aStrIn, int indent) {
    if (0 == indent) {
        return aStrIn;
    }
    /* add space */
    char* sSpaces = pikaMalloc(indent + 1);
    pika_platform_memset(sSpaces, ' ', indent);
    sSpaces[indent] = '\0';
    Arg* aRet = arg_newStr(sSpaces);
    aRet = arg_strAppend(aRet, arg_getStr(aStrIn));
    pikaFree(sSpaces, indent + 1);
    arg_deinit(aStrIn);
    return aRet;
}

Arg* arg_strAddIndentMulti(Arg* aStrInMuti, int indent) {
    if (0 == indent) {
        return aStrInMuti;
    }
    char* sStrInMuti = arg_getStr(aStrInMuti);
    char* sLine = NULL;
    int iLineNum = strGetLineNum(sStrInMuti);
    Arg* aStrOut = arg_newStr("");
    Args buffs = {0};
    for (int i = 0; i < iLineNum; i++) {
        sLine = strsPopLine(&buffs, &sStrInMuti);
        Arg* aLine = arg_newStr(sLine);
        aLine = arg_strAddIndent(aLine, indent);
        sLine = arg_getStr(aLine);
        aStrOut = arg_strAppend(aStrOut, sLine);
        if (i != iLineNum - 1) {
            aStrOut = arg_strAppend(aStrOut, "\n");
        }
        arg_deinit(aLine);
    }
    strsDeinit(&buffs);
    arg_deinit(aStrInMuti);
    return aStrOut;
}