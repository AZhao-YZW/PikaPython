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
#include "obj_data.h"
#include "PikaObj.h"
#include "TinyObj.h"
#include "obj_class.h"
#include "gc.h"
#if __linux
#include "signal.h"
#include "termios.h"
#include "unistd.h"
#endif

PikaObj* New_PikaStdData_Dict(Args* args);
PikaObj* New_PikaStdData_List(Args* args);
PikaObj* New_PikaStdData_Tuple(Args* args);
void PikaStdData_Dict___init__(PikaObj* self);
void PikaStdData_List___init__(PikaObj* self);
void PikaStdData_Tuple___init__(PikaObj* self);

extern volatile PikaObjState g_PikaObjState;

static PikaObj* _pika_new_obj_with_args(PikaObj* (*constructor)(),
                                        void (*init_func)(PikaObj*),
                                        int num_args,
                                        va_list args) {
    PikaObj* obj = newNormalObj(constructor);
    init_func(obj);

    if (constructor == New_PikaStdData_Tuple ||
        constructor == New_PikaStdData_List) {
        for (int i = 0; i < num_args; i++) {
            Arg* arg = va_arg(args, Arg*);
            if (num_args == 1 && NULL == arg) {
                /* empty tuple */
                return obj;
            }
            pikaList_append(obj, arg);
        }
    } else if (constructor == New_PikaStdData_Dict) {
        if (num_args == 1) {
            /* empty dict */
            return obj;
        }
        for (int i = 0; i + 1 < num_args; i += 2) {
            Arg* aKey = va_arg(args, Arg*);
            char* sKey = arg_getStr(aKey);
            Arg* value = va_arg(args, Arg*);
            pikaDict_set(obj, sKey, value);
            arg_deinit(aKey);
        }
    }

    return obj;
}

PikaObj* _pika_dict_new(int num_args, ...) {
    va_list args;
    va_start(args, num_args);
    PikaObj* dict = _pika_new_obj_with_args(
        New_PikaStdData_Dict, PikaStdData_Dict___init__, num_args, args);
    va_end(args);
    return dict;
}

PikaObj* _pika_list_new(int num_args, ...) {
    va_list args;
    va_start(args, num_args);

    PikaObj* list = _pika_new_obj_with_args(
        New_PikaStdData_List, PikaStdData_List___init__, num_args, args);

    va_end(args);
    return list;
}

PikaObj* _pika_tuple_new(int num_args, ...) {
    va_list args;
    va_start(args, num_args);

    PikaObj* tuple = _pika_new_obj_with_args(
        New_PikaStdData_Tuple, PikaStdData_Tuple___init__, num_args, args);

    va_end(args);
    return tuple;
}

PikaObj* obj_newObjFromConstructor(PikaObj* context,
                                   char* name,
                                   NewFun constructor) {
    PikaObj* self = constructor(NULL);
    self->constructor = constructor;
    self->refcnt = 1;
    return self;
}

PikaObj* newNormalObj(NewFun newObjFun) {
    PikaObj* thisClass = obj_newObjFromConstructor(NULL, "", newObjFun);
    obj_setFlag(thisClass, OBJ_FLAG_ALREADY_INIT);
    return removeMethodInfo(thisClass);
}

static volatile uint8_t logo_printed = 0;
extern volatile PikaObj* __pikaMain;

#ifdef __linux
void signal_handler(int sig);
void enable_raw_mode(void);
#endif

PikaObj* newRootObj(char* name, NewFun newObjFun) {
    g_PikaObjState.inRootObj = pika_true;
#if PIKA_POOL_ENABLE
    mem_pool_init();
#endif
#ifdef __linux
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTRAP, signal_handler);
    signal(SIGHUP, signal_handler);
    enable_raw_mode();
#endif
    PikaObj* newObj = newNormalObj(newObjFun);
    if (!logo_printed) {
        logo_printed = 1;
        pika_platform_printf("\r\n");
        pika_platform_printf("~~~/ POWERED BY \\~~~\r\n");
        pika_platform_printf("~  pikapython.com  ~\r\n");
        pika_platform_printf("~~~~~~~~~~~~~~~~~~~~\r\n");
    }
    if (NULL != __pikaMain) {
        pika_platform_printf("Error: root object already exists\r\n");
        pika_platform_panic_handle();
        return NULL;
    }
    __pikaMain = newObj;
    obj_setName(newObj, name);
    g_PikaObjState.inRootObj = pika_false;
    return newObj;
}

Arg* arg_newMetaObj(NewFun new_obj_fun) {
    Arg* arg_new = New_arg(NULL);
    /* m means meta-object */
    arg_new = arg_setPtr(arg_new, "", ARG_TYPE_OBJECT_META, (void*)new_obj_fun);
    return arg_new;
}

Arg* arg_newDirectObj(NewFun new_obj_fun) {
    PikaObj* newObj = newNormalObj(new_obj_fun);
    Arg* arg_new = arg_newPtr(ARG_TYPE_OBJECT_NEW, newObj);
    return arg_new;
}

Arg* obj_newObjInPackage(NewFun new_obj_fun) {
    return arg_newDirectObj(new_obj_fun);
}

PikaObj* New_PikaObj(void) {
    PikaObj* self = pikaMalloc(sizeof(PikaObj));
    /* List */
    self->list = New_args(NULL);
    self->refcnt = 0;
    self->constructor = New_PikaObj;
    self->flag = 0;
    self->vmFrame = NULL;
#if PIKA_GC_MARK_SWEEP_ENABLE
    self->gcNext = NULL;
    obj_setFlag(self, OBJ_FLAG_GC_ROOT);
#endif
#if PIKA_KERNAL_DEBUG_ENABLE
    self->aName = NULL;
    self->name = "PikaObj";
    self->parent = NULL;
    self->isAlive = pika_true;
#endif
#if PIKA_GC_MARK_SWEEP_ENABLE && PIKA_KERNAL_DEBUG_ENABLE
    self->gcRoot = NULL;
#endif
    /* append to gc chain */
    pikaGC_append(self);
    pikaGC_checkThreshold();
    return self;
}

int32_t obj_newDirectObj(PikaObj* self, char* objName, NewFun newFunPtr) {
    Arg* aNewObj = arg_newDirectObj(newFunPtr);
    aNewObj = arg_setName(aNewObj, objName);
    obj_setName(arg_getPtr(aNewObj), objName);
    arg_setType(aNewObj, ARG_TYPE_OBJECT);
    // pikaGC_enable(arg_getPtr(aNewObj));
    obj_setArg_noCopy(self, objName, aNewObj);
    return 0;
}

int32_t obj_newHostObj(PikaObj* self, char* objName) {
    Args buffs = {0};
    size_t tokenCnt = strCountSign(objName, '.');
    if (0 == tokenCnt) {
        return 0;
    }
    PikaObj* this = self;
    objName = strsCopy(&buffs, objName);
    for (int i = 0; i < tokenCnt; i++) {
        char* name = strsPopToken(&buffs, &objName, '.');
        if (!obj_isArgExist(this, name)) {
            obj_newDirectObj(this, name, New_TinyObj);
            this = obj_getObj(this, name);
        }
    }
    strsDeinit(&buffs);
    return 0;
}

int32_t obj_newMetaObj(PikaObj* self, char* objName, NewFun newFunPtr) {
    /* add meta Obj, no inited */
    Arg* aMetaObj = arg_newMetaObj(newFunPtr);
    aMetaObj = arg_setName(aMetaObj, objName);
    args_setArg(self->list, aMetaObj);
    return 0;
}

static void _append_help(char* name) {
    if (NULL == g_PikaObjState.helpModulesCmodule) {
        g_PikaObjState.helpModulesCmodule = arg_newStr("");
    }
    Arg* _help = g_PikaObjState.helpModulesCmodule;
    _help = arg_strAppend(_help, name);
    _help = arg_strAppend(_help, "\r\n");
    g_PikaObjState.helpModulesCmodule = _help;
}

int32_t obj_newObj(PikaObj* self,
                   char* objName,
                   char* className,
                   NewFun newFunPtr) {
    /* before init root object */
    if (g_PikaObjState.inRootObj) {
        _append_help(objName);
    }
    return obj_newMetaObj(self, objName, newFunPtr);
}

PikaObj* _New_pikaListOrTuple(int isTuple) {
    Args* list = New_args(NULL);
    /* set top index for append */
    args_pushArg_name(list, "top", arg_newInt(0));
    PikaObj* self = NULL;
    if (isTuple) {
        self = newNormalObj(New_PikaStdData_Tuple);
    } else {
        self = newNormalObj(New_PikaStdData_List);
    }
    obj_setPtr(self, "list", list);
    return self;
}

PikaList* New_PikaList(void) {
    return _New_pikaListOrTuple(0);
}

PikaTuple* New_pikaTuple(void) {
    return _New_pikaListOrTuple(1);
}

PikaDict* New_PikaDict(void) {
    PikaDict* self = newNormalObj(New_PikaStdData_Dict);
    pikaDict_init(self);
    return self;
}