/*
 * This file is part of the PikaPython project.
 * http://github.com/pikastech/pikapython
 *
 * MIT License
 *
 * Copyright (c) 2021 lyon liang6516@outlook.com
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
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __PIKA_PARSER__H
#define __PIKA_PARSER__H

#include "PikaVM.h"
#include "dataQueueObj.h"
#include "dataStack.h"

// ===== parser.h start =====
typedef struct Parser Parser;
typedef PikaObj AST;
// ===== parser.h  end  =====

typedef enum TokenType {
    TOKEN_strEnd = 0,
    TOKEN_symbol,
    TOKEN_keyword,
    TOKEN_operator,
    TOKEN_devider,
    TOKEN_literal,
} TokenType;

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

typedef struct Asmer Asmer;
struct Asmer {
    char* asm_code;
    uint8_t block_deepth_now;
    uint8_t is_new_line;
    char* line_pointer;
};

typedef enum _GenRuleValType {
    VAL_NONEVAL,
    VAL_DYNAMIC,
    VAL_STATIC_,
} GenRuleValType;

typedef struct GenRule {
    char* ins;
    GenRuleValType type;
    char* ast;
    char* val;
} GenRule;

typedef char* (*fn_parser_Lines2Target)(Parser* self, char* sPyLines);

#define PIKA_BLOCK_SPACE 4

typedef struct LexToken LexToken;
struct LexToken {
    char* tokenStream;
    enum TokenType type;
    char* pyload;
};

typedef struct Cursor ParsetState;
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

char* Lexer_printTokenStream(Args* outBuffs, char* tokenStream);

char* pika_file2Asm(Args* outBuffs, char* filename);
char* pika_lines2Asm(Args* outBuffs, char* multiLine);
char* pika_lines2Array(char* lines);
char* pika_line2Asm(Args* buffs_p, char* line, Stack* blockStack);
AST* parser_line_to_ast(Parser* self, char* line);
char* parser_file2Doc(Parser* self, char* sPyFile);
int parser_file2DocFile(Parser* self, char* sPyFile, char* sDocFile);
char* parser_ast2Asm(Parser* self, AST* ast);
char* parser_lines2Doc(Parser* self, char* sPyLines);
char* parser_file2Doc(Parser* self, char* filename);
AST* line2Ast(char* line);

PIKA_RES pika_lines2Bytes(ByteCodeFrame* bf, char* py_lines);
char* parser_line2Target(Parser* self, char* line);

int parser_deinit(Parser* parser);

char* Cursor_popLastToken(Args* outBuffs, char** pStmt, char* str);
char* Cursor_getCleanStmt(Args* outBuffs, char* cmd);
void _Cursor_init(struct Cursor* cs);
void _Cursor_parse(struct Cursor* cs, char* stmt);
void _Cursor_beforeIter(struct Cursor* cs);
uint8_t Cursor_count(char* stmt, TokenType type, char* pyload);
uint8_t _Cursor_count(char* stmt,
                      TokenType type,
                      char* pyload,
                      pika_bool bSkipbracket);

char* AST_genAsm_top(AST* oAST, Args* outBuffs);
int32_t ast_deinit(AST* ast);

char* instructUnit_fromAsmLine(Args* outBuffs, char* pikaAsm);
ByteCodeFrame* byteCodeFrame_appendFromAsm(ByteCodeFrame* bf, char* pikaAsm);

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

uint16_t TokenStream_getSize(char* tokenStream);
Arg* arg_strAddIndent(Arg* aStrIn, int indent);
Arg* arg_strAddIndentMuti(Arg* aStrIn, int indent);

#endif

#ifdef __cplusplus
}
#endif
