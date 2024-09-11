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
#include "inst_common.h"
#include "PikaCompiler.h"

Arg *vm_inst_handler_IMP(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    Args buffs = {0};
    char *sModuleNameRedirect = NULL;
    if (NULL == data) {
        goto __exit;
    }
    sModuleNameRedirect = LibObj_redirectModule(pika_getLibObj(), &buffs, data);
    if (NULL != sModuleNameRedirect) {
        data = sModuleNameRedirect;
    }
    /* the module is already imported, skip. */
    if (obj_isArgExist(self, data)) {
        goto __exit;
    }
    extern volatile PikaObj *__pikaMain;
    /* the module is already imported to root object, import it to self. */
    if (obj_isArgExist((PikaObj*)__pikaMain, data)) {
        obj_setArg(self, data, obj_getArg((PikaObj*)__pikaMain, data));
        goto __exit;
    }
    if (NULL == sModuleNameRedirect) {
        /* find cmodule in root object */
        char *cmodule_try = strsGetFirstToken(&buffs, data, '.');
        if (obj_isArgExist((PikaObj*)__pikaMain, cmodule_try)) {
            obj_setArg(self, cmodule_try,
                       obj_getArg((PikaObj*)__pikaMain, cmodule_try));
            goto __exit;
        }
    }

    /* import module from '@lib' */
    if (0 == obj_importModule(self, data)) {
        goto __exit;
    }
    vm_frame_set_error_code(vm, PIKA_RES_ERR_ARG_NO_FOUND);
    vm_frame_set_sys_out(vm, "ModuleNotFoundError: No module named '%s'",
                          data);
__exit:
    strsDeinit(&buffs);
    return NULL;
}