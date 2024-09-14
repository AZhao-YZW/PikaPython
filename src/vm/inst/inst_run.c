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
#include "PikaParser.h"

extern VMParameters* _pikaVM_runByteCodeFrameWithState(PikaObj *self, VMParameters* locals,
    VMParameters* globals, ByteCodeFrame *bytecode_frame, uint16_t pc, PikaVMThread* vm_thread);
extern Arg* obj_runMethodArgWithState_noalloc(PikaObj *self, PikaObj *locals, Arg* method_arg,
    PikaVMThread* vm_thread, Arg* ret_arg_reg);
extern Arg* obj_runMethodArgWithState(PikaObj *self, PikaObj *method_args_obj, Arg* method_arg,
    PikaVMThread* vm_thread);
extern char *_find_super_class_name(ByteCodeFrame *bcframe, int32_t pc_start);
extern PikaObj *New_builtins_object(Args* args);

static Arg *_VM_instruction_eval(PikaObj *self,
                                 PikaVMFrame *vm,
                                 char *sRunPath,
                                 pika_bool* bIsEval) {
    Arg *aReturn = NULL;
    Args buffs = {0};
    *bIsEval = pika_false;
    if (sRunPath[0] != 'e') {
        return NULL;
    }
    if (!strEqu(sRunPath, "eval") && !strEqu(sRunPath, "exec")) {
        return NULL;
    }
    /* eval || exec */
    *bIsEval = pika_true;
    ByteCodeFrame bcFrame = {0};
    /* generate byte code */
    bc_frame_init(&bcFrame);
    Arg *aCode = stack_popArg_alloc(&(vm->stack));
    char *sCode = arg_getStr(aCode);
    char *sCmd = strsAppend(&buffs, "@res = ", sCode);
    if (PIKA_RES_OK != pika_lines2Bytes(&bcFrame, sCmd)) {
        vm_frame_set_sys_out(vm, PIKA_ERR_STRING_SYNTAX_ERROR);
        aReturn = NULL;
        goto __exit;
    }
    _pikaVM_runByteCodeFrameWithState(self, vm->locals, vm->globals, &bcFrame,
                                      0, vm->vm_thread);

    aReturn = obj_getArg(vm->locals, "@res");
    if (NULL == aReturn) {
        aReturn = obj_getArg(vm->globals, "@res");
    }
    if (strEqu(sRunPath, "eval")) {
        aReturn = arg_copy(aReturn);
    }
    goto __exit;
__exit:
    bc_frame_deinit(&bcFrame);
    arg_deinit(aCode);
    strsDeinit(&buffs);
    return aReturn;
}

#if !PIKA_NANO_ENABLE
static char *_find_self_name(PikaVMFrame *vm) {
    /* find super class */
    int offset = 0;
    char *self_name = NULL;
    while (1) {
        offset -= inst_unit_get_size();
        if (vm->pc + offset >= (int)vm_frame_get_inst_array_size(vm)) {
            return 0;
        }
        if ((PIKA_INS(CLS) == vm_frame_get_inst_with_offset(vm, offset))) {
            break;
        }
    }

    while (1) {
        offset += inst_unit_get_size();
        if (vm->pc + offset >= (int)vm_frame_get_inst_array_size(vm)) {
            return 0;
        }
        if ((PIKA_INS(OUT) ==
             inst_unit_get_inst_index(
                 vm_frame_get_inst_unit_with_offset(vm, offset)))) {
            self_name = vm_frame_get_const_with_offset(vm, offset);
            return self_name;
        }
    }
}
#endif

Arg *vm_inst_handler_RUN(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    Arg *aReturn = NULL;
    VMParameters* oSublocals = NULL;
    VMParameters* oSublocalsInit = NULL;
    char *sRunPath = data;
    char *sArgName = NULL;
    char *sProxyName = NULL;
    PikaObj *oMethodHost = NULL;
    PikaObj *oThis = NULL;
    Arg *aMethod = NULL;
    Arg *aStack = NULL;
    pika_bool bIsTemp = pika_false;
    pika_bool bSkipInit = pika_false;
    pika_bool bIsEval = pika_false;
    int iNumUsed = 0;
    PikaObj *oBuiltin = NULL;
    arg_newReg(arg_reg1, 32);
    pika_assert(NULL != vm->vm_thread);

    if (NULL != sRunPath) {
        sArgName = strPointToLastToken(sRunPath, '.');
    }

    /* inhert */
    if (vm->pc - 2 * (int)inst_unit_get_size() >= 0) {
        if (PIKA_INS(CLS) == vm_frame_get_inst_with_offset(
                                 vm, -2 * (int)inst_unit_get_size())) {
            bSkipInit = pika_true;
        }
    }

    /* tuple or single arg */
    if (NULL == sRunPath || sRunPath[0] == 0) {
        if (vm_frame_get_input_arg_num(vm) == 1) {
            /* return arg directly */
            aReturn = stack_popArg(&(vm->stack), arg_ret_reg);
            goto __exit;
        }
        /* create a tuple */
        aReturn = _vm_create_list_or_tuple(self, vm, pika_false);
        goto __exit;
    }

#if !PIKA_NANO_ENABLE
    /* support for super() */
    if (strEqu(sRunPath, "super")) {
        sRunPath = _find_super_class_name(vm->bytecode_frame, vm->pc);
        sArgName = strPointToLastToken(sRunPath, '.');
        vm->in_super = pika_true;
        vm->super_invoke_deepth = vm_frame_get_invoke_deepth_now(vm);
        bSkipInit = pika_true;
    }
#endif

    /* return tiny obj */
    if (strEqu(sRunPath, "TinyObj")) {
        aReturn = arg_newMetaObj(New_builtins_object);
        if (NULL != aReturn) {
            goto __exit;
        }
    }

    /* eval and exec */
    aReturn = _VM_instruction_eval(self, vm, sRunPath, &bIsEval);
    if (bIsEval) {
        goto __exit;
    }

    /* get method host obj from reg */
    if (NULL == oMethodHost) {
        oMethodHost = locals_get_lreg(vm->locals, sRunPath);
    }

#if !PIKA_NANO_ENABLE
    /* get method host obj from stack */
    if (NULL == oMethodHost && sRunPath[0] == '.') {
        /* get method host obj from stack */
        Arg** stack_tmp = (Arg**)pikaMalloc(sizeof(Arg*) * PIKA_ARG_NUM_MAX);
        uint32_t n_arg = vm_frame_get_input_arg_num(vm);
        if (n_arg > PIKA_ARG_NUM_MAX) {
            pika_platform_printf(
                "[ERROR] Too many args in RUN instruction, please use "
                "bigger "
                "#define PIKA_ARG_NUM_MAX\n");
            pika_platform_panic_handle();
        }
        for (int i = 0; i < n_arg; i++) {
            stack_tmp[i] = stack_popArg_alloc(&(vm->stack));
        }
        aStack = stack_tmp[n_arg - 1];
        if (sRunPath[1] == '\0') {
            /* for .(...) */
            aMethod = aStack;
            pika_assert(arg_isCallable(aMethod));
        } else {
            /* for .xxx(...) */
            oMethodHost = _arg_to_obj(aStack, &bIsTemp);
            pika_assert(NULL != oMethodHost);
        }
        if (NULL != aStack) {
            iNumUsed++;
        }
        /* push back other args to stack */
        for (int i = n_arg - 2; i >= 0; i--) {
            stack_pushArg(&(vm->stack), stack_tmp[i]);
        }
        pikaFree(stack_tmp, sizeof(Arg*) * PIKA_ARG_NUM_MAX);
    }
#endif
    /* use RunAs object */
    if (obj_getFlag(vm->locals, OBJ_FLAG_RUN_AS)) {
        PikaObj *oContext = args_getPtr(vm->locals->list, "@r");
        oMethodHost = obj_getHostObjWithIsTemp(oContext, sRunPath, &bIsTemp);
    }

    /* get method host obj from local scope */
    if (NULL == oMethodHost) {
        oMethodHost = obj_getHostObjWithIsTemp(vm->locals, sRunPath, &bIsTemp);
    }

    /* get method host obj from global scope */
    if (NULL == oMethodHost) {
        oMethodHost = obj_getHostObjWithIsTemp(vm->globals, sRunPath, &bIsTemp);
    }

    /* method host obj is not found */
    if (NULL == oMethodHost) {
        /* error, not found object */
        vm_frame_set_error_code(vm, PIKA_RES_ERR_ARG_NO_FOUND);
        vm_frame_set_sys_out(vm, "Error: method '%s' no found.", sRunPath);
        goto __exit;
    }

    pika_assert(obj_checkAlive(oMethodHost));

#if !PIKA_NANO_ENABLE
    if (!bSkipInit && vm->in_super &&
        vm_frame_get_invoke_deepth_now(vm) == vm->super_invoke_deepth - 1) {
        vm->in_super = pika_false;
        oThis = obj_getPtr(vm->locals, _find_self_name(vm));
    }
#endif

    /* get object this */
    if (NULL == oThis) {
        oThis = oMethodHost;
    }

    /* get method in object */
    if (NULL == aMethod) {
        aMethod = obj_getMethodArg_noalloc(oMethodHost, sArgName, &arg_reg1);
    }

    if (sArgName == sRunPath) {
        /* find method in locals */
        if (NULL == aMethod) {
            aMethod = obj_getMethodArg_noalloc(vm->locals, sArgName, &arg_reg1);
        }
        /* find method in global */
        if (NULL == aMethod) {
            aMethod =
                obj_getMethodArg_noalloc(vm->globals, sArgName, &arg_reg1);
            if (aMethod != NULL) {
                oThis = vm->globals;
            }
        }
        /* find method in builtin */
        if (NULL == aMethod) {
            oBuiltin = obj_getBuiltins();
            aMethod = obj_getMethodArg_noalloc(oBuiltin, sArgName, &arg_reg1);
            if (aMethod != NULL) {
                oThis = oBuiltin;
            }
        }
    }

    if (NULL == aMethod) {
        /* get proxy method */
        if (obj_getFlag(oMethodHost, OBJ_FLAG_PROXY_METHOD)) {
            if (strCountSign(sArgName, '.') == 0) {
                /* __proxy__ takes the first arg as the method name */
                sProxyName = sArgName;
                sArgName = "__proxy__";
                aMethod =
                    obj_getMethodArg_noalloc(oMethodHost, sArgName, &arg_reg1);
            }
        }
    }

    /* assert method exist */
    if (NULL == aMethod || ARG_TYPE_NONE == arg_getType(aMethod)) {
        /* error, method no found */
        vm_frame_set_error_code(vm, PIKA_RES_ERR_ARG_NO_FOUND);
        vm_frame_set_sys_out(vm, "NameError: name '%s' is not defined",
                              sRunPath);
        goto __exit;
    }

    /* assert methodd type */
    if (!argType_isCallable(arg_getType(aMethod))) {
        /* error, method no found */
        vm_frame_set_error_code(vm, PIKA_RES_ERR_ARG_NO_FOUND);
        vm_frame_set_sys_out(vm, "TypeError: '%s' object is not callable",
                              sRunPath);
        goto __exit;
    }

    /* create sub local scope */
    oSublocals = locals_new(NULL);
    oThis->vmFrame = vm;

    /* load args from PikaVMFrame to sub_local->list */
    iNumUsed += vm_frame_load_args_from_method_arg(
        vm, oThis, oSublocals->list, aMethod, sRunPath, sProxyName, iNumUsed);

    /* load args failed */
    if (vm->vm_thread->error_code != 0) {
        goto __exit;
    }

    /* run method arg */
    aReturn = obj_runMethodArgWithState_noalloc(oThis, oSublocals, aMethod,
                                                vm->vm_thread, arg_ret_reg);
    if (bSkipInit) {
        if (arg_getType(aReturn) == ARG_TYPE_OBJECT_NEW) {
            pika_assert(NULL != aReturn);
            arg_setType(aReturn, ARG_TYPE_OBJECT);
        }
    }

    if (vm->vm_thread->try_result != TRY_RESULT_NONE) {
        /* try result */
        vm->vm_thread->error_code = vm->vm_thread->try_result;
    }

    /* __init__() */
    if (NULL != aReturn && ARG_TYPE_OBJECT_NEW == arg_getType(aReturn)) {
        pika_assert(NULL != aReturn);
        arg_setType(aReturn, ARG_TYPE_OBJECT);
        /* init object */
        PikaObj *oNew = arg_getPtr(aReturn);
        obj_setName(oNew, sRunPath);
        Arg *aMethod2 = obj_getMethodArgWithFullPath_noalloc(oNew, "__init__", &arg_reg1);
        oSublocalsInit = locals_new(NULL);
        Arg *aReturnInit = NULL;
        if (NULL == aMethod2) {
            goto __init_exit;
        }
        vm_frame_load_args_from_method_arg(vm, oNew, oSublocalsInit->list,
                                          aMethod2, "__init__", NULL, iNumUsed);
        /* load args failed */
        if (vm->vm_thread->error_code != 0) {
            goto __init_exit;
        }
        aReturnInit = obj_runMethodArgWithState(oNew, oSublocalsInit, aMethod2, vm->vm_thread);
    __init_exit:
        if (NULL != aReturnInit) {
            arg_deinit(aReturnInit);
        }
#if PIKA_GC_MARK_SWEEP_ENABLE
        pika_assert(obj_getFlag(oSublocals, OBJ_FLAG_GC_ROOT));
#endif
        obj_deinit(oSublocalsInit);
        if (NULL != aMethod2) {
            arg_deinit(aMethod2);
        }
    }

__exit:
    if (NULL != aMethod) {
        arg_deinit(aMethod);
    }
    if (NULL != oSublocals) {
#if PIKA_GC_MARK_SWEEP_ENABLE
        pika_assert(obj_getFlag(oSublocals, OBJ_FLAG_GC_ROOT));
#endif
        obj_deinit(oSublocals);
    }
    if (NULL != oBuiltin) {
        obj_deinit(oBuiltin);
    }
    if (NULL != aStack && aMethod != aStack) {
        arg_deinit(aStack);
    }
    if (NULL != oMethodHost && bIsTemp) {
        /* class method */
        obj_GC(oMethodHost);
    }
    pika_assert_arg_alive(aReturn);
    return aReturn;
}