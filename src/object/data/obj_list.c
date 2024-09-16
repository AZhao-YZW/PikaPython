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

int32_t pikaList_forEach(PikaObj* self,
                         int32_t (*eachHandle)(PikaObj* self,
                                               int itemIndex,
                                               Arg* itemEach,
                                               void* context),
                         void* context) {
    int i = 0;
    while (PIKA_TRUE) {
        Arg* item = pikaList_get(self, i);
        if (NULL == item) {
            break;
        }
        // Call the handler function with the item.
        int32_t result = eachHandle(self, i, item, context);
        if (result != 0) {
            // If the handler function returns a non-zero value,
            // stop the iteration.
            return result;
        }
        i++;
    }
    return 0;
}

PIKA_RES pikaList_set(PikaList* self, int index, Arg* arg) {
    char buff[11];
    char* i_str = fast_itoa(buff, index);
    int top = pikaList_getSize(self);
    if (index > top) {
        return PIKA_RES_ERR_OUT_OF_RANGE;
    }
    arg = arg_setName(arg, i_str);
    args_setArg(_OBJ2LIST(self), arg);
    return PIKA_RES_OK;
}

Arg* pikaList_get(PikaList* self, int index) {
    pika_assert(NULL != self);
    char buff[11];
    char* i_str = fast_itoa(buff, index);
    return args_getArg(_OBJ2LIST(self), i_str);
}

int pikaList_getInt(PikaList* self, int index) {
    Arg* arg = pikaList_get(self, index);
    return arg_getInt(arg);
}

pika_bool pikaList_getBool(PikaList* self, int index) {
    Arg* arg = pikaList_get(self, index);
    return arg_getBool(arg);
}

pika_float pikaList_getFloat(PikaList* self, int index) {
    Arg* arg = pikaList_get(self, index);
    return arg_getFloat(arg);
}

char* pikaList_getStr(PikaList* self, int index) {
    Arg* arg = pikaList_get(self, index);
    return arg_getStr(arg);
}

void* pikaList_getPtr(PikaList* self, int index) {
    Arg* arg = pikaList_get(self, index);
    return arg_getPtr(arg);
}

PIKA_RES pikaList_append(PikaList* self, Arg* arg) {
    if (NULL == arg) {
        return PIKA_RES_ERR_ARG_NO_FOUND;
    }
    int top = pikaList_getSize(self);
    char buff[11];
    char* topStr = fast_itoa(buff, top);
    Arg* arg_to_push = arg;
    arg_setName(arg_to_push, topStr);
    args_setArg(_OBJ2LIST(self), arg_to_push);
    /* top++ */
    return args_setInt(_OBJ2LIST(self), "top", top + 1);
}

void pikaList_deinit(PikaList* self) {
    args_deinit(_OBJ2LIST(self));
}

ArgType pikaList_getType(PikaList* self, int index) {
    Arg* arg = pikaList_get(self, index);
    return arg_getType(arg);
}

Arg* pikaList_pop_withIndex(PikaList* self, int index) {
    int top = pikaList_getSize(self);
    if (top <= 0) {
        return NULL;
    }
    if (index < 0) {
        index = top + index;
    }
    Arg* arg = pikaList_get(self, index);
    Arg* res = arg_copy(arg);
    pikaList_remove(self, arg);
    return res;
}

Arg* pikaList_pop(PikaList* self) {
    int top = pikaList_getSize(self);
    if (top <= 0) {
        return NULL;
    }
    return pikaList_pop_withIndex(self, top - 1);
}

PIKA_RES pikaList_remove(PikaList* self, Arg* arg) {
    int top = pikaList_getSize(self);
    int i_remove = 0;
    if (top <= 0) {
        return PIKA_RES_ERR_OUT_OF_RANGE;
    }
    for (int i = 0; i < top; i++) {
        Arg* arg_now = pikaList_get(self, i);
        if (arg_isEqual(arg_now, arg)) {
            i_remove = i;
            args_removeArg(_OBJ2LIST(self), arg_now);
            break;
        }
    }
    /* move args */
    for (int i = i_remove + 1; i < top; i++) {
        char buff[11];
        char* i_str = fast_itoa(buff, i - 1);
        Arg* arg_now = pikaList_get(self, i);
        arg_setName(arg_now, i_str);
    }
    args_setInt(_OBJ2LIST(self), "top", top - 1);
    return PIKA_RES_OK;
}

PIKA_RES pikaList_insert(PikaList* self, int index, Arg* arg) {
    int top = pikaList_getSize(self);
    if (index > top) {
        return PIKA_RES_ERR_OUT_OF_RANGE;
    }
    /* move args */
    for (int i = top - 1; i >= index; i--) {
        char buff[11];
        char* i_str = fast_itoa(buff, i + 1);
        Arg* arg_now = pikaList_get(self, i);
        arg_setName(arg_now, i_str);
    }
    char buff[11];
    char* i_str = fast_itoa(buff, index);
    Arg* arg_to_push = arg_copy(arg);
    arg_setName(arg_to_push, i_str);
    args_setArg(_OBJ2LIST(self), arg_to_push);
    args_setInt(_OBJ2LIST(self), "top", top + 1);
    return PIKA_RES_OK;
}

size_t pikaList_getSize(PikaList* self) {
    if (NULL == self) {
        return 0;
    }
    int64_t ret = args_getInt(_OBJ2LIST(self), "top");
    pika_assert(ret >= 0);
    return ret;
}

void pikaList_init(PikaObj* self) {
    Args* list = New_args(NULL);
    args_pushArg_name(list, "top", arg_newInt(0));
    obj_setPtr(self, "list", list);
}

void pikaList_reverse(PikaList* self) {
    pika_assert(NULL != self);
    int top = pikaList_getSize(self);
    for (int i = 0; i < top / 2; i++) {
        Arg* arg_i = arg_copy(pikaList_get(self, i));
        Arg* arg_top = arg_copy(pikaList_get(self, top - i - 1));
        pikaList_set(self, i, arg_top);
        pikaList_set(self, top - i - 1, arg_i);
    }
}
