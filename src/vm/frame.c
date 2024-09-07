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
#include "frame.h"
#include "dataArgs.h"
#include "PikaVM.h"

void vm_frame_set_error_code(PikaVMFrame* vm, int8_t error_code)
{
    vm->vm_thread->error_code = error_code;
}

void _do_vsysOut(char* fmt, va_list args);
void vm_frame_set_sys_out(PikaVMFrame* vm, char* fmt, ...)
{
    pika_assert(NULL != vm);
    if (vm->vm_thread->error_code == 0) {
        vm->vm_thread->error_code = PIKA_RES_ERR_RUNTIME_ERROR;
    }
    if (vm->vm_thread->try_state == TRY_STATE_INNER) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    _do_vsysOut(fmt, args);
    va_end(args);
}

enum InstructIndex vm_frame_get_inst_with_offset(PikaVMFrame* vm, int32_t offset)
{
    return instructUnit_getInstructIndex(PikaVMFrame_getInstructUnitWithOffset(vm, offset));
}

int vm_frame_get_blk_deepth_now(PikaVMFrame* vm)
{
    /* support run byteCode */
    InstructUnit* ins_unit = PikaVMFrame_getInstructNow(vm);
    return instructUnit_getBlockDeepth(ins_unit);
}

#if !PIKA_NANO_ENABLE
char* vm_frame_get_const_with_offset(PikaVMFrame* vm, int32_t offset)
{
    return PikaVMFrame_getConstWithInstructUnit(
        vm, PikaVMFrame_getInstructUnitWithOffset(vm, offset));
}
#endif

int vm_frame_get_invoke_deepth_now(PikaVMFrame* vm)
{
    /* support run byteCode */
    InstructUnit* ins_unit = PikaVMFrame_getInstructNow(vm);
    return instructUnit_getInvokeDeepth(ins_unit);
}

extern volatile VMState g_PikaVMState;
pika_bool vm_frame_check_break_point(PikaVMFrame* vm)
{
#if !PIKA_DEBUG_BREAK_POINT_MAX
    return pika_false;
#else
    if (g_PikaVMState.break_point_cnt == 0) {
        return pika_false;
    }
    if (NULL == vm->bytecode_frame->name) {
        return pika_false;
    }
    Hash module_hash = byteCodeFrame_getNameHash(vm->bytecode_frame);
    return pika_debug_check_break_hash(module_hash, vm->pc);
#endif
}

static int32_t vm_frame_get_jmp_back_addr_offset(PikaVMFrame* vm)
{
    int offset = 0;
    int blockDeepthGot = -1;
    int blockDeepthNow = vm_frame_get_blk_deepth_now(vm);

    /* find loop depth */
    while (1) {
        offset -= instructUnit_getSize();
        InstructUnit* insUnitThis =
            PikaVMFrame_getInstructUnitWithOffset(vm, offset);
        uint16_t invokeDeepth = instructUnit_getInvokeDeepth(insUnitThis);
        enum InstructIndex ins = instructUnit_getInstructIndex(insUnitThis);
        char* data = PikaVMFrame_getConstWithInstructUnit(vm, insUnitThis);
        if ((0 == invokeDeepth) && (PIKA_INS(JEZ) == ins) && data[0] == '2') {
            InstructUnit* insUnitLast = PikaVMFrame_getInstructUnitWithOffset(
                vm, offset - instructUnit_getSize());
            enum InstructIndex insLast =
                instructUnit_getInstructIndex(insUnitLast);
            /* skip try stmt */
            if (PIKA_INS(GER) == insLast) {
                continue;
            }
            /* skip inner break */
            int blockDeepthThis = instructUnit_getBlockDeepth(insUnitThis);
            if (blockDeepthThis >= blockDeepthNow) {
                continue;
            }
            blockDeepthGot = instructUnit_getBlockDeepth(insUnitThis);
            break;
        }
    }

    offset = 0;
    while (1) {
        offset += instructUnit_getSize();
        InstructUnit* insUnitThis =
            PikaVMFrame_getInstructUnitWithOffset(vm, offset);
        enum InstructIndex ins = instructUnit_getInstructIndex(insUnitThis);
        char* data = PikaVMFrame_getConstWithInstructUnit(vm, insUnitThis);
        int blockDeepthThis = instructUnit_getBlockDeepth(insUnitThis);
        if ((blockDeepthThis == blockDeepthGot) && (PIKA_INS(JMP) == ins) &&
            data[0] == '-' && data[1] == '1') {
            return offset;
        }
    }
}

int32_t vm_frame_get_jmp_addr_offset(PikaVMFrame* vm)
{
    int offset = 0;
    /* run byte Code */
    InstructUnit* this_ins_unit = PikaVMFrame_getInstructNow(vm);
    int this_blk_depth = instructUnit_getBlockDeepth(this_ins_unit);
    int8_t blockNum = 0;
    int pc_max = (int)PikaVMFrame_getInstructArraySize(vm);
    if (vm->jmp > 0) {
        offset = 0;
        while (1) {
            offset += instructUnit_getSize();
            /* reach the end */
            if (vm->pc + offset >= pc_max) {
                break;
            }
            this_ins_unit = PikaVMFrame_getInstructUnitWithOffset(vm, offset);
            if (instructUnit_getIsNewLine(this_ins_unit)) {
                uint8_t blockDeepth =
                    instructUnit_getBlockDeepth(this_ins_unit);
                if (blockDeepth <= this_blk_depth) {
                    blockNum++;
                }
            }
            if (blockNum >= vm->jmp) {
                break;
            }
        }
    }
    if (vm->jmp < 0) {
        while (1) {
            offset -= instructUnit_getSize();
            this_ins_unit = PikaVMFrame_getInstructUnitWithOffset(vm, offset);
            if (instructUnit_getIsNewLine(this_ins_unit)) {
                uint8_t blockDeepth =
                    instructUnit_getBlockDeepth(this_ins_unit);
                if (blockDeepth == this_blk_depth) {
                    blockNum--;
                }
            }
            if (blockNum <= vm->jmp) {
                break;
            }
        }
    }
    return offset;
}

int32_t vm_frame_get_break_addr_offset(PikaVMFrame* vm)
{
    int32_t offset = vm_frame_get_jmp_back_addr_offset(vm);
    /* byteCode */
    offset += instructUnit_getSize();
    return offset;
}

#if !PIKA_NANO_ENABLE
int32_t vm_frame_get_raise_addr_offset(PikaVMFrame* vm)
{
    int offset = 0;
    InstructUnit* ins_unit_now = PikaVMFrame_getInstructNow(vm);
    while (1) {
        offset += instructUnit_getSize();
        if (vm->pc + offset >= (int)PikaVMFrame_getInstructArraySize(vm)) {
            return 0;
        }
        ins_unit_now = PikaVMFrame_getInstructUnitWithOffset(vm, offset);
        enum InstructIndex ins = instructUnit_getInstructIndex(ins_unit_now);
        if (PIKA_INS(NTR) == ins) {
            return offset;
        }
        /* if not found except, return */
        if (PIKA_INS(RET) == ins) {
            return 0;
        }
    }
}
#endif

int32_t vm_frame_get_continue_addr_offset(PikaVMFrame* vm)
{
    int32_t offset = vm_frame_get_jmp_back_addr_offset(vm);
    /* byteCode */
    return offset;
}

static void vm_frame_init_reg(PikaVMFrame* self)
{
    for (uint8_t i = 0; i < PIKA_REGIST_SIZE; i++) {
        self->ireg[i] = pika_false;
    }
    pika_platform_memset(self->oreg, 0, sizeof(self->oreg));
}

static void _type_list_parse(FunctionArgsInfo* f)
{
    if (f->type_list[0] == 0) {
        f->n_positional = 0;
        return;
    }
    int8_t iArgc = strCountSign(f->type_list, ',') + 1;
    int8_t iStar = strCountSign(f->type_list, '*');
    int8_t iAssign = strCountSign(f->type_list, '=');
    /* default */
    if (iAssign > 0) {
        iArgc -= iAssign;
        f->is_default = pika_true;
        f->n_default = iAssign;
    }
    /* vars */
    if (iStar == 1) {
        f->is_vars = pika_true;
        f->n_positional = iArgc - 1;
        return;
    }
    /* kw */
    if (iStar == 2) {
        f->is_keys = pika_true;
        f->n_positional = iArgc - 1;
        return;
    }
    /* vars and kw */
    if (iStar == 3) {
        f->is_vars = pika_true;
        f->is_keys = pika_true;
        f->n_positional = iArgc - 2;
        return;
    }
    f->n_positional = iArgc;
    return;
}

static uint32_t _get_n_input_with_unpack(PikaVMFrame* vm, int n_used)
{
#if PIKA_NANO_ENABLE
    return vm_frame_get_input_arg_num(vm);
#else
    uint32_t n_input = vm_frame_get_input_arg_num(vm) - n_used;
    int get_star = 0;
    int unpack_num = 0;
    for (int i = 0; i < n_input; i++) {
        Arg* arg_check = stack_checkArg(&(vm->stack), i);
        if (NULL == arg_check) {
            break;
        }
        if (arg_getIsDoubleStarred(arg_check) || arg_getIsStarred(arg_check)) {
            get_star++;
        }
    }
    if (0 == get_star) {
        return n_input;
    }
    Stack stack_tmp = {0};
    stack_init(&stack_tmp);
    for (int i = 0; i < n_input; i++) {
        /* unpack starred arg */
        Arg* call_arg = stack_popArg_alloc(&(vm->stack));
        if (call_arg == NULL) {
            break;
        }
        if (arg_getIsStarred(call_arg)) {
            if (!arg_isObject(call_arg)) {
                stack_pushArg(&stack_tmp, arg_copy(call_arg));
                goto __continue;
            }
            PikaObj* obj = arg_getPtr(call_arg);
            int len = obj_getSize(obj);
            for (int i_star_arg = len - 1; i_star_arg >= 0; i_star_arg--) {
                Arg* arg_a =
                    obj_runMethod1(obj, "__getitem__", arg_newInt(i_star_arg));
                stack_pushArg(&stack_tmp, arg_a);
                unpack_num++;
            }
            goto __continue;
        }
        if (arg_getIsDoubleStarred(call_arg)) {
            pika_assert(arg_isObject(call_arg));
            PikaObj* New_PikaStdData_Dict(Args * args);
            PikaObj* obj = arg_getPtr(call_arg);
            pika_assert(obj->constructor == New_PikaStdData_Dict);
            Args* dict = _OBJ2DICT(obj);
            int i_item = args_getSize(dict);
            while (pika_true) {
                i_item--;
                if (i_item < 0) {
                    break;
                }
                Arg* item_val = args_getArgByIndex(dict, i_item);
                /* unpack as keyword arg */
                arg_setIsKeyword(item_val, pika_true);
                stack_pushArg(&stack_tmp, arg_copy(item_val));
            }
            goto __continue;
        }
        stack_pushArg(&stack_tmp, arg_copy(call_arg));
    __continue:
        if (NULL != call_arg) {
            arg_deinit(call_arg);
        }
    }
    uint32_t n_input_new = stack_getTop(&stack_tmp);
    for (int i = 0; i < n_input_new; i++) {
        Arg* arg = stack_popArg_alloc(&stack_tmp);
        stack_pushArg(&(vm->stack), arg);
    }
    stack_deinit(&stack_tmp);
    return n_input_new;
#endif
}

static char* _kw_pos_to_default_all(FunctionArgsInfo* f, char* sArgName, int* argc,
                                    Arg* argv[], Arg* aCall)
{
#if PIKA_NANO_ENABLE
    return sArgName;
#endif
    int iDefaultSkip = 0;
    int iDefaultSkiped = 0;
    if (f->i_arg == f->n_arg) {
        iDefaultSkip = f->n_default - f->n_arg + f->n_positional;
    }
    while (strIsContain(sArgName, '=')) {
        strPopLastToken(sArgName, '=');
        Arg* aKeyword = NULL;
        /* load default arg from kws */
        if (f->kw != NULL) {
            aKeyword = pikaDict_get(f->kw, sArgName);
            if (aKeyword != NULL) {
                argv[(*argc)++] = arg_copy(aKeyword);
                pikaDict_removeArg(f->kw, aKeyword);
                goto __next;
            }
        }

        /* load default arg from pos */
        if (NULL != aCall && f->is_default) {
            /* load pos to default with right order */
            if (iDefaultSkiped < iDefaultSkip) {
                iDefaultSkiped++;
                sArgName = strPopLastToken(f->type_list, ',');
                continue;
            }
            /* load default from pos */
            if (f->i_arg > f->n_positional) {
                arg_setNameHash(aCall, hash_time33EndWith(sArgName, ':'));
                argv[(*argc)++] = aCall;
                return (char*)1;
            }
        }

    __next:
        sArgName = strPopLastToken(f->type_list, ',');
    }
    return sArgName;
}

static int _kw_to_pos_one(FunctionArgsInfo* f,
                          char* arg_name,
                          int* argc,
                          Arg* argv[]) {
    if (f->kw == NULL) {
        return 0;
    }
    Arg* pos_arg = pikaDict_get(f->kw, arg_name);
    if (pos_arg == NULL) {
        return 0;
    }
    argv[(*argc)++] = arg_copy(pos_arg);
    pikaDict_removeArg(f->kw, pos_arg);
    return 1;
}

static void _kw_push(FunctionArgsInfo* f, Arg* call_arg, int i) {
    if (NULL == f->kw) {
        f->kw = New_PikaDict();
    }
    arg_setIsKeyword(call_arg, pika_false);
    Hash kw_hash = call_arg->name_hash;
    char buff[32] = {0};
    _pikaDict_setVal(f->kw, call_arg);
    char* sHash = fast_itoa(buff, kw_hash);
    args_setStr(_OBJ2KEYS(f->kw), sHash, sHash);
    pikaDict_reverse(f->kw);
}

static void _load_call_arg(PikaVMFrame* vm, Arg* call_arg, FunctionArgsInfo* f, int* i,
                           int* argc, Arg* argv[])
{
    /* load the kw arg */
    pika_assert(NULL != call_arg);
    if (arg_getIsKeyword(call_arg)) {
        _kw_push(f, call_arg, *i);
        return;
    }
    /* load variable arg */
    if (f->i_arg > f->n_positional) {
        if (f->is_vars) {
            pikaList_append(f->tuple, call_arg);
            return;
        }
    }
    char* arg_name = strPopLastToken(f->type_list, ',');
    /* load default from kw */
    arg_name = _kw_pos_to_default_all(f, arg_name, argc, argv, call_arg);
    if (((char*)1) == arg_name) {
        /* load default from pos */
        return;
    }
    /* load position arg */
    if (_kw_to_pos_one(f, arg_name, argc, argv)) {
        /* load pos from kw */
        (f->n_positional_got)++;
        /* restore the stack */
        (*i)--;
        stack_pushArg(&(vm->stack), call_arg);
        return;
    }
    /*load pos from pos */
    arg_setNameHash(call_arg, hash_time33EndWith(arg_name, ':'));
    pika_assert(call_arg != NULL);
    argv[(*argc)++] = call_arg;
    (f->n_positional_got)++;
}

static void _kw_to_pos_all(FunctionArgsInfo* f, int* argc, Arg* argv[])
{
    int arg_num_need = f->n_positional - f->n_positional_got;
    if (0 == arg_num_need) {
        return;
    }
    for (int i = 0; i < arg_num_need; i++) {
        char* arg_name = strPopLastToken(f->type_list, ',');
        pika_assert(f->kw != NULL);
        Arg* pos_arg = pikaDict_get(f->kw, arg_name);
        pika_assert(pos_arg != NULL);
        argv[(*argc)++] = arg_copy(pos_arg);
        pikaDict_removeArg(f->kw, pos_arg);
    }
}

static void _loadLocalsFromArgv(Args* locals, int argc, Arg* argv[])
{
    for (int i = 0; i < argc; i++) {
        Arg* arg = argv[i];
        pika_assert(arg != NULL);
        args_setArg(locals, arg);
    }
}

#define vars_or_keys_or_default (f.is_vars || f.is_keys || f.is_default)
#define METHOD_TYPE_LIST_MAX_LEN PIKA_LINE_BUFF_SIZE * 2
int vm_frame_load_args_from_method_arg(PikaVMFrame* vm, PikaObj* oMethodHost, Args* aLoclas,
                                       Arg* aMethod, char* sMethodName, char* sProxyName, int iNumUsed)
{
    int argc = 0;
    Arg** argv = (Arg**)pikaMalloc(sizeof(Arg*) * PIKA_ARG_NUM_MAX);
    char* buffs1 = (char*)pikaMalloc(METHOD_TYPE_LIST_MAX_LEN);
    char* buffs2 = (char*)pikaMalloc(METHOD_TYPE_LIST_MAX_LEN);
    FunctionArgsInfo f = {0};
    char* type_list_buff = NULL;
    /* get method type list */
    f.type_list =
        methodArg_getTypeList(aMethod, buffs1, METHOD_TYPE_LIST_MAX_LEN);
    pika_assert_msg(NULL != f.type_list, "method: %s", sMethodName);
    if (NULL == f.type_list) {
        pika_platform_printf(
            "OverflowError: type list is too long, please use bigger "
            "PIKA_LINE_BUFF_SIZE\r\n");
        pika_platform_panic_handle();
    }
    f.method_type = arg_getType(aMethod);

    /* get arg_num_pos */
    _type_list_parse(&f);
    if (f.method_type == ARG_TYPE_METHOD_OBJECT) {
        /* delete the 'self' */
        f.n_positional--;
    }

    f.n_input = _get_n_input_with_unpack(vm, iNumUsed);

    if (NULL != sProxyName) {
        /* method proxy takes the first arg as method name */
        f.n_input++;
    }

    do {
        /* check arg num */
        if (f.method_type == ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR ||
            f.method_type == ARG_TYPE_METHOD_CONSTRUCTOR || iNumUsed != 0) {
            /* skip for constrctor */
            /* skip for variable args */
            /* n_used != 0 means it is a factory method */
            break;
        }
        /* check position arg num */
        if (!vars_or_keys_or_default) {
            if (f.n_positional != f.n_input) {
                vm_frame_set_error_code(vm, PIKA_RES_ERR_INVALID_PARAM);
                vm_frame_set_sys_out(
                    vm,
                    "TypeError: %s() takes %d positional argument but %d "
                    "were "
                    "given",
                    sMethodName, f.n_positional, f.n_input);
                goto __exit;
            }
            break;
        }
#if !PIKA_NANO_ENABLE
        if (f.is_keys) {
            break;
        }
        if (f.is_vars) {
            if (f.n_input < f.n_positional) {
                vm_frame_set_error_code(vm, PIKA_RES_ERR_INVALID_PARAM);
                vm_frame_set_sys_out(
                    vm,
                    "TypeError: %s() takes %d positional argument but "
                    "%d "
                    "were "
                    "given",
                    sMethodName, f.n_positional, f.n_input);
                goto __exit;
            }
            break;
        }
        if (f.is_default) {
            int8_t n_min = f.n_positional;
            int8_t n_max = f.n_positional + f.n_default;
            if (f.n_input < n_min || f.n_input > n_max) {
                vm_frame_set_error_code(vm, PIKA_RES_ERR_INVALID_PARAM);
                vm_frame_set_sys_out(
                    vm,
                    "TypeError: %s() takes from %d to %d positional "
                    "arguments "
                    "but %d were given",
                    sMethodName, n_min, n_max, f.n_input);
                goto __exit;
            }
        }
#endif
    } while (0);

    if (vars_or_keys_or_default) {
        f.n_arg = f.n_input;
    } else {
        f.n_arg = f.n_positional;
    }

    /* create tuple/dict for vars/keys */
    if (vars_or_keys_or_default) {
        if (strGetSize(f.type_list) > METHOD_TYPE_LIST_MAX_LEN) {
            pika_platform_printf(
                "OverFlowError: please use bigger PIKA_LINE_BUFF_SIZE\r\n");
            while (1) {
            }
        }
        type_list_buff = strCopy(buffs2, f.type_list);
        uint8_t n_typelist = strCountSign(type_list_buff, ',') + 1;
        for (int i = 0; i < n_typelist; i++) {
            char* arg_def = strPopLastToken(type_list_buff, ',');
            if (arg_def[0] == '*' && arg_def[1] != '*') {
                /* get variable tuple name */
                /* skip the '*' */
                f.var_tuple_name = arg_def + 1;
                /* create tuple */
                if (NULL == f.tuple) {
                    f.tuple = New_pikaTuple();
                    /* remove the format arg */
                    strPopLastToken(f.type_list, ',');
                }
                continue;
            }
            if (arg_def[0] == '*' && arg_def[1] == '*') {
                /* get kw dict name */
                f.kw_dict_name = arg_def + 2;
                f.kw = New_PikaDict();
                /* remove the format arg */
                strPopLastToken(f.type_list, ',');
                continue;
            }
        }
    }

    /* load args */
    for (int i = 0; i < f.n_arg; i++) {
        Arg* call_arg = NULL;
        f.i_arg = f.n_arg - i;
        if (NULL != sProxyName && i == f.n_arg - 1) {
            call_arg = arg_newStr(sProxyName);
        } else {
            call_arg = stack_popArg_alloc(&(vm->stack));
        }
        if (NULL == call_arg) {
            call_arg = arg_newNone();
        }
        _load_call_arg(vm, call_arg, &f, &i, &argc, argv);
    }

/* only default */
#if !PIKA_NANO_ENABLE
    if (strIsContain(f.type_list, '=')) {
        char* arg_name = strPopLastToken(f.type_list, ',');
        _kw_pos_to_default_all(&f, arg_name, &argc, argv, NULL);
    }
    /* load kw to pos */
    _kw_to_pos_all(&f, &argc, argv);
#endif

    if (f.tuple != NULL) {
        pikaList_reverse(f.tuple);
        /* load variable tuple */
        Arg* argi =
            arg_setPtr(NULL, f.var_tuple_name, ARG_TYPE_OBJECT, f.tuple);
        argv[argc++] = argi;
    }

    if (f.kw != NULL) {
        if (NULL == f.kw_dict_name) {
            f.kw_dict_name = "__kwargs";
        }
        /* load kw dict */
        PikaObj* New_PikaStdData_Dict(Args * args);
        Arg* argi = arg_setPtr(NULL, f.kw_dict_name, ARG_TYPE_OBJECT, f.kw);
        argv[argc++] = argi;
    }

    /* load 'self' as the first arg when call object method */
    if (f.method_type == ARG_TYPE_METHOD_OBJECT) {
        PikaObj* method_self = NULL;
        method_self = methodArg_getHostObj(aMethod);
        if (NULL == method_self) {
            method_self = oMethodHost;
        }

        Arg* call_arg = arg_setRef(NULL, "self", method_self);
        pika_assert(call_arg != NULL);
        argv[argc++] = call_arg;
    }
    _loadLocalsFromArgv(aLoclas, argc, argv);
__exit:
    pikaFree(buffs1, METHOD_TYPE_LIST_MAX_LEN);
    pikaFree(buffs2, METHOD_TYPE_LIST_MAX_LEN);
    pikaFree(argv, sizeof(Arg*) * PIKA_ARG_NUM_MAX);
    return f.n_arg;
}

uint32_t vm_frame_get_input_arg_num(PikaVMFrame* vm)
{
    InstructUnit* ins_unit_now = PikaVMFrame_getInstructNow(vm);
    uint8_t invoke_deepth_this = instructUnit_getInvokeDeepth(ins_unit_now);
    int32_t pc_this = vm->pc;
    uint32_t num = 0;
    while (1) {
        ins_unit_now--;
        pc_this -= instructUnit_getSize();
        uint8_t invode_deepth = instructUnit_getInvokeDeepth(ins_unit_now);
        if (pc_this < 0) {
            break;
        }
        if (invode_deepth == invoke_deepth_this + 1) {
            if (instructUnit_getInstructIndex(ins_unit_now) == PIKA_INS(OUT)) {
                continue;
            }
            if (instructUnit_getInstructIndex(ins_unit_now) == PIKA_INS(NLS)) {
                continue;
            }
            num++;
        }
        if (instructUnit_getIsNewLine(ins_unit_now)) {
            break;
        }
        if (invode_deepth <= invoke_deepth_this) {
            break;
        }
    }
    return num;
}

PIKA_WEAK void pika_hook_unused_stack_arg(PikaVMFrame* vm, Arg* arg) {
    if (vm->in_repl) {
        arg_print(arg, pika_true, "\r\n");
    }
}

void vm_frame_solve_unused_stack(PikaVMFrame* vm)
{
    uint8_t top = stack_getTop(&(vm->stack));
    for (int i = 0; i < top; i++) {
        Arg* arg = stack_popArg_alloc(&(vm->stack));
        ArgType type = arg_getType(arg);
        if (type == ARG_TYPE_NONE) {
            arg_deinit(arg);
            continue;
        }
        if (vm->vm_thread->line_error_code != 0) {
            arg_deinit(arg);
            continue;
        }
        pika_hook_unused_stack_arg(vm, arg);
        arg_deinit(arg);
    }
}

PikaVMFrame* vm_frame_create(VMParameters* locals, VMParameters* globals, ByteCodeFrame* bytecode_frame,
                             int32_t pc, PikaVMThread* vm_thread)
{
    PikaVMFrame* vm = (PikaVMFrame*)pikaMalloc(sizeof(PikaVMFrame));
    vm->bytecode_frame = bytecode_frame;
    vm->locals = locals;
    vm->globals = globals;
    vm->pc = pc;
    vm->vm_thread = vm_thread;
    vm->jmp = 0;
    vm->loop_deepth = 0;
    vm->vm_thread->error_code = PIKA_RES_OK;
    vm->vm_thread->line_error_code = PIKA_RES_OK;
    vm->vm_thread->try_error_code = PIKA_RES_OK;
    vm->ins_cnt = 0;
    vm->in_super = pika_false;
    vm->super_invoke_deepth = 0;
    vm->in_repl = pika_false;
    stack_init(&(vm->stack));
    vm_frame_init_reg(vm);
    return vm;
}