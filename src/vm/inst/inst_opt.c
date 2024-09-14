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
#include "TinyObj.h"

typedef struct OperatorInfo OperatorInfo;
struct OperatorInfo {
    char *opt;
    ArgType t1;
    ArgType t2;
    Arg* a1;
    Arg* a2;
    pika_float f1;
    pika_float f2;
    int64_t i1;
    int64_t i2;
    Arg* res;
    uint32_t num;
    PikaVMFrame *vm;
};

int operatorInfo_init(OperatorInfo* info, PikaObj *self, PikaVMFrame *vm, char *data,
                      Arg *arg_ret_reg)
{
    if (info->a1 == NULL && info->a2 == NULL) {
        return -1;
    }
    info->opt = data;
    info->res = arg_ret_reg;
    if (info->a1 != NULL) {
        info->t1 = arg_getType(info->a1);
        if (info->t1 == ARG_TYPE_INT) {
            info->i1 = arg_getInt(info->a1);
            info->f1 = (pika_float)info->i1;
        } else if (info->t1 == ARG_TYPE_FLOAT) {
            info->f1 = arg_getFloat(info->a1);
            info->i1 = (int64_t)info->f1;
        } else if (info->t1 == ARG_TYPE_BOOL) {
            info->i1 = arg_getBool(info->a1);
            info->f1 = (pika_float)info->i1;
        }
    }
    info->t2 = arg_getType(info->a2);
    info->vm = vm;
    if (info->t2 == ARG_TYPE_INT) {
        info->i2 = arg_getInt(info->a2);
        info->f2 = (pika_float)info->i2;
    } else if (info->t2 == ARG_TYPE_FLOAT) {
        info->f2 = arg_getFloat(info->a2);
        info->i2 = (int64_t)info->f2;
    } else if (info->t2 == ARG_TYPE_BOOL) {
        info->i2 = arg_getBool(info->a2);
        info->f2 = (pika_float)info->i2;
    }
    return 0;
}

static Arg *_OPT_Method_ex(PikaObj *host, Arg *arg, OperatorInfo* op, char *method_name,
                           PIKA_RES err_no, char *errinfo)
{
    Arg *method = obj_getMethodArgWithFullPath(host, method_name);
    if (NULL == method) {
        vm_frame_set_error_code(op->vm, err_no);
        vm_frame_set_sys_out(op->vm, errinfo);
        return NULL;
    }
    Arg *res = obj_runMethodArg1(host, method, arg_copy(arg));
    return res;
}

static Arg *_OPT_Method(OperatorInfo* op, char *method_name, PIKA_RES err_no, char *errinfo)
{
    PikaObj *obj1 = arg_getPtr(op->a1);
    return _OPT_Method_ex(obj1, op->a2, op, method_name, err_no, errinfo);
}

static void _OPT_ADD(OperatorInfo* op)
{
#if !PIKA_NANO_ENABLE
    if (argType_isObject(op->t1)) {
        if (!argType_isObject(op->t2)) {
            vm_frame_set_error_code(op->vm, PIKA_RES_ERR_OPERATION_FAILED);
            vm_frame_set_sys_out(op->vm, "TypeError: unsupported operand +");
            op->res = NULL;
            return;
        }
        op->res = _OPT_Method(op, "__add__", PIKA_RES_ERR_OPERATION_FAILED,
                              "TypeError: unsupported operand +");
        return;
    }
#endif
    // Check if either argument is a string and the other is not a string
    if ((op->t1 == ARG_TYPE_STRING && op->t2 != ARG_TYPE_STRING) ||
        (op->t2 == ARG_TYPE_STRING && op->t1 != ARG_TYPE_STRING)) {
        vm_frame_set_error_code(op->vm, PIKA_RES_ERR_OPERATION_FAILED);
        vm_frame_set_sys_out(
            op->vm, "TypeError: unsupported operand + between str and non-str");
        op->res = NULL;
        return;
    }
    if ((op->t1 == ARG_TYPE_STRING) && (op->t2 == ARG_TYPE_STRING)) {
        char *num1_s = NULL;
        char *num2_s = NULL;
        Args str_opt_buffs = {0};
        num1_s = arg_getStr(op->a1);
        num2_s = arg_getStr(op->a2);
        char *opt_str_out = strsAppend(&str_opt_buffs, num1_s, num2_s);
        op->res = arg_setStr(op->res, "", opt_str_out);
        strsDeinit(&str_opt_buffs);
        return;
    }
#if !PIKA_NANO_ENABLE
    if ((op->t1 == ARG_TYPE_BYTES) && (op->t2 == ARG_TYPE_BYTES)) {
        uint8_t* bytes1 = arg_getBytes(op->a1);
        uint8_t* bytes2 = arg_getBytes(op->a2);
        size_t size1 = arg_getBytesSize(op->a1);
        size_t size2 = arg_getBytesSize(op->a2);
        op->res = arg_setBytes(op->res, "", NULL, size1 + size2);
        uint8_t* bytes_out = arg_getBytes(op->res);
        pika_platform_memcpy(bytes_out, bytes1, size1);
        pika_platform_memcpy(bytes_out + size1, bytes2, size2);
        return;
    }
#endif
    /* match float */
    if ((op->t1 == ARG_TYPE_FLOAT) || op->t2 == ARG_TYPE_FLOAT) {
        op->res = arg_setFloat(op->res, "", op->f1 + op->f2);
        return;
    }
    /* int is default */
    op->res = arg_setInt(op->res, "", op->i1 + op->i2);
    return;
}

static void _OPT_SUB(OperatorInfo* op)
{
#if !PIKA_NANO_ENABLE
    if (argType_isObject(op->t1)) {
        if (!argType_isObject(op->t2)) {
            vm_frame_set_error_code(op->vm, PIKA_RES_ERR_OPERATION_FAILED);
            vm_frame_set_sys_out(op->vm, "TypeError: unsupported operand +");
            op->res = NULL;
            return;
        }
        op->res = _OPT_Method(op, "__sub__", PIKA_RES_ERR_OPERATION_FAILED,
                              "TypeError: unsupported operand -");
        return;
    }
#endif
    if (op->t2 == ARG_TYPE_NONE) {
        if (op->t1 == ARG_TYPE_INT) {
            op->res = arg_setInt(op->res, "", -op->i1);
            return;
        }
        if (op->t1 == ARG_TYPE_FLOAT) {
            op->res = arg_setFloat(op->res, "", -op->f1);
            return;
        }
    }
    if ((op->t1 == ARG_TYPE_FLOAT) || op->t2 == ARG_TYPE_FLOAT) {
        op->res = arg_setFloat(op->res, "", op->f1 - op->f2);
        return;
    }
    op->res = arg_setInt(op->res, "", op->i1 - op->i2);
    return;
}

static void _OPT_EQU(OperatorInfo* op)
{
    int8_t is_equ = -1;
    if (op->t1 == ARG_TYPE_NONE && op->t2 == ARG_TYPE_NONE) {
        is_equ = 1;
        goto __exit;
    }
    /* type not equl, and type is not int or float or bool*/
    if (!argType_isEqual(op->t1, op->t2)) {
        if ((op->t1 != ARG_TYPE_FLOAT) && (op->t1 != ARG_TYPE_INT) &&
            (op->t1 != ARG_TYPE_BOOL)) {
            is_equ = 0;
            goto __exit;
        }
        if ((op->t2 != ARG_TYPE_FLOAT) && (op->t2 != ARG_TYPE_INT) &&
            (op->t2 != ARG_TYPE_BOOL)) {
            is_equ = 0;
            goto __exit;
        }
    }
    /* string compire */
    if (op->t1 == ARG_TYPE_STRING) {
        is_equ = strEqu(arg_getStr(op->a1), arg_getStr(op->a2));
        goto __exit;
    }
    /* bytes compire */
    if (op->t1 == ARG_TYPE_BYTES) {
        if (arg_getBytesSize(op->a1) != arg_getBytesSize(op->a2)) {
            is_equ = 0;
            goto __exit;
        }
        is_equ = 1;
        for (size_t i = 0; i < arg_getBytesSize(op->a1); i++) {
            if (arg_getBytes(op->a1)[i] != arg_getBytes(op->a2)[i]) {
                is_equ = 0;
                goto __exit;
            }
        }
        goto __exit;
    }
    if (argType_isCallable(op->t1) && argType_isCallable(op->t2)) {
        is_equ = (arg_getPtr(op->a1) == arg_getPtr(op->a2));
        goto __exit;
    }
    if (argType_isObject(op->t1) && argType_isObject(op->t2)) {
        is_equ = (arg_getPtr(op->a1) == arg_getPtr(op->a2));
        if (is_equ) {
            goto __exit;
        }
        Arg *__eq__ =
            obj_getMethodArgWithFullPath(arg_getPtr(op->a1), "__eq__");
        if (NULL == __eq__) {
            goto __exit;
        }
        arg_deinit(__eq__);
        Arg *res = _OPT_Method(op, "__eq__", PIKA_RES_ERR_OPERATION_FAILED,
                               "TypeError: unsupported operand ==");
        if (NULL == res) {
            is_equ = 0;
            goto __exit;
        }
        is_equ = arg_getBool(res);
        arg_deinit(res);
        goto __exit;
    }
    /* default: int bool, and float */
    is_equ = ((op->f1 - op->f2) * (op->f1 - op->f2) < (pika_float)0.000001);
    goto __exit;
__exit:
    if (op->opt[0] == '=') {
        op->res = arg_setBool(op->res, "", is_equ);
    } else {
        op->res = arg_setBool(op->res, "", !is_equ);
    }
    return;
}

static void _OPT_POW(OperatorInfo* op) {
    if (op->num == 1) {
        op->res = arg_copy(op->a2);
        arg_setIsDoubleStarred(op->res, 1);
        return;
    }
    if (op->t1 == ARG_TYPE_INT && op->t2 == ARG_TYPE_INT) {
        int lhs = op->i1;
        int rhs = op->i2;
        if (rhs < 0)
            rhs = -rhs;
        int64_t ret = 1;
        while (rhs) {
            if (rhs & 1)
                ret *= lhs;
            lhs *= lhs;
            rhs >>= 1;
        }
        if (op->i2 < 0) {
            op->res = arg_setFloat(op->res, "", 1.0 / ret);
        } else {
            op->res = arg_setInt(op->res, "", ret);
        }
        return;
    }
    if (op->t1 == ARG_TYPE_FLOAT && op->t2 == ARG_TYPE_INT) {
        float res = 1;
        for (int i = 0; i < op->i2; i++) {
            res = res * op->f1;
        }
        op->res = arg_setFloat(op->res, "", res);
        return;
    }
#if PIKA_MATH_ENABLE
    float res = 1;
    res = pow(op->f1, op->f2);
    op->res = arg_setFloat(op->res, "", res);
    return;
#else
    vm_frame_set_error_code(op->vm, PIKA_RES_ERR_OPERATION_FAILED);
    vm_frame_set_sys_out(op->vm,
                          "Operation float ** float is not enabled, please set "
                          "PIKA_MATH_ENABLE");
#endif
}

static void _OPT_MUL(OperatorInfo* op)
{
    if (op->num == 1) {
        op->res = arg_copy(op->a2);
        arg_setIsStarred(op->res, 1);
        return;
    }
    if ((op->t1 == ARG_TYPE_FLOAT) || op->t2 == ARG_TYPE_FLOAT) {
        op->res = arg_setFloat(op->res, "", op->f1 * op->f2);
        return;
    }
    if ((op->t1 == ARG_TYPE_STRING && op->t2 == ARG_TYPE_INT) ||
        (op->t1 == ARG_TYPE_INT && op->t2 == ARG_TYPE_STRING)) {
        char *str = NULL;
        int64_t num = 0;
        if (op->t1 == ARG_TYPE_STRING) {
            str = arg_getStr(op->a1);
            num = op->i2;
        } else {
            str = arg_getStr(op->a2);
            num = op->i1;
        }
        Args str_opt_buffs = {0};
        char *opt_str_out = strsRepeat(&str_opt_buffs, str, num);
        op->res = arg_setStr(op->res, "", opt_str_out);
        strsDeinit(&str_opt_buffs);
        return;
    }
    if ((op->t1 == ARG_TYPE_BYTES && op->t2 == ARG_TYPE_INT) ||
        (op->t1 == ARG_TYPE_INT && op->t2 == ARG_TYPE_BYTES)) {
        uint8_t* bytes = NULL;
        size_t size = 0;
        int64_t num = 0;
        if (op->t1 == ARG_TYPE_BYTES) {
            bytes = arg_getBytes(op->a1);
            size = arg_getBytesSize(op->a1);
            num = op->i2;
        } else {
            bytes = arg_getBytes(op->a2);
            size = arg_getBytesSize(op->a2);
            num = op->i1;
        }
        op->res = arg_setBytes(op->res, "", NULL, size * num);
        uint8_t* bytes_out = arg_getBytes(op->res);
        for (int i = 0; i < num; i++) {
            pika_platform_memcpy(bytes_out + i * size, bytes, size);
        }
        return;
    }
    if (argType_isObject(op->t1) || argType_isObject(op->t2)) {
        Arg *__mul__ = NULL;
        PikaObj *obj = NULL;
        Arg *arg = NULL;
        if (argType_isObject(op->t1)) {
            __mul__ =
                obj_getMethodArgWithFullPath(arg_getPtr(op->a1), "__mul__");
            obj = arg_getPtr(op->a1);
            arg = op->a2;
        }
        if (NULL == __mul__) {
            if (argType_isObject(op->t2)) {
                __mul__ =
                    obj_getMethodArgWithFullPath(arg_getPtr(op->a2), "__mul__");
                obj = arg_getPtr(op->a2);
                arg = op->a1;
            }
        }
        if (NULL == __mul__) {
            vm_frame_set_error_code(op->vm, PIKA_RES_ERR_OPERATION_FAILED);
            vm_frame_set_sys_out(op->vm, "TypeError: unsupported operand *");
            op->res = NULL;
            return;
        }
        op->res = obj_runMethodArg1(obj, __mul__, arg_copy(arg));
        return;
    }
    op->res = arg_setInt(op->res, "", op->i1 * op->i2);
    return;
}

Arg *vm_inst_handler_OPT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    OperatorInfo op = {0};
    op.num = vm_frame_get_input_arg_num(vm);
    arg_newReg(arg_reg1, PIKA_ARG_BUFF_SIZE);
    arg_newReg(arg_reg2, PIKA_ARG_BUFF_SIZE);
    if (op.num == 2) {
        /* tow input */
        op.a2 = stack_popArg(&(vm->stack), &arg_reg2);
        op.a1 = stack_popArg(&(vm->stack), &arg_reg1);
    } else if (op.num == 1) {
        /* only one input */
        op.a2 = stack_popArg(&(vm->stack), &arg_reg2);
        op.a1 = NULL;
    }
    /* init operator info */
    int ret = operatorInfo_init(&op, self, vm, data, arg_ret_reg);
    if (0 != ret) {
        vm_frame_set_sys_out(vm, PIKA_ERR_STRING_SYNTAX_ERROR);
        vm_frame_set_error_code(vm, PIKA_RES_ERR_SYNTAX_ERROR);
        return NULL;
    }
    switch (data[0]) {
        case '+':
            _OPT_ADD(&op);
            goto __exit;
        case '%':
            if ((op.t1 == ARG_TYPE_INT) && (op.t2 == ARG_TYPE_INT)) {
                op.res = arg_setInt(op.res, "", op.i1 % op.i2);
                goto __exit;
            }
#if PIKA_MATH_ENABLE
            if (op.t1 == ARG_TYPE_FLOAT || op.t2 == ARG_TYPE_FLOAT) {
                op.res = arg_setFloat(op.res, "", fmod(op.f1, op.f2));
                goto __exit;
            }
#endif
            vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
            vm_frame_set_sys_out(
                vm, "TypeError: unsupported operand type(s) for %%: 'float'");
            op.res = NULL;
            goto __exit;
        case '-':
            _OPT_SUB(&op);
            goto __exit;
    }
    if (data[1] == '=' && (data[0] == '!' || data[0] == '=')) {
        _OPT_EQU(&op);
        goto __exit;
    }
    if (data[1] == 0) {
        switch (data[0]) {
            case '<':
                op.res = arg_setBool(op.res, "", op.f1 < op.f2);
                goto __exit;
            case '*':
                _OPT_MUL(&op);
                goto __exit;
            case '&':
                if ((op.t1 == ARG_TYPE_INT) && (op.t2 == ARG_TYPE_INT)) {
                    op.res = arg_setInt(op.res, "", op.i1 & op.i2);
                    goto __exit;
                }
                vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
                vm_frame_set_sys_out(vm,
                                      "TypeError: unsupported operand "
                                      "type(s) for &: 'float'");
                op.res = NULL;
                goto __exit;
            case '|':
                if ((op.t1 == ARG_TYPE_INT) && (op.t2 == ARG_TYPE_INT)) {
                    op.res = arg_setInt(op.res, "", op.i1 | op.i2);
                    goto __exit;
                }
                vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
                vm_frame_set_sys_out(vm,
                                      "TypeError: unsupported operand "
                                      "type(s) for |: 'float'");
                op.res = NULL;
                goto __exit;
            case '~':
                if (op.t2 == ARG_TYPE_INT) {
                    op.res = arg_setInt(op.res, "", ~op.i2);
                    goto __exit;
                }
                vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
                vm_frame_set_sys_out(vm,
                                      "TypeError: unsupported operand "
                                      "type(s) for ~: 'float'");
                op.res = NULL;
                goto __exit;
            case '/':
                if (0 == op.f2) {
                    vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
                    vm_frame_set_sys_out(
                        vm, "ZeroDivisionError: division by zero");
                    op.res = NULL;
                    goto __exit;
                }
                op.res = arg_setFloat(op.res, "", op.f1 / op.f2);
                goto __exit;
            case '>':
                op.res = arg_setInt(op.res, "", op.f1 > op.f2);
                goto __exit;
            case '^':
                if ((op.t1 == ARG_TYPE_INT) && (op.t2 == ARG_TYPE_INT)) {
                    op.res = arg_setInt(op.res, "", op.i1 ^ op.i2);
                    goto __exit;
                }
                vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
                vm_frame_set_sys_out(vm,
                                      "TypeError: unsupported operand "
                                      "type(s) for ^: 'float'");
                op.res = NULL;
                goto __exit;
        }
    }
    if (data[1] == 'i' && data[2] == 'n') {
        if (op.t1 == ARG_TYPE_STRING && op.t2 == ARG_TYPE_STRING) {
            if (strstr(arg_getStr(op.a2), arg_getStr(op.a1))) {
                op.res = arg_setBool(op.res, "", pika_true);
            } else {
                op.res = arg_setBool(op.res, "", pika_false);
            }
            goto __exit;
        }
        if (op.t1 == ARG_TYPE_BYTES) {
            op.res = arg_setBool(op.res, "", _bytes_contains(op.a1, op.a2));
            goto __exit;
        }
#if !PIKA_NANO_ENABLE
        if (argType_isObject(op.t2)) {
            PikaObj *obj2 = arg_getPtr(op.a2);
            Arg *__contains__ =
                obj_getMethodArgWithFullPath(obj2, "__contains__");
            if (NULL != __contains__) {
                arg_deinit(__contains__);
                op.res = _OPT_Method_ex(obj2, op.a1, &op, "__contains__",
                                        PIKA_RES_ERR_OPERATION_FAILED,
                                        "TypeError: unsupported operand in");
                goto __exit;
            }
            PikaObj *local = New_TinyObj(NULL);
            obj_setArg(local, "@list", op.a2);
            obj_setArg(local, "@val", op.a1);
            /* clang-format off */
            PIKA_PYTHON(
            @res_contains = 0
            for @item in @list:
                if @item == @val:
                    @res_contains = 1
                    break
            )
            /* clang-format on */
            const uint8_t bytes[] = {
                0x48, 0x00, 0x00, 0x00, /* instruct array size */
                0x00, 0x85, 0x01, 0x00, 0x00, 0x04, 0x03, 0x00, 0x10, 0x81,
                0x11, 0x00, 0x00, 0x02, 0x17, 0x00, 0x00, 0x04, 0x1c, 0x00,
                0x00, 0x82, 0x20, 0x00, 0x00, 0x04, 0x2d, 0x00, 0x00, 0x0d,
                0x2d, 0x00, 0x00, 0x07, 0x33, 0x00, 0x11, 0x81, 0x2d, 0x00,
                0x11, 0x01, 0x35, 0x00, 0x01, 0x08, 0x3a, 0x00, 0x01, 0x07,
                0x3d, 0x00, 0x02, 0x85, 0x3d, 0x00, 0x02, 0x04, 0x03, 0x00,
                0x02, 0x8e, 0x00, 0x00, 0x00, 0x86, 0x3f, 0x00, 0x00, 0x8c,
                0x1c, 0x00,
                /* instruct array */
                0x42, 0x00, 0x00, 0x00, /* const pool size */
                0x00, 0x30, 0x00, 0x40, 0x72, 0x65, 0x73, 0x5f, 0x63, 0x6f,
                0x6e, 0x74, 0x61, 0x69, 0x6e, 0x73, 0x00, 0x40, 0x6c, 0x69,
                0x73, 0x74, 0x00, 0x69, 0x74, 0x65, 0x72, 0x00, 0x24, 0x6c,
                0x30, 0x00, 0x24, 0x6c, 0x30, 0x2e, 0x5f, 0x5f, 0x6e, 0x65,
                0x78, 0x74, 0x5f, 0x5f, 0x00, 0x40, 0x69, 0x74, 0x65, 0x6d,
                0x00, 0x32, 0x00, 0x40, 0x76, 0x61, 0x6c, 0x00, 0x3d, 0x3d,
                0x00, 0x31, 0x00, 0x2d, 0x31, 0x00, /* const pool */
            };
            pikaVM_runByteCode(local, (uint8_t*)bytes);
            op.res =
                arg_setBool(op.res, "", obj_getInt(local, "@res_contains"));
            obj_deinit(local);
            goto __exit;
        }
#endif

        vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
        vm_frame_set_sys_out(vm,
                              "Operation 'in' is not supported for this "
                              "type");
        op.res = NULL;
        goto __exit;
    }
    if (data[0] == '*' && data[1] == '*') {
        _OPT_POW(&op);
        goto __exit;
    }
    if (data[0] == '/' && data[1] == '/') {
        if ((op.t1 == ARG_TYPE_INT) && (op.t2 == ARG_TYPE_INT)) {
            op.res = arg_setInt(op.res, "", op.i1 / op.i2);
            goto __exit;
        }
#if PIKA_MATH_ENABLE
        if ((op.t1 == ARG_TYPE_FLOAT) || (op.t2 == ARG_TYPE_FLOAT)) {
            op.res = arg_setFloat(op.res, "", floor(op.f1 / op.f2));
            goto __exit;
        }
#endif
        vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
        vm_frame_set_sys_out(
            vm,
            "Operation float \\\\ float is not enabled, please set "
            "PIKA_MATH_ENABLE");
        op.res = NULL;
        goto __exit;
    }
    if (data[1] == 'i' && data[2] == 's') {
#if !PIKA_NANO_ENABLE
        if (argType_isObject(op.t1) && argType_isObject(op.t2)) {
            if (arg_getPtr(op.a1) == arg_getPtr(op.a2)) {
                op.res = arg_setBool(op.res, "", pika_true);
            } else {
                op.res = arg_setBool(op.res, "", pika_false);
            }
            goto __exit;
        }
        if ((op.t1 != op.t2) && (op.t1 != ARG_TYPE_NONE) &&
            (op.t2 != ARG_TYPE_NONE)) {
            op.res = arg_setInt(op.res, "", 0);
            goto __exit;
        }
#endif
        op.opt = "==";
        _OPT_EQU(&op);
        goto __exit;
    }
    if (data[0] == '>' && data[1] == '=') {
        op.res = arg_setBool(
            op.res, "",
            (op.f1 > op.f2) ||
                ((op.f1 - op.f2) * (op.f1 - op.f2) < (pika_float)0.000001));
        goto __exit;
    }
    if (data[0] == '<' && data[1] == '=') {
        op.res = arg_setBool(
            op.res, "",
            (op.f1 < op.f2) ||
                ((op.f1 - op.f2) * (op.f1 - op.f2) < (pika_float)0.000001));
        goto __exit;
    }
    if (data[0] == '>' && data[1] == '>') {
        if ((op.t1 == ARG_TYPE_INT) && (op.t2 == ARG_TYPE_INT)) {
            op.res = arg_setInt(op.res, "", op.i1 >> op.i2);
            goto __exit;
        }
        vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
        vm_frame_set_sys_out(
            vm, "TypeError: unsupported operand type(s) for >>: 'float'");
        op.res = NULL;
        goto __exit;
    }
    if (data[0] == '<' && data[1] == '<') {
        if ((op.t1 == ARG_TYPE_INT) && (op.t2 == ARG_TYPE_INT)) {
            op.res = arg_setInt(op.res, "", op.i1 << op.i2);
            goto __exit;
        }
        vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
        vm_frame_set_sys_out(
            vm, "TypeError: unsupported operand type(s) for <<: 'float'");
        op.res = NULL;
        goto __exit;
    }
    if (data[0] == ' ' && data[1] == 'a' && data[2] == 'n' && data[3] == 'd' &&
        data[4] == ' ') {
        op.res = arg_setBool(op.res, "", op.i1 && op.i2);
        goto __exit;
    }
    if (data[0] == ' ' && data[1] == 'o' && data[2] == 'r' && data[3] == ' ' &&
        data[4] == 0) {
        op.res = arg_setBool(op.res, "", op.i1 || op.i2);
        goto __exit;
    }
    if (data[0] == ' ' && data[1] == 'n' && data[2] == 'o' && data[3] == 't' &&
        data[4] == ' ' && data[5] == 0) {
        pika_bool bTrue = pika_false;
        _transeBool(op.a2, &bTrue);
        op.res = arg_setBool(op.res, "", !bTrue);
        goto __exit;
    }
    vm_frame_set_sys_out(vm, "Error: unknown operator '%s'", data);
    vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
__exit:
    if (NULL != op.a1) {
        arg_deinit(op.a1);
    }
    if (NULL != op.a2) {
        arg_deinit(op.a2);
    }
    if (NULL != op.res) {
        return op.res;
    }
    return NULL;
}