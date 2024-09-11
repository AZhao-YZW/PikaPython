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

static Arg *_proxy_getattribute(PikaObj *host, char *name) {
#if PIKA_NANO_ENABLE
    return NULL;
#endif
    if ('@' != name[0] && obj_getFlag(host, OBJ_FLAG_PROXY_GETATTRIBUTE)) {
        Arg *aRes = obj_runMethod1(host, "__getattribute__", arg_newStr(name));
        return aRes;
    }
    return NULL;
}

static Arg *_proxy_getattr(PikaObj *host, char *name) {
#if PIKA_NANO_ENABLE
    return NULL;
#endif
    if ('@' != name[0] && obj_getFlag(host, OBJ_FLAG_PROXY_GETATTR)) {
        Arg *aRes = obj_runMethod1(host, "__getattr__", arg_newStr(name));
        return aRes;
    }
    return NULL;
}

Arg *vm_inst_handler_REF(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    PikaObj *oHost = NULL;
    char *arg_path = data;
    char *arg_name = strPointToLastToken(arg_path, '.');
    pika_bool is_temp = pika_false;
    pika_bool is_alloc = pika_false;
    PikaObj *oBuiltins = NULL;

    switch (data[0]) {
        case 'T':
            if (strEqu(arg_path, (char*)"True")) {
                return arg_setBool(arg_ret_reg, "", pika_true);
            }
            break;
        case 'F':
            if (strEqu(arg_path, (char*)"False")) {
                return arg_setBool(arg_ret_reg, "", pika_false);
            }
            break;
        case 'N':
            if (strEqu(arg_path, (char*)"None")) {
                return arg_setNone(arg_ret_reg);
            }
            break;
        case 'R':
            if (strEqu(arg_path, (char*)"RuntimeError")) {
                return arg_setInt(arg_ret_reg, "", PIKA_RES_ERR_RUNTIME_ERROR);
            }
            break;
    }

    Arg *aRes = NULL;
    if (arg_path[0] == '.') {
        /* find host from stack */
        Arg *host_arg = stack_popArg_alloc(&(vm->stack));
        if (NULL == host_arg) {
            goto __exit;
        }
        if (arg_isObject(host_arg)) {
            oHost = arg_getPtr(host_arg);
            aRes = arg_copy_noalloc(obj_getArg(oHost, arg_path + 1), arg_ret_reg);
        }
        arg_deinit(host_arg);
        goto __exit;
    }

    /* find in local list first */
    if (NULL == oHost) {
        oHost = obj_getHostObjWithIsTemp(vm->locals, arg_path, &is_temp);
    }

    /* find in global list */
    if (NULL == oHost) {
        oHost = obj_getHostObjWithIsTemp(vm->globals, arg_path, &is_temp);
    }

    /* error cannot found host_object */
    if (NULL == oHost) {
        goto __exit;
    }

    /* proxy */
    if (NULL == aRes) {
        aRes = _proxy_getattribute(oHost, arg_name);
        if (NULL != aRes) {
            is_alloc = pika_true;
        }
    }

    /* find res in host */
    if (NULL == aRes) {
        aRes = args_getArg(oHost->list, arg_name);
    }

    /* find res in host prop */
    if (NULL == aRes) {
        aRes = _obj_getPropArg(oHost, arg_name);
    }

    /* find res in globals */
    if (arg_path == arg_name) {
        if (NULL == aRes) {
            aRes = args_getArg(vm->globals->list, arg_name);
        }

        /* find res in globals prop */
        if (NULL == aRes) {
            aRes = _obj_getPropArg(vm->globals, arg_name);
        }

        /* find res in builtins */
        if (NULL == aRes) {
            oBuiltins = obj_getBuiltins();
            aRes = args_getArg(oBuiltins->list, arg_name);
        }

        if (NULL == aRes) {
            aRes = _obj_getPropArg(oBuiltins, arg_name);
        }
    }

    /* proxy */
    if (NULL == aRes) {
        aRes = _proxy_getattr(oHost, arg_name);
        if (NULL != aRes) {
            is_alloc = pika_true;
        }
    }
__exit:
    if (NULL != oBuiltins) {
        obj_deinit(oBuiltins);
    }
    if (NULL == aRes) {
        vm_frame_set_error_code(vm, PIKA_RES_ERR_ARG_NO_FOUND);
        vm_frame_set_sys_out(vm, "NameError: name '%s' is not defined",
                              arg_path);
    } else {
        aRes = methodArg_setHostObj(aRes, oHost);
        if ((arg_getType(aRes) != ARG_TYPE_METHOD_NATIVE_ACTIVE) && !is_alloc) {
            aRes = arg_copy_noalloc(aRes, arg_ret_reg);
        }
        pika_assert_arg_alive(aRes);
    }
    if (is_temp) {
        obj_GC(oHost);
    }
    return aRes;
}
