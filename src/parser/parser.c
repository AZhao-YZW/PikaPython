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
#include "parser.h"
#include "PikaParser.h"
#include "dataString.h"

char* parser_remove_comment(char* line)
{
    bool is_comment_exit = false;
    bool is_in_single_quotes = false;
    bool is_in_double_quotes = false;
    bool is_backslash = false;

    for (uint32_t i = 0; i < strGetSize(line); i++) {
        // ignore '\'
        if (is_backslash) {
            is_backslash = false;
            continue;
        }

        if ('\\' == line[i]) {
            is_backslash = true;
            continue;
        }

        // ignore the '#' in quotes
        if ('\'' == line[i] && !is_in_double_quotes) {
            is_in_single_quotes = !is_in_single_quotes;
            continue;
        }
        if ('"' == line[i] && !is_in_single_quotes) {
            is_in_double_quotes = !is_in_double_quotes;
            continue;
        }
        if (is_in_single_quotes || is_in_double_quotes) {
            continue;
        }

        // remove contents after '#'
        if ('#' == line[i]) {
            line[i] = 0;
            is_comment_exit = true;
            break;
        }
    }

    if (!is_comment_exit) {
        return line;
    }

    for (uint32_t i = 0; i < strGetSize(line); i++) {
        if (' ' != line[i]) {
            return line;
        }
    }

    line = "@annotation";
    return line;
}

extern char* parser_ast2Asm(Parser* self, AST_S* ast);

Parser_S* parser_create(void)
{
    Parser_S* self = (Parser_S*)pikaMalloc(sizeof(Parser_S));
    pika_platform_memset(self, 0, sizeof(Parser_S));
    self->blk_state.stack = pikaMalloc(sizeof(Stack));
    /* generate asm as default */
    self->ast_to_target = parser_ast2Asm;
    pika_platform_memset(self->blk_state.stack, 0, sizeof(Stack));
    stack_init(self->blk_state.stack);
    /* -1 means not inited */
    self->blk_depth_origin = _VAL_NEED_INIT;
    return self;
}