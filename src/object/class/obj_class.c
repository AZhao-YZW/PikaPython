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
#include "obj_class.h"
#include "PikaObj.h"

NewFun obj_getClass(PikaObj* obj) {
    return (NewFun)obj->constructor;
}

PikaObj* obj_getClassObj(PikaObj* obj) {
    NewFun classPtr = obj_getClass(obj);
    if (NULL == classPtr) {
        return NULL;
    }
    PikaObj* classObj = obj_newObjFromConstructor(obj, "", classPtr);
    return classObj;
}

void* getNewClassObjFunByName(PikaObj* obj, char* name) {
    char* classPath = name;
    /* init the subprocess */
    void* newClassObjFun = args_getPtr(obj->list, classPath);
    return newClassObjFun;
}

static int32_t __class_defineMethodWithType(PikaObj* self,
                                            char* declareation,
                                            char* name,
                                            char* typelist,
                                            Method method_ptr,
                                            ArgType method_type,
                                            PikaObj* def_context,
                                            ByteCodeFrame *bytecode_frame) {
    int32_t res = 0;
    Args buffs = {0};
    PikaObj* method_host = self;
    MethodInfo method_info = {0};
    if (NULL == method_host) {
        /* no found method object */
        res = 1;
        goto __exit;
    }
    method_info.dec = declareation;
    method_info.name = name;
    method_info.ptr = (void*)method_ptr;
    method_info.type = method_type;
    method_info.def_context = def_context;
    method_info.bytecode_frame = bytecode_frame;
    method_info.typelist = typelist;
    obj_saveMethodInfo(method_host, &method_info);
    res = 0;
    goto __exit;
__exit:
    strsDeinit(&buffs);
    return res;
}

/* define a constructor method */
int32_t class_defineConstructor(PikaObj* self,
                                char* name,
                                char* typelist,
                                Method methodPtr) {
    return __class_defineMethodWithType(self, NULL, name, typelist, methodPtr,
                                        ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR,
                                        NULL, NULL);
}

/* define a native method as default */
int32_t class_defineMethod(PikaObj* self,
                           char* name,
                           char* typelist,
                           Method methodPtr) {
    return __class_defineMethodWithType(self, NULL, name, typelist, methodPtr,
                                        ARG_TYPE_METHOD_NATIVE, NULL, NULL);
}

/* define object method, object method is which startwith (self) */
int32_t class_defineRunTimeConstructor(PikaObj* self,
                                       char* declareation,
                                       Method methodPtr,
                                       PikaObj* def_context,
                                       ByteCodeFrame *bytecode_frame) {
    return __class_defineMethodWithType(self, declareation, NULL, NULL,
                                        methodPtr, ARG_TYPE_METHOD_CONSTRUCTOR,
                                        def_context, bytecode_frame);
}

/* define object method, object method is which startwith (self) */
int32_t class_defineObjectMethod(PikaObj* self,
                                 char* declareation,
                                 Method methodPtr,
                                 PikaObj* def_context,
                                 ByteCodeFrame *bytecode_frame) {
    return __class_defineMethodWithType(self, declareation, NULL, NULL,
                                        methodPtr, ARG_TYPE_METHOD_OBJECT,
                                        def_context, bytecode_frame);
}

/* define a static method as default */
int32_t class_defineStaticMethod(PikaObj* self,
                                 char* declareation,
                                 Method methodPtr,
                                 PikaObj* def_context,
                                 ByteCodeFrame *bytecode_frame) {
    return __class_defineMethodWithType(self, declareation, NULL, NULL,
                                        methodPtr, ARG_TYPE_METHOD_STATIC,
                                        def_context, bytecode_frame);
}