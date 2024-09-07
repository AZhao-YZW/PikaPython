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
#ifndef __INST_COMMON_H__
#define __INST_COMMON_H__

#include "dataArgs.h"
#include "PikaVM.h"
#include "frame.h"

int arg_getLen(Arg* self);
Arg *_vm_get(PikaVMFrame* vm, PikaObj* self, Arg *aKey, Arg *aObj);
Arg* _vm_create_list_or_tuple(PikaObj* self, PikaVMFrame* vm, pika_bool is_list);
void locals_set_lreg(PikaObj* self, char* name, PikaObj* obj);
pika_bool locals_check_lreg(char* data);
void locals_del_lreg(PikaObj* self, char* name);
PikaObj* locals_get_lreg(PikaObj* self, char* name);
PikaObj* locals_new(Args* args);
void locals_deinit(PikaObj* self);

#endif