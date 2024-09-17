/*
 * MIT License
 *
 * Copyright (c) 2024 vetor7 张悦斌 1015149201@qq.com
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
#include "object.h"

static PIKA_RES obj_setValue(PikaObj* self, char* argPath, void* value, SetType type, size_t size) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return PIKA_RES_ERR_ARG_NO_FOUND;
    }
    char* name = strPointToLastToken(argPath, '.');
    
    switch (type) {
        case SET_INT: args_setInt(obj->list, name, *(int64_t*)value); break;
        case SET_PTR: args_setPtr(obj->list, name, value); break;
        case SET_REF: args_setRef(obj->list, name, (PikaObj*)value); break;
        case SET_FLOAT: args_setFloat(obj->list, name, *(pika_float*)value); break;
        case SET_STR: 
            return args_setStr(obj->list, name, (char*)value);
        case SET_NONE: args_setNone(obj->list, name); break;
        case SET_BYTES: args_setBytes(obj->list, name, (uint8_t*)value, size); break;
    }
    return PIKA_RES_OK;
}

PIKA_RES obj_setInt(PikaObj* self, char* argPath, int64_t val) {
    return obj_setValue(self, argPath, &val, SET_INT, 0);
}

PIKA_RES obj_setPtr(PikaObj* self, char* argPath, void* pointer) {
    return obj_setValue(self, argPath, pointer, SET_PTR, 0);
}

PIKA_RES obj_setRef(PikaObj* self, char* argPath, PikaObj* pointer) {
    return obj_setValue(self, argPath, pointer, SET_REF, 0);
}

PIKA_RES obj_setFloat(PikaObj* self, char* argPath, pika_float value) {
    return obj_setValue(self, argPath, &value, SET_FLOAT, 0);
}

PIKA_RES obj_setStr(PikaObj* self, char* argPath, char* str) {
    return obj_setValue(self, argPath, str, SET_STR, 0);
}

PIKA_RES obj_setNone(PikaObj* self, char* argPath) {
    return obj_setValue(self, argPath, NULL, SET_NONE, 0);
}

PIKA_RES obj_setBytes(PikaObj* self, char* argPath, uint8_t* src, size_t size) {
    return obj_setValue(self, argPath, src, SET_BYTES, size);
}