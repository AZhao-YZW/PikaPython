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

void pikaTuple_deinit(PikaTuple* self) {
    pikaList_deinit(self);
}

Arg* pikaTuple_getArg(PikaTuple* self, int index) {
    return pikaList_get(self, (index));
}

size_t pikaTuple_getSize(PikaTuple* self) {
    if (self == NULL) {
        return 0;
    }
    return pikaList_getSize(self);
}

int64_t pikaTuple_getInt(PikaTuple* self, int index) {
    return pikaList_getInt(self, (index));
}

pika_bool pikaTuple_getBool(PikaTuple* self, int index) {
    return pikaList_getBool(self, (index));
}

pika_float pikaTuple_getFloat(PikaTuple* self, int index) {
    return pikaList_getFloat(self, (index));
}

char* pikaTuple_getStr(PikaTuple* self, int index) {
    return pikaList_getStr(self, (index));
}

void* pikaTuple_getPtr(PikaTuple* self, int index) {
    return pikaList_getPtr(self, (index));
}

ArgType pikaTuple_getType(PikaTuple* self, int index) {
    return pikaList_getType(self, (index));
}
