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
#include "history.h"
#include "PikaObj.h"

#if PIKA_SHELL_HISTORY_ENABLE
ShellHistory* shHistory_create(int max_size) {
    ShellHistory* self = (ShellHistory*)pikaMalloc(sizeof(ShellHistory));
    self->max_size = max_size;
    self->current = -1;
    self->count = 0;
    self->last_offset = 0;
    self->cached_current = 0;
    self->history = (char**)pikaMalloc(max_size * sizeof(char*));
    return self;
}

void shHistory_destroy(ShellHistory* self) {
    for (int i = 0; i < self->count; i++) {
        pikaFree(self->history[i], strGetSize(self->history[i]) + 1);
    }
    pikaFree(self->history, sizeof(char*) * self->max_size);
    pikaFree(self, sizeof(ShellHistory));
}

void shHistory_add(ShellHistory* self, char* command) {
    if (self->count == self->max_size) {
        pikaFree(self->history[0], strGetSize(self->history[0]) + 1);
        pika_platform_memmove(self->history, self->history + 1,
                              (self->max_size - 1) * sizeof(char*));
        self->count--;
    }

    /* filter for empty command */
    if (self->count > 0 && self->history[self->count - 1][0] == '\0') {
        pikaFree(self->history[self->count - 1],
                 strGetSize(self->history[self->count - 1]) + 1);
        self->count--;
    }

    /* filter for same command */
    if (self->count > 0 && strEqu(self->history[self->count - 1], command)) {
        return;
    }

    self->history[self->count] = pikaMalloc(strGetSize(command) + 1);
    pika_platform_memcpy(self->history[self->count], command,
                         strGetSize(command) + 1);
    self->count++;
    self->current = self->count - 1;
    self->last_offset = 0;
}

char* shHistory_get(ShellHistory* self, int offset) {
    int actual_offset = offset + self->last_offset;
    int index = self->current + actual_offset;
    if (index < 0 || index >= self->count) {
        return NULL;
    }
    self->last_offset = actual_offset;
    return self->history[index];
}

char* shHistory_getPrev(ShellHistory* self) {
    return shHistory_get(self, -1);
}

char* shHistory_getNext(ShellHistory* self) {
    return shHistory_get(self, 1);
}
#endif

#if __linux
#define PIKA_BACKSPACE_FORCE() printf("\b \b")
#else
#define PIKA_BACKSPACE_FORCE() pika_platform_printf("\b \b")
#endif

extern void _putc_cmd(char KEY_POS, int pos);

void handle_history_navigation(char inputChar, ShellConfig* shell, pika_bool bIsUp)
{
#if PIKA_SHELL_HISTORY_ENABLE
    if (NULL == shell->history) {
        shell->history = shHistory_create(PIKA_SHELL_HISTORY_NUM);
    }
    if (0 == shell->history->cached_current) {
        /* save the current line */
        shHistory_add(shell->history, shell->lineBuff);
        shell->history->cached_current = 1;
    }
    char* history_line = bIsUp ? shHistory_getPrev(shell->history)
                               : shHistory_getNext(shell->history);
    if (NULL == history_line) {
        return;
    }
    /* move to the last position */
    for (int i = 0; i < shell->line_position - shell->line_curpos; i++) {
        _putc_cmd(PIKA_KEY_RIGHT, 1);
    }
    /* clear the current line */
    for (int i = 0; i < shell->line_position; i++) {
        PIKA_BACKSPACE_FORCE();
    }
    pika_platform_memcpy(shell->lineBuff, history_line,
                         strGetSize(history_line) + 1);
    /* show the previous line */
    pika_platform_printf("%s", shell->lineBuff);
    shell->line_position = strGetSize(history_line);
    shell->line_curpos = shell->line_position;
#endif
    return;
}
