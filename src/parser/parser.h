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
#ifndef __PARSER_H__
#define __PARSER_H__

#include "PikaPlatform.h"
#include "PikaObj.h"
#include "dataArgs.h"
#include "dataStack.h"
#include "ast.h"

#define PIKA_BLOCK_SPACE 4

typedef struct {
    Stack* stack;
    int depth;
} BlockState;

typedef struct Parser {
    Args line_buffs;
    Args gen_buffs;
    BlockState blk_state;
    int blk_depth_origin;
    char *(*ast_to_target)(struct Parser* self, AST* ast);
    bool is_gen_bytecode;
    ByteCodeFrame *bytecode_frame;
    uint8_t this_blk_depth;
    uint32_t label_pc;
} Parser;

typedef char* (*fn_parser_Lines2Target)(Parser* self, char* sPyLines);

char* parser_remove_comment(char* line);
Parser* parser_create(void);
AST* parser_line_to_ast(Parser* self, char* line);
char* parser_line2Target(Parser* self, char* line);
int parser_deinit(Parser* parser);
int parser_file2DocFile(Parser* self, char* sPyFile, char* sDocFile);
char* parser_ast2Asm(Parser* self, AST* ast);
char* parser_lines2Doc(Parser* self, char* sPyLines);
char* parser_file2Doc(Parser* self, char* filename);
char* pika_file2Asm(Args* outBuffs, char* filename);
char* pika_lines2Array(char* lines);
char* pika_line2Asm(Args* buffs_p, char* line, Stack* blockStack);

#endif