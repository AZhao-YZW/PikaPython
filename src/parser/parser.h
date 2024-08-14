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

#define _VAL_NEED_INIT -1

typedef PikaObj AST_S;

typedef struct {
    Stack* stack;
    int depth;
} BlockState_S;

typedef struct Parser_S {
    Args line_buffs;
    Args gen_buffs;
    BlockState_S blk_state;
    int blk_depth_origin;
    char *(*ast_to_target)(struct Parser_S* self, AST_S* ast);
    bool is_gen_bytecode;
    ByteCodeFrame* bytecode_frame;
    uint8_t this_blk_depth;
    uint32_t label_pc;
} Parser_S;

char* parser_remove_comment(char* line);
Parser_S* parser_create(void);

#endif