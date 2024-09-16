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
#include "PikaObj.h"
#include "TinyObj.h"

Arg* _obj_getMethodArg(PikaObj* obj, char* methodName, Arg* arg_reg) {
    Arg* aMethod = NULL;
    aMethod = obj_getArg(obj, methodName);
    if (NULL != aMethod) {
        aMethod = arg_copy_noalloc(aMethod, arg_reg);
        goto __exit;
    }
    aMethod = _obj_getPropArg(obj, methodName);
__exit:
    return aMethod;
}

Arg* _obj_getMethodArgWithFullPath(PikaObj* obj,
                                   char* methodPath,
                                   Arg* arg_reg) {
    char* methodName = strPointToLastToken(methodPath, '.');
    return _obj_getMethodArg(obj, methodName, arg_reg);
}

Arg* obj_getMethodArgWithFullPath(PikaObj* obj, char* methodPath) {
    return _obj_getMethodArgWithFullPath(obj, methodPath, NULL);
}

Arg* obj_getMethodArgWithFullPath_noalloc(PikaObj* obj,
                                          char* methodPath,
                                          Arg* arg_reg) {
    return _obj_getMethodArgWithFullPath(obj, methodPath, arg_reg);
}

Arg* obj_getMethodArg(PikaObj* obj, char* methodName) {
    return _obj_getMethodArg(obj, methodName, NULL);
}

Arg* obj_getMethodArg_noalloc(PikaObj* obj, char* methodName, Arg* arg_reg) {
    return _obj_getMethodArg(obj, methodName, arg_reg);
}

PikaObj* removeMethodInfo(PikaObj* thisClass) {
#if PIKA_METHOD_CACHE_ENABLE
#else
    args_removeArg(thisClass->list, args_getArg(thisClass->list, "@p_"));
#endif
    return thisClass;
}

Method methodArg_getPtr(Arg* method_arg) {
    MethodProp* method_store = (MethodProp*)arg_getContent(method_arg);
    return (Method)method_store->ptr;
}

char* methodArg_getTypeList(Arg* method_arg, char* buffs, size_t size) {
    pika_assert(arg_isCallable(method_arg));
    MethodProp* prop = (MethodProp*)arg_getContent(method_arg);
    if (NULL != prop->type_list) {
        pika_assert(strGetSize(prop->type_list) < size);
        return strcpy(buffs, prop->type_list);
    }
    char* method_dec = methodArg_getDec(method_arg);
    pika_assert(strGetSize(method_dec) <= size);
    if (strGetSize(method_dec) > size) {
        pika_platform_printf(
            "OverFlowError: please use bigger PIKA_LINE_BUFF_SIZE\r\n");
        while (1) {
        }
    }
    char* res = strCut(buffs, method_dec, '(', ')');
    return res;
}

MethodProp* methodArg_getProp(Arg* method_arg) {
    return (MethodProp*)arg_getContent(method_arg);
}

PikaObj* methodArg_getHostObj(Arg* method_arg) {
    pika_assert(argType_isObjectMethodActive(arg_getType(method_arg)));
    MethodProp* prop = (MethodProp*)arg_getContent(method_arg);
    return prop->host_obj;
}

Arg* methodArg_active(Arg* method_arg) {
    pika_assert(arg_getType(method_arg) == ARG_TYPE_METHOD_NATIVE);
    Arg* aActive = New_arg(NULL);
    MethodPropNative* propNative =
        (MethodPropNative*)arg_getContent(method_arg);
    MethodProp prop = {0};
    /* active the method */
    pika_platform_memcpy(&prop, propNative, sizeof(MethodPropNative));
    aActive = arg_setStruct(aActive, "", (uint8_t*)&prop, sizeof(MethodProp));
    arg_setType(aActive, ARG_TYPE_METHOD_NATIVE_ACTIVE);
    return aActive;
}

Arg* methodArg_setHostObj(Arg* method_arg, PikaObj* host_obj) {
    if (!argType_isObjectMethod(arg_getType(method_arg))) {
        return method_arg;
    }
    if (arg_getType(method_arg) == ARG_TYPE_METHOD_NATIVE) {
        method_arg = methodArg_active(method_arg);
    }
    MethodProp* prop = (MethodProp*)arg_getContent(method_arg);
    if (prop->host_obj == NULL) {
        prop->host_obj = host_obj;
#if 0
        obj_refcntInc(host_obj);
#endif
        return method_arg;
    }
    return method_arg;
}

char* methodArg_getName(Arg* method_arg, char* buffs, size_t size) {
    MethodProp* method_store = (MethodProp*)arg_getContent(method_arg);
    if (NULL != method_store->name) {
        return strcpy(buffs, method_store->name);
    }
    char* method_dec = methodArg_getDec(method_arg);
    pika_assert(strGetSize(method_dec) <= size);
    char* res = strGetFirstToken(buffs, method_dec, '(');
    return res;
}

extern Arg* _get_return_arg(PikaObj* locals);
extern NativeProperty* obj_getProp(PikaObj* self);

NativeProperty* methodArg_toProp(Arg* method_arg) {
    pika_assert(arg_getType(method_arg) == ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR);
    PikaObj* locals = New_TinyObj(NULL);
    MethodProp* method_store = (MethodProp*)arg_getContent(method_arg);
    Method fMethod = (Method)method_store->ptr;
    fMethod(NULL, locals->list);
    Arg* aReturn = _get_return_arg(locals);
    if (arg_getType(aReturn) == ARG_TYPE_OBJECT_NEW) {
        arg_setType(aReturn, ARG_TYPE_OBJECT);
    }
    PikaObj* obj = arg_getPtr(aReturn);
    NativeProperty* prop = obj_getProp(obj);
    arg_deinit(aReturn);
    obj_deinit(locals);
    return prop;
}

char* _find_super_class_name(ByteCodeFrame *bcframe, int32_t pc_start);
Arg* methodArg_super(Arg* aThis, NativeProperty** p_prop) {
    Arg* aSuper = NULL;
    PikaObj* builtins = NULL;
    ArgType type = arg_getType(aThis);
    *p_prop = NULL;
    if (!argType_isConstructor(type)) {
        aSuper = NULL;
        goto __exit;
    }
    if (type == ARG_TYPE_METHOD_CONSTRUCTOR) {
        builtins = obj_getBuiltins();
        MethodProp* method_store = (MethodProp*)arg_getContent(aThis);
        ByteCodeFrame *bcframe = method_store->bytecode_frame;
        int32_t pc = (uintptr_t)method_store->ptr -
                     (uintptr_t)bcframe->instruct_array.content_start;
        char* sSuper = _find_super_class_name(bcframe, pc);
        /* map TinyObj to object */
        if (strEqu(sSuper, "TinyObj")) {
            sSuper = "object";
        }
        PikaObj* context = method_store->def_context;
        aSuper = obj_getMethodArgWithFullPath(context, sSuper);
        if (NULL == aSuper) {
            aSuper = obj_getMethodArgWithFullPath(builtins, sSuper);
        }
        goto __exit;
    }
    if (type == ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR) {
        NativeProperty* prop = methodArg_toProp(aThis);
        *p_prop = prop;
        arg_deinit(aThis);
        aSuper = NULL;
        goto __exit;
    }
__exit:
    if (NULL != builtins) {
        obj_deinit(builtins);
    }
    if (NULL != aSuper) {
        arg_deinit(aThis);
    }
    return aSuper;
}

Method obj_getNativeMethod(PikaObj* self, char* method_name) {
    Arg* method_arg = obj_getMethodArgWithFullPath(self, method_name);
    if (NULL == method_arg) {
        return NULL;
    }
    Method res = methodArg_getPtr(method_arg);
    arg_deinit(method_arg);
    return res;
}

ByteCodeFrame *methodArg_getBytecodeFrame(Arg* method_arg) {
    MethodProp* method_store = (MethodProp*)arg_getContent(method_arg);
    return method_store->bytecode_frame;
}

char* methodArg_getDec(Arg* method_arg) {
    MethodProp* method_store = (MethodProp*)arg_getContent(method_arg);
    return method_store->declareation;
}

PikaObj* methodArg_getDefContext(Arg* method_arg) {
    MethodProp* method_store = (MethodProp*)arg_getContent(method_arg);
    return method_store->def_context;
}

void _update_proxy(PikaObj* self, char* name) {
#if PIKA_NANO_ENABLE
    return;
#endif
    if (name[0] != '_' || name[1] != '_') {
        return;
    }
    if (!obj_getFlag(self, OBJ_FLAG_PROXY_GETATTRIBUTE)) {
        if (strEqu(name, "__getattribute__")) {
            obj_setFlag(self, OBJ_FLAG_PROXY_GETATTRIBUTE);
            return;
        }
    }
    if (!obj_getFlag(self, OBJ_FLAG_PROXY_GETATTR)) {
        if (strEqu(name, "__getattr__")) {
            obj_setFlag(self, OBJ_FLAG_PROXY_GETATTR);
            return;
        }
    }
    if (!obj_getFlag(self, OBJ_FLAG_PROXY_SETATTR)) {
        if (strEqu(name, "__setattr__")) {
            obj_setFlag(self, OBJ_FLAG_PROXY_SETATTR);
            return;
        }
    }
    if (!obj_getFlag(self, OBJ_FLAG_PROXY_METHOD)) {
        if (strEqu(name, "__proxy__")) {
            obj_setFlag(self, OBJ_FLAG_PROXY_METHOD);
            return;
        }
    }
}

void obj_saveMethodInfo(PikaObj* self, MethodInfo* tInfo) {
    Arg* aMethod = New_arg(NULL);
    MethodProp tProp = {
        .ptr = tInfo->ptr,
        .type_list = tInfo->typelist,
        .name = tInfo->name,
        .bytecode_frame = tInfo->bytecode_frame,
        .def_context = tInfo->def_context,
        .declareation = tInfo->dec,  // const
        .host_obj = NULL,
    };
    char* name = tInfo->name;
    if (NULL == tInfo->name) {
        char name_buff[PIKA_LINE_BUFF_SIZE / 2] = {0};
        name = strGetFirstToken(name_buff, tInfo->dec, '(');
    }
    /* the first arg_value */
    aMethod = arg_setStruct(aMethod, name, &tProp, sizeof(tProp));
    pika_assert(NULL != aMethod);
    arg_setType(aMethod, tInfo->type);
    _update_proxy(self, name);
    args_setArg(self->list, aMethod);
}

pika_bool obj_isMethodExist(PikaObj* self, char* method) {
    Arg* arg = obj_getMethodArg(self, method);
    if (NULL == arg) {
        return pika_false;
    }
    arg_deinit(arg);
    return pika_true;
}

void method_returnBytes(Args* args, uint8_t* val) {
    args_pushArg_name(args, "@rt", arg_newBytes(val, PIKA_BYTES_DEFAULT_SIZE));
}

void method_returnStr(Args* args, char* val) {
    args_pushArg_name(args, "@rt", arg_newStr(val));
}

void method_returnInt(Args* args, int64_t val) {
    args_pushArg_name(args, "@rt", arg_newInt(val));
}

void method_returnBool(Args* args, pika_bool val) {
    if (val == _PIKA_BOOL_ERR) {
        return;
    }
    args_pushArg_name(args, "@rt", arg_newBool(val));
}

void method_returnFloat(Args* args, pika_float val) {
    args_pushArg_name(args, "@rt", arg_newFloat(val));
}

void method_returnPtr(Args* args, void* val) {
    args_pushArg_name(args, "@rt", arg_newPtr(ARG_TYPE_POINTER, val));
}

void method_returnObj(Args* args, void* val) {
    if (NULL == val) {
        args_pushArg_name(args, "@rt", arg_newNone());
        return;
    }
    ArgType type;
    PikaObj* obj = (PikaObj*)val;
    if (obj_getFlag(obj, OBJ_FLAG_ALREADY_INIT)) {
        type = ARG_TYPE_OBJECT;
    } else {
        type = ARG_TYPE_OBJECT_NEW;
    }
    args_pushArg_name(args, "@rt", arg_newPtr(type, val));
}

void method_returnArg(Args* args, Arg* arg) {
    args_pushArg_name(args, "@rt", arg);
}

int64_t method_getInt(Args* args, char* argName) {
    return args_getInt(args, argName);
}

pika_float method_getFloat(Args* args, char* argName) {
    return args_getFloat(args, argName);
}

char* method_getStr(Args* args, char* argName) {
    return args_getStr(args, argName);
}
