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
#ifndef __CURSOR_H__
#define __CURSOR_H__

#include "lexer.h"
#include "token.h"
#include "dataArgs.h"

struct Cursor {
    char* tokenStream;
    uint16_t length;
    uint16_t iter_index;
    int8_t bracket_deepth;
    struct LexToken token1;
    struct LexToken token2;
    Arg* last_token;
    Args* iter_buffs;
    Args* buffs_p;
    PIKA_RES result;
};

char* Cursor_popLastToken(Args* outBuffs, char** pStmt, char* str);
char* Cursor_getCleanStmt(Args* outBuffs, char* cmd);
void _Cursor_init(struct Cursor* cs);
void _Cursor_parse(struct Cursor* cs, char* stmt);
void _Cursor_beforeIter(struct Cursor* cs);
uint8_t Cursor_count(char* stmt, TokenType type, char* pyload);
uint8_t _Cursor_count(char* stmt, TokenType type, char* pyload, pika_bool bSkipbracket);
char* Cursor_splitCollect(Args* buffs, char* sStmt, char* sDevide, int index);
pika_bool Cursor_isContain(char* sStmt, TokenType eType, char* sPyload);
char* Cursor_popToken(Args* buffs, char** sStmt_p, char* sDevide);
void Cursor_iterStart(struct Cursor* cs);
void Cursor_iterEnd(struct Cursor* cs);
void Cursor_deinit(struct Cursor* cs);
char* Cursor_removeTokensBetween(Args* outBuffs, char* input, char* token_pyload1, char* token_pyload2);

#define _Cursor_forEach(cursor)  \
    _Cursor_beforeIter(&cursor); \
    for (int __i = 0; __i < cursor.length; __i++)

#define Cursor_forEachExistPs(cursor, stmt) \
    /* init parserStage */                  \
    _Cursor_init(&cursor);                  \
    _Cursor_parse(&cursor, stmt);           \
    _Cursor_forEach(cursor)

#define Cursor_forEach(cursor, stmt) \
    struct Cursor cursor;            \
    Cursor_forEachExistPs(cursor, stmt)

char* _remove_sub_stmt(Args* outBuffs, char* sStmt);
char* Suger_format(Args* outBuffs, char* sRight);
char* Suger_leftSlice(Args* outBuffs, char* sRight, char** sLeft_p);
char* Suger_not_in(Args* out_buffs, char* sLine);
char* Suger_is_not(Args* out_buffs, char* sLine);
char* Suger_multiReturn(Args* out_buffs, char* sLine);
uint8_t Suger_selfOperator(Args* outbuffs, char* sStmt, char** sRight_p, char** sLeft_p);
pika_bool _check_is_multi_assign(char* sArgList);

#endif