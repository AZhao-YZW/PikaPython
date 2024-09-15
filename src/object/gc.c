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
#include "gc.h"
#include "PikaObj.h"

extern volatile PikaObjState g_PikaObjState;

pika_bool obj_checkAlive(PikaObj* self) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return pika_true;
#else
    pika_bool ret = pika_false;
    if (NULL == g_PikaObjState.gcChain) {
        ret = pika_false;
        goto __exit;
    }
    PikaObj* obj = g_PikaObjState.gcChain;
    while (NULL != obj) {
        if (obj == self) {
            ret = pika_true;
            goto __exit;
        }
        obj = obj->gcNext;
    }
__exit:
#if PIKA_KERNAL_DEBUG_ENABLE
    if (ret == pika_true) {
        self->isAlive = ret;
    }
#endif
    return ret;
#endif
}

int obj_GC(PikaObj* self) {
    if (!obj_checkAlive(self)) {
        return 0;
    }
    if (obj_getFlag(self, OBJ_FLAG_IN_DEL)) {
        /* in __del__, skip gc */
        return 0;
    }
    obj_refcntDec(self);
    int ref_cnt = obj_refcntNow(self);
    if (ref_cnt <= 0) {
        obj_deinit(self);
    }
    return 0;
}

#if PIKA_GC_MARK_SWEEP_ENABLE
PikaObj* pikaGC_getLast(PikaObj* self) {
    PikaObj* obj = g_PikaObjState.gcChain;
    PikaObj* last = NULL;
    while (NULL != obj) {
        if (obj == self) {
            return last;
        }
        last = obj;
        obj = obj->gcNext;
    }
    return NULL;
}

void pikaGC_clean(PikaGC* gc) {
    PikaObj* obj = g_PikaObjState.gcChain;
    while (NULL != obj) {
        obj_clearFlag(obj, OBJ_FLAG_GC_MARKED);
        // obj->gcRoot = NULL;
        obj = obj->gcNext;
    }
}

uint32_t pikaGC_count(void) {
    uint32_t count = 0;
    PikaObj* obj = g_PikaObjState.gcChain;
    while (NULL != obj) {
        count++;
        obj = obj->gcNext;
    }
    return count;
}

uint32_t pikaGC_countMarked(void) {
    uint32_t count = 0;
    PikaObj* obj = g_PikaObjState.gcChain;
    while (NULL != obj) {
        if (obj_getFlag(obj, OBJ_FLAG_GC_MARKED)) {
            count++;
        }
        obj = obj->gcNext;
    }
    return count;
}

void obj_dump(PikaObj* self) {
#if PIKA_KERNAL_DEBUG_ENABLE
    pika_platform_printf("[\033[32m%s\033[0m]", self->name);
#endif
    pika_platform_printf(" \033[36m@%p\033[0m", self);
    pika_platform_printf("\r\n");
}

uint32_t pikaGC_printFreeList(void) {
    uint32_t count = 0;
    PikaObj* obj = g_PikaObjState.gcChain;
    pika_platform_printf("-----\r\n");
    while (NULL != obj) {
        if (!obj_getFlag(obj, OBJ_FLAG_GC_MARKED)) {
            count++;
            pika_platform_printf("gc free: ");
            obj_dump(obj);
        }
        obj = obj->gcNext;
    }
    pika_platform_printf("-----\r\n");
    return count;
}

int32_t _pikaGC_markHandler(Arg* argEach, void* context);
PikaObj* New_PikaStdData_Dict(Args* args);
PikaObj *New_PikaStdData_List(Args* args);
PikaObj *New_PikaStdData_Tuple(Args* args);

void pikaGC_markObj(PikaGC* gc, PikaObj* self) {
    gc->oThis = self;
    gc->markDeepth++;
    if (NULL == self) {
        goto __exit;
    }
    if (obj_getFlag(self, OBJ_FLAG_GC_MARKED)) {
        goto __exit;
    }
    obj_setFlag(self, OBJ_FLAG_GC_MARKED);
    if (NULL != gc->onMarkObj) {
        gc->onMarkObj(gc);
    }
    args_foreach(self->list, _pikaGC_markHandler, gc);
    if (self->constructor == New_PikaStdData_Dict) {
        Args* dict = _OBJ2DICT(self);
        if (NULL == dict) {
            goto __exit;
        }
        args_foreach(dict, _pikaGC_markHandler, (void*)gc);
        goto __exit;
    }
    if (self->constructor == New_PikaStdData_List ||
        self->constructor == New_PikaStdData_Tuple) {
        Args* list = _OBJ2LIST(self);
        if (NULL == list) {
            goto __exit;
        }
        args_foreach(list, _pikaGC_markHandler, (void*)gc);
        goto __exit;
    }
__exit:
    gc->markDeepth--;
    return;
}

int32_t _pikaGC_markHandler(Arg* argEach, void* context)
{
    PikaGC* gc = (PikaGC*)context;
    if (arg_isObject(argEach)) {
        PikaObj* obj = (PikaObj*)arg_getPtr(argEach);
#if PIKA_KERNAL_DEBUG_ENABLE
        obj->gcRoot = (void*)gc->oThis;
#endif
        pikaGC_markObj(gc, obj);
    }
    return 0;
}

void _pikaGC_mark(PikaGC* gc) {
    pikaGC_clean(gc);
    PikaObj* root = g_PikaObjState.gcChain;
    while (NULL != root) {
        if (obj_getFlag(root, OBJ_FLAG_GC_ROOT)) {
            pikaGC_markObj(gc, root);
        }
        root = root->gcNext;
    }
}

uint32_t pikaGC_markSweepOnce(PikaGC* gc) {
    _pikaGC_mark(gc);
    uint32_t count = 0;
    PikaObj* freeList[16] = {0};
    PikaObj* obj = g_PikaObjState.gcChain;
    while (NULL != obj) {
        if (!obj_getFlag(obj, OBJ_FLAG_GC_MARKED)) {
            if (count > dimof(freeList) - 1) {
                break;
            }
            freeList[count] = obj;
            count++;
        }
        obj = obj->gcNext;
    }
    if (count > 0) {
        // pikaGC_markDump();
        // pikaGC_printFreeList();
        for (uint32_t i = 0; i < count; i++) {
            pika_platform_printf("GC Free:");
            obj_dump(freeList[i]);
        }
        for (uint32_t i = 0; i < count; i++) {
            obj_GC(freeList[i]);
        }
    }
    return count;
}

void pikaGC_mark(void) {
    PikaGC gc = {0};
    _pikaGC_mark(&gc);
}

int _pikaGC_markDumpHandler(PikaGC* gc) {
    for (uint32_t i = 0; i < gc->markDeepth - 1; i++) {
        pika_platform_printf("  |");
    }
    if (gc->markDeepth != 1) {
        pika_platform_printf("- ");
    }
    obj_dump(gc->oThis);
    return 0;
}
#endif

void pikaGC_markDump(void) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return;
#else
    PikaGC gc = {0};
    pika_platform_printf(
        "\033[32m"
        "========= PIKA GC DUMP =========\r\n"
        "\033[0m");
    gc.onMarkObj = _pikaGC_markDumpHandler;
    _pikaGC_mark(&gc);
#endif
}

void pikaGC_lock(void) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return;
#else
    g_PikaObjState.markSweepBusy++;
#endif
}

void pikaGC_unlock(void) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return;
#else
    g_PikaObjState.markSweepBusy--;
#endif
}

pika_bool pikaGC_islock(void) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return pika_false;
#else
    return g_PikaObjState.markSweepBusy > 0;
#endif
}

uint32_t pikaGC_markSweep(void) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return 0;
#else
    PikaGC gc = {0};
    uint32_t count = 0;
    if (pikaGC_islock()) {
        return 0;
    }
    pikaGC_lock();
    while (pikaGC_markSweepOnce(&gc) != 0) {
        count++;
    };
    if (count > 0) {
        // pikaGC_markDump();
    }
    /* update gc state */
    g_PikaObjState.objCntLastGC = g_PikaObjState.objCnt;
    pikaGC_unlock();
    return count;
#endif
}

void pikaGC_checkThreshold(void) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return;
#else
    if (g_PikaObjState.objCnt >
        g_PikaObjState.objCntLastGC + PIKA_GC_MARK_SWEEP_THRESHOLD) {
        pikaGC_markSweep();
    }
#endif
}

void pikaGC_append(PikaObj* self) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return;
#else
    g_PikaObjState.objCnt++;
    if (g_PikaObjState.objCntMax < g_PikaObjState.objCnt) {
        g_PikaObjState.objCntMax = g_PikaObjState.objCnt;
    }
    /* gc single chain */
    if (NULL == g_PikaObjState.gcChain) {
        g_PikaObjState.gcChain = self;
        return;
    }
    /* append to head of gc chain */
    self->gcNext = g_PikaObjState.gcChain;
    g_PikaObjState.gcChain = self;
#endif
}

void obj_removeGcChain(PikaObj* self) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return;
#else
    g_PikaObjState.objCnt--;
    PikaObj* last = pikaGC_getLast(self);
    if (NULL == last) {
        /* remove head */
        g_PikaObjState.gcChain = self->gcNext;
        return;
    }
    last->gcNext = self->gcNext;
#endif
}

void obj_enableGC(PikaObj* self) {
#if !PIKA_GC_MARK_SWEEP_ENABLE
    return;
#else
    obj_clearFlag(self, OBJ_FLAG_GC_ROOT);
#endif
}