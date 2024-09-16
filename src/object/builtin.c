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
#include "builtin.h"
#include "PikaObj.h"
#include "PikaVM.h"
#include "gc.h"

PIKA_RES _transeInt(Arg* arg, int base, int64_t* res);

void builtins_remove(PikaObj* self, char* argPath) {
    obj_setErrorCode(self, 0);
    int32_t res = obj_removeArg(self, argPath);
    if (1 == res) {
        obj_setSysOut(self, "[error] del: object no found.");
        obj_setErrorCode(self, 1);
        return;
    }
    if (2 == res) {
        obj_setSysOut(self, "[error] del: arg not match.");
        obj_setErrorCode(self, 2);
        return;
    }
}

Arg* _type(Arg* arg);
Arg* builtins_type(PikaObj* self, Arg* arg) {
#if PIKA_NANO_ENABLE
    pika_platform_printf("PIKA_NANO_ENABLE is not enable");
    return NULL;
#else
    if (NULL == arg) {
        obj_setSysOut(self, "[error] type: arg no found.");
        obj_setErrorCode(self, 1);
        return NULL;
    }
    return _type(arg);
#endif
}

pika_float builtins_float(PikaObj* self, Arg* arg) {
    ArgType type = arg_getType(arg);
    if (ARG_TYPE_INT == type) {
        return (pika_float)arg_getInt(arg);
    }
    if (ARG_TYPE_FLOAT == type) {
        return (pika_float)arg_getFloat(arg);
    }
    if (ARG_TYPE_STRING == type) {
        return strtod(arg_getStr(arg), NULL);
    }
    if (ARG_TYPE_BOOL == type) {
        return (pika_float)arg_getBool(arg);
    }
    obj_setSysOut(self, "[error] convert to pika_float type failed.");
    obj_setErrorCode(self, 1);
    return _PIKA_FLOAT_ERR;
}

Arg* builtins_int(PikaObj* self, Arg* arg, PikaTuple* base) {
    int64_t res = 0;
    int64_t iBase = 10;
    if (pikaTuple_getSize(base) > 0) {
        if (arg_getType(arg) != ARG_TYPE_STRING &&
            arg_getType(arg) != ARG_TYPE_BYTES) {
            obj_setSysOut(self,
                          "TypeError: int() can't convert non-string with "
                          "explicit base");
            obj_setErrorCode(self, 1);
            return NULL;
        }
        iBase = (int64_t)pikaTuple_getInt(base, 0);
    }
    if (_transeInt(arg, iBase, &res) == PIKA_RES_OK) {
        return arg_newInt(res);
    }
    obj_setSysOut(self, "ValueError: invalid literal for int(): '%s'",
                  arg_getStr(arg));
    obj_setErrorCode(self, 1);
    return NULL;
}

pika_bool builtins_bool(PikaObj* self, Arg* arg) {
    pika_bool res = 0;
    if (_transeBool(arg, &res) == PIKA_RES_OK) {
        return res;
    }
    obj_setSysOut(self, "ValueError: invalid literal for bool()");
    obj_setErrorCode(self, 1);
    return _PIKA_BOOL_ERR;
}

char* builtins_str(PikaObj* self, Arg* arg) {
    pika_assert(NULL != arg);
    pika_assert(NULL != self);
    // if (arg_getType(arg) == ARG_TYPE_BYTES) {
    //     return obj_cacheStr(self, (char*)arg_getBytes(arg));
    // }
    Arg* arg_str = arg_toStrArg(arg);
    if (NULL == arg_str) {
        obj_setSysOut(self, "Error: convert to str type failed.");
        obj_setErrorCode(self, 1);
        return NULL;
    }
    char* str = obj_cacheStr(self, arg_getStr(arg_str));
    arg_deinit(arg_str);
    return str;
}

PikaObj* New_builtins_RangeObj(Args* args);
Arg* builtins_iter(PikaObj* self, Arg* arg) {
    pika_assert(NULL != arg);
    /* object */
    pika_bool bIsTemp = pika_false;
    PikaObj* oArg = _arg_to_obj(arg, &bIsTemp);
    pika_assert(NULL != oArg);
    NewFun _clsptr = (NewFun)oArg->constructor;
    if (_clsptr == New_builtins_RangeObj) {
        /* found RangeObj, return directly */
        return arg_copy(arg);
    }
    /* clang-format off */
    PIKA_PYTHON(
    @res_iter = __iter__()
    )
    /* clang-format on */
    const uint8_t bytes[] = {
        0x08, 0x00, 0x00, 0x00, /* instruct array size */
        0x00, 0x82, 0x01, 0x00, 0x00, 0x04, 0x0a, 0x00, /* instruct array */
        0x14, 0x00, 0x00, 0x00,                         /* const pool size */
        0x00, 0x5f, 0x5f, 0x69, 0x74, 0x65, 0x72, 0x5f, 0x5f, 0x00, 0x40,
        0x72, 0x65, 0x73, 0x5f, 0x69, 0x74, 0x65, 0x72, 0x00, /* const pool */
    };
    Arg* res = pikaVM_runByteCodeReturn(oArg, (uint8_t*)bytes, "@res_iter");
    if (bIsTemp) {
        obj_refcntDec(oArg);
    }
    return res;
}

Arg* builtins_range(PikaObj* self, PikaTuple* ax) {
    /* set template arg to create rangeObj */
    Arg* aRangeObj = arg_newDirectObj(New_builtins_RangeObj);
    PikaObj* oRangeObj = arg_getPtr(aRangeObj);
    RangeData tRangeData = {0};
    if (pikaTuple_getSize(ax) == 1) {
        int start = 0;
        int end = arg_getInt(pikaTuple_getArg(ax, 0));
        tRangeData.start = start;
        tRangeData.end = end;
        tRangeData.step = 1;
    } else if (pikaTuple_getSize(ax) == 2) {
        int start = arg_getInt(pikaTuple_getArg(ax, 0));
        int end = arg_getInt(pikaTuple_getArg(ax, 1));
        tRangeData.start = start;
        tRangeData.end = end;
        tRangeData.step = 1;
    } else if (pikaTuple_getSize(ax) == 3) {
        int start = arg_getInt(pikaTuple_getArg(ax, 0));
        int end = arg_getInt(pikaTuple_getArg(ax, 1));
        int step = arg_getInt(pikaTuple_getArg(ax, 2));
        tRangeData.start = start;
        tRangeData.end = end;
        tRangeData.step = step;
    }
    tRangeData.i = tRangeData.start;
    obj_setStruct(oRangeObj, "_", tRangeData);
    return aRangeObj;
}

Arg* builtins___getitem__(PikaObj* self, Arg* obj, Arg* key) {
    return _vm_get(NULL, self, key, obj);
}

Arg* builtins___setitem__(PikaObj* self, Arg* obj, Arg* key, Arg* val) {
    ArgType obj_type = arg_getType(obj);
    if (ARG_TYPE_STRING == obj_type) {
        int index = arg_getInt(key);
        char* str_val = arg_getStr(val);
        char* str_pyload = arg_getStr(obj);
        str_pyload[index] = str_val[0];
        return arg_newStr(str_pyload);
    }
    if (ARG_TYPE_BYTES == obj_type) {
        int index = arg_getInt(key);
        uint8_t byte_val = 0;
        if (ARG_TYPE_BYTES == arg_getType(val)) {
            uint8_t* bytes_val = arg_getBytes(val);
            byte_val = bytes_val[0];
        }
        if (ARG_TYPE_INT == arg_getType(val)) {
            byte_val = arg_getInt(val);
        }
        uint8_t* bytes_pyload = arg_getBytes(obj);
        size_t bytes_len = arg_getBytesSize(obj);
        bytes_pyload[index] = byte_val;
        return arg_newBytes(bytes_pyload, bytes_len);
    }
    if (argType_isObject(obj_type)) {
        PikaObj* arg_obj = arg_getPtr(obj);
        obj_setArg(arg_obj, "__key", key);
        obj_setArg(arg_obj, "__val", val);
        /* clang-format off */
        PIKA_PYTHON(
        __setitem__(__key, __val)
        )
        /* clang-format on */
        const uint8_t bytes[] = {
            0x0c, 0x00, 0x00, 0x00, /* instruct array size */
            0x10, 0x81, 0x01, 0x00, 0x10, 0x01, 0x07, 0x00, 0x00, 0x02, 0x0d,
            0x00,
            /* instruct array */
            0x19, 0x00, 0x00, 0x00, /* const pool size */
            0x00, 0x5f, 0x5f, 0x6b, 0x65, 0x79, 0x00, 0x5f, 0x5f, 0x76, 0x61,
            0x6c, 0x00, 0x5f, 0x5f, 0x73, 0x65, 0x74, 0x69, 0x74, 0x65, 0x6d,
            0x5f, 0x5f, 0x00,
            /* const pool */
        };
        pikaVM_runByteCode(arg_obj, (uint8_t*)bytes);
        return arg_newRef(arg_obj);
    }
    return NULL;
}

int builtins_len(PikaObj* self, Arg* arg) {
    if (ARG_TYPE_STRING == arg_getType(arg)) {
        return strGetSize(arg_getStr(arg));
    }
    if (ARG_TYPE_BYTES == arg_getType(arg)) {
        return arg_getBytesSize(arg);
    }

    if (arg_isObject(arg)) {
        PikaObj* arg_obj = arg_getPtr(arg);
        return obj_getSize(arg_obj);
    }

    obj_setErrorCode(self, 1);
    __platform_printf("[Error] len: arg type not support\r\n");
    return -1;
}

Arg* builtins_list(PikaObj* self, PikaTuple* val) {
#if PIKA_BUILTIN_STRUCT_ENABLE
    if (1 == pikaTuple_getSize(val)) {
        Arg* aInput = pikaTuple_getArg(val, 0);
        if (!arg_isIterable(aInput)) {
            obj_setErrorCode(self, 1);
            obj_setSysOut(self, "Error: input arg must be iterable");
            return NULL;
        }
        obj_setArg(self, "__list", aInput);
        /* clang-format off */
        PIKA_PYTHON(
        @res_list = []
        for __item in __list:
            @res_list.append(__item)
        del __item
        del __list

        )
        /* clang-format on */
        const uint8_t bytes[] = {
            0x40, 0x00, 0x00, 0x00, /* instruct array size */
            0x00, 0x95, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x04, 0x01,
            0x00, 0x10, 0x81, 0x0b, 0x00, 0x00, 0x02, 0x12, 0x00, 0x00, 0x04,
            0x17, 0x00, 0x00, 0x82, 0x1b, 0x00, 0x00, 0x04, 0x28, 0x00, 0x00,
            0x0d, 0x28, 0x00, 0x00, 0x07, 0x2f, 0x00, 0x11, 0x81, 0x28, 0x00,
            0x01, 0x02, 0x31, 0x00, 0x00, 0x86, 0x42, 0x00, 0x00, 0x8c, 0x17,
            0x00, 0x00, 0x8c, 0x28, 0x00, 0x00, 0x8c, 0x0b, 0x00, /* instruct
                                                                     array */
            0x45, 0x00, 0x00, 0x00, /* const pool size */
            0x00, 0x40, 0x72, 0x65, 0x73, 0x5f, 0x6c, 0x69, 0x73, 0x74, 0x00,
            0x5f, 0x5f, 0x6c, 0x69, 0x73, 0x74, 0x00, 0x69, 0x74, 0x65, 0x72,
            0x00, 0x24, 0x6c, 0x30, 0x00, 0x24, 0x6c, 0x30, 0x2e, 0x5f, 0x5f,
            0x6e, 0x65, 0x78, 0x74, 0x5f, 0x5f, 0x00, 0x5f, 0x5f, 0x69, 0x74,
            0x65, 0x6d, 0x00, 0x32, 0x00, 0x40, 0x72, 0x65, 0x73, 0x5f, 0x6c,
            0x69, 0x73, 0x74, 0x2e, 0x61, 0x70, 0x70, 0x65, 0x6e, 0x64, 0x00,
            0x2d, 0x31, 0x00, /* const pool */
        };
        return pikaVM_runByteCodeReturn(self, (uint8_t*)bytes, "@res_list");
    }
    return arg_newObj(New_pikaListFrom(NULL));
#else
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "[Error] built-in list is not enabled.");
    return arg_newNull();
#endif
}

Arg* builtins_dict(PikaObj* self, PikaTuple* val) {
#if PIKA_BUILTIN_STRUCT_ENABLE
    return arg_newObj(New_PikaDict());
#else
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "[Error] built-in dist is not enabled.");
    return arg_newNull();
#endif
}

PikaObj *New_PikaStdData_Tuple(Args* args);
Arg* builtins_tuple(PikaObj* self, PikaTuple* val) {
#if PIKA_BUILTIN_STRUCT_ENABLE
    Arg* tuple = builtins_list(self, val);
    if (NULL == tuple) {
        return NULL;
    }
    PikaObj* oTuple = arg_getObj(tuple);
    oTuple->constructor = New_PikaStdData_Tuple;
    return tuple;
#else
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "[Error] built-in tuple is not enabled.");
    return arg_newNull();
#endif
}

char* builtins_hex(PikaObj* self, int val) {
    char buff[PIKA_SPRINTF_BUFF_SIZE] = {0};
    if (val >= 0) {
        __platform_sprintf(buff, "0x%02x", val);
    } else {
        __platform_sprintf(buff, "-0x%02x", -val);
    }
    /* load the string from stack to heap */
    obj_setStr(self, "__buf", buff);
    return obj_getStr(self, "__buf");
}

int builtins_ord(PikaObj* self, char* val) {
    return (int)val[0];
}

char* builtins_chr(PikaObj* self, int val) {
    char buff[PIKA_SPRINTF_BUFF_SIZE] = {0};
    char to_str[] = "0";
    to_str[0] = val;
    __platform_sprintf(buff, "%s", to_str);
    /* load the string from stack to heap */
    obj_setStr(self, "__buf", buff);
    return obj_getStr(self, "__buf");
}

Arg* builtins_bytes(PikaObj* self, Arg* val) {
    ArgType type = arg_getType(val);
    if (ARG_TYPE_INT == type) {
        int size = arg_getInt(val);
        /* src is NULL so the bytes are all '\0' */
        Arg* bytes = arg_newBytes(NULL, size);
        return bytes;
    }
    if (ARG_TYPE_BYTES == type) {
        return arg_copy(val);
    }
    if (ARG_TYPE_STRING == type) {
        int size = strGetSize(arg_getStr(val));
        Arg* bytes = arg_newBytes((uint8_t*)arg_getStr(val), size);
        return bytes;
    }
#if !PIKA_NANO_ENABLE
    if (argType_isObject(type)) {
        PikaObj* obj = arg_getPtr(val);
        PikaObj* New_PikaStdData_List(Args * args);
        PikaObj* New_PikaStdData_Tuple(Args * args);
        if (obj->constructor == New_PikaStdData_List ||
            obj->constructor == New_PikaStdData_Tuple) {
            Arg* bytes = arg_newBytes(NULL, pikaList_getSize(obj));
            uint8_t* bytes_raw = arg_getBytes(bytes);
            for (size_t i = 0; i < pikaList_getSize(obj); i++) {
                bytes_raw[i] = (uint8_t)pikaList_getInt(obj, i);
            }
            return bytes;
        }
    }
#endif
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "Error: input arg type not supported.");
    return arg_newNone();
}

void builtins_print(PikaObj* self, PikaTuple* val, PikaDict* ops) {
    int arg_size = pikaTuple_getSize(val);
    pika_assert(arg_size >= 0);
    char* end = pikaDict_getStr(ops, "end");
    if (NULL == end) {
        /* default */
        end = "\r\n";
    }
    if (arg_size == 1) {
        arg_print(pikaTuple_getArg(val, 0), pika_false, end);
        return;
    }
    Arg* print_out_arg = NULL;
    pika_bool is_get_print = pika_false;
    for (int i = 0; i < arg_size; i++) {
        Arg* arg = pikaTuple_getArg(val, i);
        Arg* item_arg_str = arg_toStrArg(arg);
        if (NULL != item_arg_str) {
            is_get_print = PIKA_TRUE;
            if (NULL == print_out_arg) {
                print_out_arg = arg_newStr("");
            }
            print_out_arg =
                arg_strAppend(print_out_arg, arg_getStr(item_arg_str));
            if (i < arg_size - 1) {
                print_out_arg = arg_strAppend(print_out_arg, " ");
            }
            arg_deinit(item_arg_str);
        }
    }
    if (PIKA_TRUE == is_get_print) {
        __platform_printf("%s%s", arg_getStr(print_out_arg), end);
    }
    if (NULL != print_out_arg) {
        arg_deinit(print_out_arg);
    }
}

char* builtins_cformat(PikaObj* self, char* fmt, PikaTuple* var) {
#if PIKA_SYNTAX_FORMAT_ENABLE
    Args buffs = {0};
    pikaMemMaxReset();
    char* res = strsFormatList(&buffs, fmt, var);
    obj_setStr(self, "_buf", res);
    res = obj_getStr(self, "_buf");
    strsDeinit(&buffs);
    return res;
#else
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "[Error] PIKA_SYNTAX_FORMAT_ENABLE is not enabled.");
    return NULL;
#endif
}

int builtins_id(PikaObj* self, Arg* obj) {
    uintptr_t ptr = 0;
    if (arg_isObject(obj)) {
        ptr = (uintptr_t)arg_getPtr(obj);
    } else {
        ptr = (uintptr_t)obj;
    }
    return ptr & (0x7FFFFFFF);
}

int PikaStdData_FILEIO_init(PikaObj* self, char* path, char* mode);
PikaObj* New_PikaStdData_FILEIO(Args* args);
PikaObj* builtins_open(PikaObj* self, char* path, char* mode) {
#if PIKA_FILEIO_ENABLE
    PikaObj* file = newNormalObj(New_PikaStdData_FILEIO);
    if (0 != PikaStdData_FILEIO_init(file, path, mode)) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "[Error] open: can not open file.");
        obj_deinit(file);
        return NULL;
    }
    return file;
#else
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "[Error] PIKA_FILEIO_ENABLE is not enabled.");
    return NULL;
#endif
}

/* __dir_each */
int32_t __dir_each(Arg* argEach, void* context) {
    if (argType_isCallable(arg_getType(argEach))) {
        char name_buff[PIKA_LINE_BUFF_SIZE] = {0};
        char* method_name =
            methodArg_getName(argEach, name_buff, sizeof(name_buff));
        Arg* arg_str = arg_newStr(method_name);
        PikaList* list = args_getPtr(context, "_list");
        pikaList_append(list, arg_str);
    }
    return 0;
}

PikaObj* builtins_dir(PikaObj* self, Arg* arg) {
    if (!arg_isObject(arg)) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "[Error] dir: not support type.");
        return NULL;
    }
    PikaObj* obj = arg_getPtr(arg);
    PikaObj* New_PikaStdData_List(Args * args);
    PikaObj* list = New_PikaList();
    Args* context = New_args(NULL);
    args_setPtr(context, "_list", list);
    args_foreach(obj->list, __dir_each, context);
    args_deinit(context);
    return list;
}

void builtins_exec(PikaObj* self, char* code) {
#if PIKA_EXEC_ENABLE
    obj_run(self, code);
#else
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "[Error] PIKA_EXEC_ENABLE is not enabled.");
#endif
}

Arg* builtins_getattr(PikaObj* self, PikaObj* obj, char* name) {
    if (NULL == obj) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "[Error] getattr: can not get attr of NULL.");
        return NULL;
    }
    Arg* aRes = obj_getArg(obj, name);
    if (NULL == aRes) {
        aRes = obj_getMethodArgWithFullPath(obj, name);
    }
    if (NULL != aRes) {
        aRes = methodArg_setHostObj(aRes, obj);
        if (arg_getType(aRes) != ARG_TYPE_METHOD_NATIVE_ACTIVE) {
            aRes = arg_copy(aRes);
        }
    }
    return aRes;
}

void builtins_setattr(PikaObj* self, PikaObj* obj, char* name, Arg* val) {
    if (NULL == obj) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "[Error] setattr: obj is null.");
        goto __exit;
    }
    obj_setArg(obj, name, val);
__exit:
    return;
}

void builtins_exit(PikaObj* self) {
    pika_vm_exit();
}

int builtins_hasattr(PikaObj* self, PikaObj* obj, char* name) {
    if (NULL == obj) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "[Error] hasattr: obj is null.");
        return 0;
    }
    if (obj_isArgExist(obj, name)) {
        return 1;
    }
    Arg* method = obj_getMethodArgWithFullPath(obj, name);
    if (NULL != method) {
        arg_deinit(method);
        return 1;
    }
    return 0;
}

Arg* builtins_eval(PikaObj* self, char* code) {
    Args buffs = {0};
    char* cmd = strsAppend(&buffs, "@res = ", code);
    obj_run(self, cmd);
    Arg* res = arg_copy(obj_getArg(self, "@res"));
    strsDeinit(&buffs);
    obj_removeArg(self, "@res");
    return res;
}

static enum shellCTRL __obj_shellLineHandler_input(PikaObj* self,
                                                   char* input_line,
                                                   struct ShellConfig* cfg) {
    cfg->context = arg_newStr(input_line);
    return SHELL_CTRL_EXIT;
}

char* builtins_input(PikaObj* self, PikaTuple* info) {
    struct ShellConfig cfg = {
        .prefix = "",
        .context = NULL,
        .handler = __obj_shellLineHandler_input,
        .fn_getchar = __platform_getchar,
    };
    if (pikaTuple_getSize(info) > 0) {
        __platform_printf("%s", pikaTuple_getStr(info, 0));
    }
    _temp__do_pikaScriptShell(self, &cfg);
    char* res = obj_cacheStr(self, arg_getStr(cfg.context));
    arg_deinit(cfg.context);
    return res;
}

extern volatile PikaObj* __pikaMain;
void builtins_help(PikaObj* self, char* name) {
    if (strEqu(name, "modules")) {
        obj_printModules((PikaObj*)__pikaMain);
        return;
    }
    pika_platform_printf(
        "Warning: help() only support help('modules') now.\r\n");
}

void builtins_reboot(PikaObj* self) {
    pika_platform_reboot();
}

void builtins_clear(PikaObj* self) {
    pika_platform_clear();
}

void builtins_gcdump(PikaObj* self) {
    pikaGC_markDump();
}

Arg* builtins_abs(PikaObj* self, Arg* val) {
    ArgType type = arg_getType(val);
    if (type == ARG_TYPE_INT) {
        int64_t v = arg_getInt(val);
        if (v < 0) {
            v = -v;
        }
        return arg_newInt(v);
    }
    if (type == ARG_TYPE_FLOAT) {
        pika_float v = arg_getFloat(val);
        if (v < 0) {
            v = -v;
        }
        return arg_newFloat(v);
    }
    obj_setSysOut(self, "TypeError: bad operand type for abs()");
    obj_setErrorCode(self, PIKA_RES_ERR_INVALID_PARAM);
    return NULL;
}

static Arg* _max_min(PikaObj* self, PikaTuple* val, uint8_t* bc) {
    int size = pikaTuple_getSize(val);
    if (size == 0) {
        obj_setSysOut(self, "TypeError: max expected 1 arguments, got 0");
        obj_setErrorCode(self, PIKA_RES_ERR_INVALID_PARAM);
        return NULL;
    }
    if (size == 1) {
        ArgType type = arg_getType(pikaTuple_getArg(val, 0));
        if (!argType_isIterable(type)) {
            obj_setSysOut(self, "TypeError: object is not iterable");
            obj_setErrorCode(self, PIKA_RES_ERR_INVALID_PARAM);
            return NULL;
        }
        obj_setArg(self, "@list", pikaTuple_getArg(val, 0));
        return pikaVM_runByteCodeReturn(self, (uint8_t*)bc, "@res_max");
    }
    PikaTuple* oTuple = newNormalObj(New_PikaStdData_Tuple);
    obj_setPtr(oTuple, "list", obj_getPtr(val, "list"));
    obj_setInt(oTuple, "needfree", 0);
    Arg* aTuple = arg_newObj(oTuple);
    obj_setArg(self, "@list", aTuple);
    Arg* aRet = pikaVM_runByteCodeReturn(self, (uint8_t*)bc, "@res_max");
    arg_deinit(aTuple);
    return aRet;
}

const uint8_t bc_max[] = {
    0x4c, 0x00, 0x00, 0x00, /* instruct array size */
    0x10, 0x81, 0x01, 0x00, 0x10, 0x05, 0x07, 0x00, 0x00, 0x1d, 0x00, 0x00,
    0x00, 0x04, 0x09, 0x00, 0x10, 0x81, 0x01, 0x00, 0x00, 0x02, 0x12, 0x00,
    0x00, 0x04, 0x17, 0x00, 0x00, 0x82, 0x1b, 0x00, 0x00, 0x04, 0x28, 0x00,
    0x00, 0x0d, 0x28, 0x00, 0x00, 0x07, 0x2e, 0x00, 0x11, 0x81, 0x28, 0x00,
    0x11, 0x01, 0x09, 0x00, 0x01, 0x08, 0x30, 0x00, 0x01, 0x07, 0x32, 0x00,
    0x02, 0x81, 0x28, 0x00, 0x02, 0x04, 0x09, 0x00, 0x00, 0x86, 0x34, 0x00,
    0x00, 0x8c, 0x17, 0x00, /* instruct array */
    0x37, 0x00, 0x00, 0x00, /* const pool size */
    0x00, 0x40, 0x6c, 0x69, 0x73, 0x74, 0x00, 0x30, 0x00, 0x40, 0x72, 0x65,
    0x73, 0x5f, 0x6d, 0x61, 0x78, 0x00, 0x69, 0x74, 0x65, 0x72, 0x00, 0x24,
    0x6c, 0x30, 0x00, 0x24, 0x6c, 0x30, 0x2e, 0x5f, 0x5f, 0x6e, 0x65, 0x78,
    0x74, 0x5f, 0x5f, 0x00, 0x40, 0x69, 0x74, 0x65, 0x6d, 0x00, 0x32, 0x00,
    0x3e, 0x00, 0x31, 0x00, 0x2d, 0x31, 0x00, /* const pool */
};

const uint8_t bc_min[] = {
    0x4c, 0x00, 0x00, 0x00, /* instruct array size */
    0x10, 0x81, 0x01, 0x00, 0x10, 0x05, 0x07, 0x00, 0x00, 0x1d, 0x00, 0x00,
    0x00, 0x04, 0x09, 0x00, 0x10, 0x81, 0x01, 0x00, 0x00, 0x02, 0x12, 0x00,
    0x00, 0x04, 0x17, 0x00, 0x00, 0x82, 0x1b, 0x00, 0x00, 0x04, 0x28, 0x00,
    0x00, 0x0d, 0x28, 0x00, 0x00, 0x07, 0x2e, 0x00, 0x11, 0x81, 0x28, 0x00,
    0x11, 0x01, 0x09, 0x00, 0x01, 0x08, 0x30, 0x00, 0x01, 0x07, 0x32, 0x00,
    0x02, 0x81, 0x28, 0x00, 0x02, 0x04, 0x09, 0x00, 0x00, 0x86, 0x34, 0x00,
    0x00, 0x8c, 0x17, 0x00, /* instruct array */
    0x37, 0x00, 0x00, 0x00, /* const pool size */
    0x00, 0x40, 0x6c, 0x69, 0x73, 0x74, 0x00, 0x30, 0x00, 0x40, 0x72, 0x65,
    0x73, 0x5f, 0x6d, 0x61, 0x78, 0x00, 0x69, 0x74, 0x65, 0x72, 0x00, 0x24,
    0x6c, 0x30, 0x00, 0x24, 0x6c, 0x30, 0x2e, 0x5f, 0x5f, 0x6e, 0x65, 0x78,
    0x74, 0x5f, 0x5f, 0x00, 0x40, 0x69, 0x74, 0x65, 0x6d, 0x00, 0x32, 0x00,
    0x3c, 0x00, 0x31, 0x00, 0x2d, 0x31, 0x00, /* const pool */
};

Arg* builtins_max(PikaObj* self, PikaTuple* val) {
    return _max_min(self, val, (uint8_t*)bc_max);
}

Arg* builtins_min(PikaObj* self, PikaTuple* val) {
    return _max_min(self, val, (uint8_t*)bc_min);
}

extern NativeProperty* methodArg_toProp(Arg* method_arg);

static pika_bool _isinstance(Arg* aObj, Arg* classinfo) {
    pika_bool res = pika_false;
    Arg* aObjType = NULL;
    aObjType = _type(aObj);
    NativeProperty* objProp = NULL;
    while (1) {
        if (arg_getPtr(aObjType) == arg_getPtr(classinfo)) {
            res = pika_true;
            goto __exit;
        }
        aObjType = methodArg_super(aObjType, &objProp);
        if (NULL != objProp) {
            if (!(arg_getType(classinfo) ==
                  ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR)) {
                res = pika_false;
                goto __exit;
            }
            NativeProperty* classProp = methodArg_toProp(classinfo);
            while (1) {
                if (objProp == classProp) {
                    res = pika_true;
                    goto __exit;
                }
                if (objProp->super == NULL) {
                    res = pika_false;
                    goto __exit;
                }
                objProp = (NativeProperty*)objProp->super;
            }
        }
        if (NULL == aObjType) {
            res = pika_false;
            goto __exit;
        }
    }
__exit:
    if (NULL != aObjType) {
        arg_deinit(aObjType);
    }
    return res;
}

pika_bool builtins_isinstance(PikaObj* self, Arg* object, Arg* classinfo) {
    if (!argType_isConstructor(arg_getType(classinfo)) &&
        !argType_isCallable(arg_getType(classinfo))) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "TypeError: isinstance() arg 2 must be a type");
        return pika_false;
    }
    return _isinstance(object, classinfo);
}

Arg* builtins_StringObj___next__(PikaObj* self) {
    return arg_newNone();
}

Arg* builtins_RangeObj___next__(PikaObj* self) {
    RangeData* _ = (RangeData*)args_getStruct(self->list, "_");
    int end = _->end;
    int step = _->step;
    /* exit */
    if (_->i >= end) {
        return arg_newNone();
    }
    Arg* res = arg_newInt(_->i);
    _->i += step;
    return res;
}

PikaObj* New_builtins(Args* args);
PikaObj* obj_getBuiltins(void) {
    return newNormalObj(New_builtins);
}

void builtins_bytearray___init__(PikaObj* self, Arg* bytes) {
    obj_setArg_noCopy(self, "raw", builtins_bytes(self, bytes));
}

Arg* builtins_bytearray___iter__(PikaObj* self) {
    obj_setInt(self, "__iter_i", 0);
    return arg_newRef(self);
}

int builtins_bytearray___len__(PikaObj* self) {
    return obj_getBytesSize(self, "raw");
}

Arg* builtins_bytearray___next__(PikaObj* self) {
    int __iter_i = args_getInt(self->list, "__iter_i");
    uint8_t* data = obj_getBytes(self, "raw");
    uint16_t len = obj_getBytesSize(self, "raw");
    Arg* res = NULL;
    char char_buff[] = " ";
    if (__iter_i < len) {
        char_buff[0] = data[__iter_i];
        res = arg_newInt(char_buff[0]);
    } else {
        return arg_newNone();
    }
    args_setInt(self->list, "__iter_i", __iter_i + 1);
    return res;
}

int builtins_bytearray___getitem__(PikaObj* self, int __key) {
    uint8_t* data = obj_getBytes(self, "raw");
    uint16_t len = obj_getBytesSize(self, "raw");
    if (__key < len) {
        return data[__key];
    } else {
        return 0;
    }
}

int builtins_bytearray___contains__(PikaObj* self, Arg* others) {
    if (arg_isObject(others)) {
        others = obj_getArg(arg_getObj(others), "raw");
    }
    return _bytes_contains(others, obj_getArg(self, "raw"));
}

void builtins_bytearray___setitem__(PikaObj* self, int __key, int __val) {
    uint8_t* data = obj_getBytes(self, "raw");
    uint16_t len = obj_getBytesSize(self, "raw");
    if (__key < len) {
        data[__key] = __val;
    }
}

char* builtins_bytearray___str__(PikaObj* self) {
    uint8_t* data = obj_getBytes(self, "raw");
    uint16_t len = obj_getBytesSize(self, "raw");
    Arg* str_arg = arg_newStr("");
    str_arg = arg_strAppend(str_arg, "bytearray(b'");
    for (int i = 0; i < len; i++) {
        char u8_str[] = "\\x00";
        uint8_t u8 = data[i];
        __platform_sprintf(u8_str, "\\x%02x", u8);
        str_arg = arg_strAppend(str_arg, u8_str);
    }
    str_arg = arg_strAppend(str_arg, "')");
    obj_removeArg(self, "_buf");
    obj_setStr(self, "_buf", arg_getStr(str_arg));
    arg_deinit(str_arg);
    return obj_getStr(self, "_buf");
}

char* builtins_bytearray_decode(PikaObj* self) {
    uint8_t* data = obj_getBytes(self, "raw");
    return obj_cacheStr(self, (char*)data);
}

PikaObj* builtins_bytearray_split(PikaObj* self, PikaTuple* vars) {
    int max_splite = -1;
    Arg* sep = NULL;
    pika_bool sep_need_free = pika_false;
    if (pikaTuple_getSize(vars) >= 1) {
        sep = pikaTuple_getArg(vars, 0);
    } else {
        sep = arg_newBytes((uint8_t*)" ", 1);
        sep_need_free = pika_true;
    }
    if (pikaTuple_getSize(vars) == 2) {
        max_splite = pikaTuple_getInt(vars, 1);
    }
    if (arg_getType(sep) != ARG_TYPE_BYTES) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "TypeError: bytes is required");
        return NULL;
    }
    PikaList* list = New_PikaList();
    uint8_t* data = obj_getBytes(self, "raw");
    size_t len = obj_getBytesSize(self, "raw");
    if (len == 0) {
        pikaList_append(list, arg_newBytes((uint8_t*)"", 0));
        goto __exit;
    }
    uint8_t* sep_data = arg_getBytes(sep);
    size_t sep_len = arg_getBytesSize(sep);

    if (sep_len == 0) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "ValueError: empty separator");
        goto __exit;
    }

    size_t start = 0;
    int splits = 0;

    for (size_t i = 0; i < len; i++) {
        // Check if the current position can accommodate the separator
        if (i <= len - sep_len && memcmp(data + i, sep_data, sep_len) == 0) {
            // Found a separator, slice the bytearray from start to current
            // position
            uint8_t* target_data = data + start;
            size_t target_len = i - start;
            pikaList_append(list, arg_newBytes(target_data, target_len));

            // Move the start index past the separator
            start = i + sep_len;
            i = start - 1;  // Adjust `i` as it will be incremented by the loop

            // Increment the split count and check if we've reached the maximum
            // number of splits
            splits++;
            if (max_splite != -1 && splits >= max_splite) {
                break;
            }
        }
    }

    // Add the last segment if any remains
    if (start < len) {
        uint8_t* target_data = data + start;
        size_t target_len = len - start;
        pikaList_append(list, arg_newBytes(target_data, target_len));
    }

    // After loop
    if (sep_len > 0 && start == len) {
        pikaList_append(list, arg_newBytes((uint8_t*)"", 0));
    }

__exit:
    if (sep_need_free) {
        arg_deinit(sep);
    }
    return list;
}