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
#ifndef __HISTORY_H__
#define __HISTORY_H__

typedef struct ShellHistory {
    int max_size;
    int current;
    int count;
    int last_offset;
    char** history;
    int cached_current;
} ShellHistory;

#if PIKA_SHELL_HISTORY_ENABLE
ShellHistory* shHistory_create(int max_size);
void shHistory_destroy(ShellHistory* self);
void shHistory_add(ShellHistory* self, char* command);
void shHistory_setMaxSize(ShellHistory* self, int max_size);
char* shHistory_get(ShellHistory* self, int index);
char* shHistory_getPrev(ShellHistory* self);
char* shHistory_getNext(ShellHistory* self);
#endif

#endif