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
#ifndef __LEXER_H__
#define __LEXER_H__

#include "dataArgs.h"

typedef enum StmtType {
    STMT_REFERENCE,
    STMT_TUPLE,
    STMT_STRING,
    STMT_BYTES,
    STMT_NUMBER,
    STMT_METHOD,
    STMT_CHAIN,
    STMT_OPERATOR,
    STMT_INHERIT,
    STMT_IMPORT,
    STMT_LIST,
    STMT_SLICE,
    STMT_DICT,
    STMT_NONE
} StmtType;

char* lexer_get_token_stream(Args *out_buff, char *stmt);
StmtType lexer_match_stmt_type(char *right);
char* Lexer_getOperator(Args* outBuffs, char* sStmt);
Arg* arg_strAddIndentMulti(Arg* aStrInMuti, int indent);

// debug
char* lexer_print_token_stream(Args* out_buffs, char *token_stream);

#endif