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

static pika_bool _proxy_setattr(PikaObj *self, char *name, Arg *arg) {
#if PIKA_NANO_ENABLE
    return pika_false;
#endif
    if (obj_getFlag(self, OBJ_FLAG_PROXY_SETATTR)) {
        obj_setStr(self, "@name", name);
        obj_setArg_noCopy(self, "@value", arg);
        /* clang-format off */
        PIKA_PYTHON(
        __setattr__(@name, @value)
        )
        /* clang-format on */
        const uint8_t bytes[] = {
            0x0c, 0x00, 0x00, 0x00, /* instruct array size */
            0x10, 0x81, 0x01, 0x00, 0x10, 0x01, 0x07, 0x00, 0x00, 0x02, 0x0e,
            0x00,
            /* instruct array */
            0x1a, 0x00, 0x00, 0x00, /* const pool size */
            0x00, 0x40, 0x6e, 0x61, 0x6d, 0x65, 0x00, 0x40, 0x76, 0x61, 0x6c,
            0x75, 0x65, 0x00, 0x5f, 0x5f, 0x73, 0x65, 0x74, 0x61, 0x74, 0x74,
            0x72, 0x5f, 0x5f, 0x00, /* const pool */
        };
        pikaVM_runByteCode(self, (uint8_t*)bytes);
        return pika_true;
    }
    return pika_false;
}

Arg *vm_inst_handler_OUT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    char *sArgPath = data;
    pika_assert(sArgPath != NULL);
    char *sArgName = strPointToLastToken(sArgPath, '.');
    PikaObj *oHost = NULL;
    pika_bool bIsTemp = pika_false;
    arg_newReg(aOutReg, PIKA_ARG_BUFF_SIZE);
    PIKA_RES res = PIKA_RES_OK;
    Arg *aOut = stack_popArg(&vm->stack, &aOutReg);
    if (NULL == aOut) {
        goto __exit;
    }
    ArgType eOutArgType = arg_getType(aOut);
    if (vm_frame_get_invoke_deepth_now(vm) > 0) {
        /* in block, is a kw arg */
        arg_setIsKeyword(aOut, pika_true);
        arg_setName(aOut, sArgPath);
        Arg *aRes = arg_copy_noalloc(aOut, arg_ret_reg);
        arg_deinit(aOut);
        return aRes;
    }

    if (locals_check_lreg(sArgPath)) {
        if (argType_isObject(eOutArgType)) {
            PikaObj *obj = arg_getPtr(aOut);
            locals_set_lreg(vm->locals, sArgPath, obj);
            arg_deinit(aOut);
        }
        goto __exit;
    }

    PikaObj *oContext = vm->locals;
    /* match global_list */
    if (obj_getFlag(vm->locals, OBJ_FLAG_GLOBALS)) {
        char *sGlobalList = args_getStr(vm->locals->list, "@g");
        /* use a arg as buff */
        Arg *aGlobalList = arg_newStr(sGlobalList);
        char *sGlobalListBuff = arg_getStr(aGlobalList);
        /* for each arg arg in global_list */
        for (int i = 0; i < strCountSign(sGlobalList, ',') + 1; i++) {
            char *sGlobalArg = strPopFirstToken(&sGlobalListBuff, ',');
            /* matched global arg, context set to global */
            if (strEqu(sGlobalArg, sArgPath)) {
                oContext = vm->globals;
            }
        }
        arg_deinit(aGlobalList);
    }
    /* use RunAs object */
    if (obj_getFlag(vm->locals, OBJ_FLAG_RUN_AS)) {
        oContext = args_getPtr(vm->locals->list, "@r");
    }
    /* set free object to nomal object */
    if (ARG_TYPE_OBJECT_NEW == eOutArgType) {
        pika_assert(NULL != aOut);
        arg_setType(aOut, ARG_TYPE_OBJECT);
    }

    /* ouput arg to context */
    if (sArgPath == sArgName) {
        res = obj_setArg_noCopy(oContext, sArgPath, aOut);
        goto __exit;
    }

    oHost = obj_getHostObjWithIsTemp(oContext, sArgPath, &bIsTemp);

    if (NULL == oHost) {
        oHost = obj_getHostObjWithIsTemp(vm->globals, sArgPath, &bIsTemp);
    }

    if (oHost != NULL) {
        if (_proxy_setattr(oHost, sArgName, aOut)) {
            goto __exit;
        }
        res = obj_setArg_noCopy(oHost, sArgName, aOut);
        goto __exit;
    }

    res = obj_setArg_noCopy(oContext, sArgPath, aOut);
__exit:
    if (res != PIKA_RES_OK) {
        vm_frame_set_error_code(vm, res);
        vm_frame_set_sys_out(vm, "Error: can't set '%s'", sArgPath);
    }
    return NULL;
}
