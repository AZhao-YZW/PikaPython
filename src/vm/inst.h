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
#ifndef __INST_H__
#define __INST_H__

#include "dataArgs.h"
#include "PikaVM.h"
#include "frame.h"

Arg *vm_inst_handler_DCT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_DEF(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_CLS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_DEL(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_GLB(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_IMP(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_INH(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_JEZ(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_JNZ(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_LST(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_NEW(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_NLS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_NUM(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_OPT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_OUT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_NON(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_BYT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_EXP(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_GER(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_JMP(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_NTR(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_RET(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_STR(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_TRY(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_SER(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_NEL(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_EST(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_BRK(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_CTN(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_RAS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_REF(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_RIS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_ASS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_RUN(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_SLC(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);
Arg *vm_inst_handler_TPL(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg);

#endif