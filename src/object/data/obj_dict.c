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

int32_t pikaDict_forEach(PikaObj* self,
                         int32_t (*eachHandle)(PikaObj* self,
                                               Arg* keyEach,
                                               Arg* valEach,
                                               void* context),
                         void* context) {
    Args* keys = _OBJ2KEYS(self);
    Args* dict = _OBJ2DICT(self);
    pika_assert(NULL != dict);
    pika_assert(NULL != keys);
    size_t size = args_getSize(keys);
    for (int i = size - 1; i >= 0; i--) {
        Arg* item_key = args_getArgByIndex(keys, i);
        Arg* item_val = args_getArgByIndex(dict, i);
        pika_assert(NULL != item_key);
        // Call the handle function on each key-value pair
        if (eachHandle(self, item_key, item_val, context) != 0) {
            return -1;
        }
    }
    return 0;
}

void pikaDict_init(PikaObj* self) {
    Args* dict = New_args(NULL);
    Args* keys = New_args(NULL);
    obj_setPtr(self, "dict", dict);
    obj_setPtr(self, "_keys", keys);
}

PIKA_RES pikaDict_setInt(PikaDict* self, char* name, int64_t val) {
    return pikaDict_set(self, name, arg_newInt(val));
}

pika_bool pikaDict_setBool(PikaDict* self, char* name, pika_bool val) {
    return pikaDict_set(self, name, arg_newBool(val));
}

PIKA_RES pikaDict_setFloat(PikaDict* self, char* name, pika_float val) {
    return pikaDict_set(self, name, arg_newFloat(val));
}

PIKA_RES pikaDict_setStr(PikaDict* self, char* name, char* val) {
    return pikaDict_set(self, name, arg_newStr(val));
}

PIKA_RES pikaDict_setPtr(PikaDict* self, char* name, void* val) {
    return pikaDict_set(self, name, arg_newPtr(ARG_TYPE_POINTER, val));
}

PIKA_RES _pikaDict_setVal(PikaDict* self, Arg* val) {
    return args_setArg(_OBJ2DICT(self), (val));
}

PIKA_RES pikaDict_set(PikaDict* self, char* name, Arg* val) {
    val = arg_setName(val, name);
    _pikaDict_setVal(self, val);
    return args_setStr(_OBJ2KEYS(self), (name), (name));
}

PIKA_RES pikaDict_removeArg(PikaDict* self, Arg* val) {
    // args_removeArg(_OBJ2KEYS(self), (val));
    return args_removeArg(_OBJ2DICT(self), (val));
}

PIKA_RES pikaDict_reverse(PikaDict* self) {
    args_reverse(_OBJ2KEYS(self));
    args_reverse(_OBJ2DICT(self));
    return PIKA_RES_OK;
}

PIKA_RES pikaDict_setBytes(PikaDict* self,
                           char* name,
                           uint8_t* val,
                           size_t size) {
    return pikaDict_set(self, name, arg_newBytes(val, size));
}

int64_t pikaDict_getInt(PikaDict* self, char* name) {
    return args_getInt(_OBJ2DICT(self), (name));
}

pika_bool pikaDict_getBool(PikaDict* self, char* name) {
    return args_getBool(_OBJ2DICT(self), (name));
}

pika_float pikaDict_getFloat(PikaDict* self, char* name) {
    return args_getFloat(_OBJ2DICT(self), (name));
}

char* pikaDict_getStr(PikaDict* self, char* name) {
    return args_getStr(_OBJ2DICT(self), (name));
}

void* pikaDict_getPtr(PikaDict* self, char* name) {
    return args_getPtr(_OBJ2DICT(self), (name));
}

int pikaDict_getSize(PikaDict* self) {
    return args_getSize(_OBJ2DICT(self));
}

Arg* pikaDict_getArgByidex(PikaDict* self, int index) {
    return args_getArgByIndex(_OBJ2DICT(self), (index));
}

Arg* pikaDict_get(PikaDict* self, char* name) {
    return args_getArg(_OBJ2DICT(self), (name));
}

int32_t pikaDict_isArgExist(PikaDict* self, char* name) {
    return args_isArgExist(_OBJ2DICT(self), (name));
}

uint8_t* pikaDict_getBytes(PikaDict* self, char* name) {
    return args_getBytes(_OBJ2DICT(self), (name));
}

ArgType pikaDict_getType(PikaDict* self, char* name) {
    return args_getType(_OBJ2DICT(self), (name));
}

size_t pikaDict_getBytesSize(PikaDict* self, char* name) {
    return args_getBytesSize(_OBJ2DICT(self), (name));
}

void pikaDict_deinit(PikaDict* self) {
    obj_deinit(self);
}