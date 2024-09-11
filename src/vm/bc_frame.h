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
#ifndef __BC_FRAME_H__
#define __BC_FRAME_H__

#include "dataArgs.h"
#include "PikaObj.h"

typedef struct Asmer {
    char *asm_code;
    uint8_t block_deepth_now;
    uint8_t is_new_line;
    char *line_pointer;
} Asmer;

void bc_frame_load_bytecode(ByteCodeFrame *self, uint8_t* bytes,
                                    char *name, pika_bool is_const);
ByteCodeFrame *_cache_bcf_fn(PikaObj *self, char *py_lines);
ByteCodeFrame *_cache_bcf_fn_bc(PikaObj *self, uint8_t* bytecode);
Hash bc_frame_get_name_hash(ByteCodeFrame *bcframe);
void bc_frame_set_name(ByteCodeFrame *self, char *name);
size_t bc_frame_get_size(ByteCodeFrame *bf);

#endif