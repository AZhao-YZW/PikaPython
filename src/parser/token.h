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
#ifndef __TOKEN_H__
#define __TOKEN_H__

#include "dataArgs.h"

typedef enum TokenType {
    TOKEN_STREND = 0,
    TOKEN_SYMBOL,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_DEVIDER,
    TOKEN_LITERAL,
} TokenType;

typedef struct LexToken {
    char* tokenStream;
    enum TokenType type;
    char* pyload;
} LexToken;

uint8_t Token_isBranketStart(LexToken* token);
char* TokenStream_pop(Args* buffs, char** sTokenStream_pp);
uint16_t TokenStream_getSize(char* tokenStream);
void LexToken_update(struct LexToken* lex_token);
void LexToken_init(struct LexToken* lt);

#endif