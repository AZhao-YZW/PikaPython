/*
 * This file is part of the PikaPython project.
 * http://github.com/pikastech/pikapython
 *
 * MIT License
 *
 * Copyright (c) 2021 lyon liang6516@outlook.com
 * Copyright (c) 2023 Gorgon Meducer embedded_zhuroan@hotmail.com
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
#include <stdint.h>
#include "BaseObj.h"
#include "PikaCompiler.h"
#include "PikaParser.h"
#include "PikaPlatform.h"
#include "dataArg.h"
#include "dataArgs.h"
#include "dataMemory.h"
#include "dataQueue.h"
#include "dataString.h"
#include "dataStrs.h"
#include "object.h"
#include "gc.h"
#include "history.h"
#include "obj_class.h"
#if __linux
#include "signal.h"
#include "termios.h"
#include "unistd.h"
#endif

extern volatile VMState g_PikaVMState;
volatile PikaObjState g_PikaObjState = {
    .helpModulesCmodule = NULL,
    .inRootObj = pika_false,
#if PIKA_GC_MARK_SWEEP_ENABLE
    .objCnt = 0,
    .objCntMax = 0,
    .objCntLastGC = 0,
    .gcChain = NULL,
    .markSweepBusy = 0,
#endif
};

extern volatile PikaObj* __pikaMain;
static volatile ShellConfig g_REPL;

PikaObj* New_PikaStdData_Dict(Args* args);
PikaObj* New_PikaStdData_List(Args* args);
PikaObj* New_PikaStdData_Tuple(Args* args);
void _mem_cache_deinit(void);
void vm_event_deinit(void);
void locals_deinit(PikaObj* self);

static enum shellCTRL __obj_shellLineHandler_REPL(PikaObj* self,
                                                  char* input_line,
                                                  ShellConfig* shell);

#if __linux
struct termios original_termios;

void enable_raw_mode(void) {
    struct termios raw;

    tcgetattr(STDIN_FILENO, &original_termios);
    raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    printf("\n");
}

void signal_handler(int sig) {
    if (sig == SIGSEGV) {
        printf("Segmentation fault");
    } else if (sig == SIGINT) {
        printf("Ctrl+C");
    } else if (sig == SIGTERM) {
        printf("SIGTERM");
    } else if (sig == SIGHUP) {
        printf("SIGHUP");
    } else if (sig == SIGABRT) {
        printf("Aborted");
    }
    disable_raw_mode();
    exit(1);
}
#endif

static int32_t obj_deinit_no_del(PikaObj* self) {
    /* free the list */
    locals_deinit(self);
    args_deinit_ex(self->list, pika_true);
#if PIKA_KERNAL_DEBUG_ENABLE
    if (NULL != self->aName) {
        arg_deinit(self->aName);
    }
#endif
    /* remove self from gc chain */
    obj_removeGcChain(self);
    /* free the pointer */
    pikaFree(self, sizeof(PikaObj));
    if (self == (PikaObj*)__pikaMain) {
        __pikaMain = NULL;
    }
    return 0;
}

int32_t obj_deinit(PikaObj* self) {
    pikaGC_lock();
    pika_bool bisRoot = pika_false;
#if PIKA_KERNAL_DEBUG_ENABLE
    self->isAlive = pika_false;
#endif
    Arg* del = obj_getMethodArgWithFullPath(self, "__del__");
    if (NULL != del) {
        obj_setFlag(self, OBJ_FLAG_IN_DEL);
        Arg* aRes = obj_runMethodArg0(self, del);
        if (NULL != aRes) {
            arg_deinit(aRes);
        }
    }
    extern volatile PikaObj* __pikaMain;
    if (self == (PikaObj*)__pikaMain) {
        bisRoot = pika_true;
        _mem_cache_deinit();
#if PIKA_EVENT_ENABLE
        vm_event_deinit();
#endif
        if (NULL != g_PikaObjState.helpModulesCmodule) {
            arg_deinit(g_PikaObjState.helpModulesCmodule);
            g_PikaObjState.helpModulesCmodule = NULL;
        }
    }
    int32_t ret = obj_deinit_no_del(self);
    pikaGC_unlock();
    if (bisRoot) {
        pikaGC_markSweep();
        shConfig_deinit((ShellConfig*)&g_REPL);
#if __linux
        disable_raw_mode();
        vm_gil_deinit();
#endif
    }
    return ret;
}

int64_t obj_getInt(PikaObj* self, char* argPath) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return _PIKA_INT_ERR;
    }
    char* argName = strPointToLastToken(argPath, '.');
    int64_t res = args_getInt(obj->list, argName);
    return res;
}

pika_bool obj_getBool(PikaObj* self, char* argPath) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return pika_false;
    }
    char* argName = strPointToLastToken(argPath, '.');
    pika_bool res = args_getBool(obj->list, argName);
    return res;
}

Arg* obj_getArg(PikaObj* self, char* argPath) {
    pika_assert(obj_checkAlive(self));
    pika_bool is_temp = pika_false;
    PikaObj* obj = obj_getHostObjWithIsTemp(self, argPath, &is_temp);
    if (NULL == obj) {
        return NULL;
    }
    Arg* res = NULL;
    char* argName = strPointToLastToken(argPath, '.');
    res = args_getArg(obj->list, argName);
    if (is_temp) {
        obj_setArg(self, "_buf", res);
        res = obj_getArg(self, "_buf");
        obj_deinit(obj);
    }
    return res;
}

uint8_t* obj_getBytes(PikaObj* self, char* argPath) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return NULL;
    }
    char* argName = strPointToLastToken(argPath, '.');
    return args_getBytes(obj->list, argName);
}

size_t obj_getBytesSize(PikaObj* self, char* argPath) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return 0;
    }
    char* argName = strPointToLastToken(argPath, '.');
    return args_getBytesSize(obj->list, argName);
}

size_t obj_loadBytes(PikaObj* self, char* argPath, uint8_t* out_buff) {
    size_t size_mem = obj_getBytesSize(self, argPath);
    void* src = obj_getBytes(self, argPath);
    if (0 == size_mem) {
        return 0;
    }
    if (NULL == src) {
        return 0;
    }
    pika_platform_memcpy(out_buff, src, size_mem);
    return size_mem;
}

void obj_setName(PikaObj* self, char* name) {
#if !PIKA_KERNAL_DEBUG_ENABLE
    return;
#else
    if (strEqu(name, "self")) {
        return;
    }
    if (NULL != self->aName) {
        if (!strstr(self->name, name)) {
            self->aName = arg_strAppend(self->aName, "|");
            self->aName = arg_strAppend(self->aName, name);
        }
    } else {
        self->aName = arg_newStr(name);
    }
    self->name = arg_getStr(self->aName);
#endif
}

static PIKA_RES _obj_setArg(PikaObj* self,
                            char* argPath,
                            Arg* arg,
                            uint8_t is_copy) {
    pika_assert(obj_checkAlive(self));
    /* setArg would copy arg */
    PikaObj* host = obj_getHostObj(self, argPath);
    PikaObj* oNew = NULL;
    pika_bool bNew = pika_false;
    if (NULL == host) {
        /* object no found */
        return PIKA_RES_ERR_ARG_NO_FOUND;
    }
    char* sArgName = strPointToLastToken(argPath, '.');
    Arg* aNew;
    if (is_copy) {
        aNew = arg_copy(arg);
    } else {
        aNew = arg;
    }
    aNew = arg_setName(aNew, sArgName);
    if (arg_isObject(aNew)) {
        oNew = arg_getPtr(aNew);
        bNew = pika_true;
        pika_assert(obj_checkAlive(oNew));
#if PIKA_KERNAL_DEBUG_ENABLE
        if (host != oNew) {
            /* skip self ref */
            oNew->parent = host;
        }
#endif
        obj_setName(oNew, sArgName);
    }
#if 0
    if (argType_isObjectMethodActive(arg_getType(arg))) {
        PikaObj* host_self = methodArg_getHostObj(arg);
        obj_refcntInc(host_self);
    }
#endif
    args_setArg(host->list, aNew);
    /* enable mark sweep to collect this object */
    if (bNew) {
        /* only enable mark sweep after setArg */
        obj_enableGC(oNew);
    }
    return PIKA_RES_OK;
}

PIKA_RES obj_setArg(PikaObj* self, char* argPath, Arg* arg) {
    return _obj_setArg(self, argPath, arg, 1);
};

PIKA_RES obj_setArg_noCopy(PikaObj* self, char* argPath, Arg* arg) {
    return _obj_setArg(self, argPath, arg, 0);
}

PIKA_RES obj_setObj(PikaObj* self, char* argPath, PikaObj* obj) {
    Arg* arg = arg_newObj(obj);
    return obj_setArg_noCopy(self, argPath, arg);
}

void* obj_getPtr(PikaObj* self, char* argPath) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return NULL;
    }
    char* argName = strPointToLastToken(argPath, '.');
    void* res = args_getPtr(obj->list, argName);
    return res;
}

pika_float obj_getFloat(PikaObj* self, char* argPath) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return -999.999;
    }
    char* argName = strPointToLastToken(argPath, '.');
    pika_float res = args_getFloat(obj->list, argName);
    return res;
}

char* obj_getStr(PikaObj* self, char* argPath) {
    PikaObj* obj = obj_getHostObj(self, argPath);
    if (NULL == obj) {
        return NULL;
    }
    char* argName = strPointToLastToken(argPath, '.');
    char* res = args_getStr(obj->list, argName);
    return res;
}

NativeProperty* obj_getProp(PikaObj* self) {
    Arg* aProp = obj_getArg(self, "@p_");
    PikaObj* class_obj = NULL;
    if (NULL == aProp) {
        if (NULL != self->constructor) {
            class_obj = obj_getClassObj(self);
            aProp = obj_getArg(class_obj, "@p_");
        }
    }
    NativeProperty* prop = NULL;
    if (aProp == NULL) {
        goto __exit;
    }
    if (arg_getType(aProp) != ARG_TYPE_POINTER) {
        goto __exit;
    }
    prop = (NativeProperty*)arg_getPtr(aProp);
__exit:
    if (NULL != class_obj) {
        obj_deinit_no_del(class_obj);
    }
    return prop;
}

Arg* _obj_getPropArg(PikaObj* obj, char* name) {
    NativeProperty* prop = obj_getProp(obj);
    Hash method_hash = hash_time33(name);
    Arg* aMethod = NULL;
    while (1) {
        if (prop == NULL) {
            break;
        }
        int size = prop->methodGroupCount;
        pika_assert(size >= 0);
        /* binary search */
        if (size == 0) {
            goto __next;
        }
#if !PIKA_NANO_ENABLE
        if (size > 16) {
            int left = 0;
            int right = size - 1;
            int mid = 0;
            while (1) {
                if (left > right) {
                    break;
                }
                mid = (right + left) >> 1;
                Arg* prop_this = (Arg*)(prop->methodGroup + mid);
                if (prop_this->name_hash == method_hash) {
                    aMethod = prop_this;
                    goto __exit;
                } else if (prop_this->name_hash < method_hash) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            goto __next;
        }
#endif
        for (int i = 0; i < (int)prop->methodGroupCount; i++) {
            Arg* prop_this = (Arg*)(prop->methodGroup + i);
            if (prop_this->name_hash == method_hash) {
                aMethod = prop_this;
                goto __exit;
            }
        }
        goto __next;
    __next:
        prop = (NativeProperty*)prop->super;
    }
__exit:
    return aMethod;
}

#ifdef __linux

#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)
static int set_disp_mode(int fd, int option) {
    int err;
    struct termios term;
    if (tcgetattr(fd, &term) == -1) {
        perror("Cannot get the attribution of the terminal");
        return 1;
    }
    if (option)
        term.c_lflag |= ECHOFLAGS;
    else
        term.c_lflag &= ~ECHOFLAGS;
    err = tcsetattr(fd, TCSAFLUSH, &term);
    if (err == -1) {
        perror("Cannot set the attribution of the terminal");
        return 1;
    }
    return 0;
}
#endif

extern volatile PikaObj* __pikaMain;

static PikaObj* _obj_initMetaObj(PikaObj* obj, char* name) {
    PikaObj* res = NULL;
    NewFun constructor = (NewFun)getNewClassObjFunByName(obj, name);
    Args buffs = {0};
    PikaObj* thisClass;
    PikaObj* oNew;
    if (NULL == constructor) {
        /* no such object */
        res = NULL;
        goto __exit;
    }
    thisClass = obj_newObjFromConstructor(obj, name, constructor);
    oNew = removeMethodInfo(thisClass);
    obj_setName(oNew, name);
    obj_runNativeMethod(oNew, "__init__", NULL);
    args_setPtrWithType(obj->list, name, ARG_TYPE_OBJECT, oNew);
    res = obj_getPtr(obj, name);
    // pikaGC_enable(res);
    goto __exit;
__exit:
    strsDeinit(&buffs);
    return res;
}

PikaObj* _arg_to_obj(Arg* self, pika_bool* pIsTemp) {
    if (NULL == self) {
        return NULL;
    }
    if (arg_isObject(self)) {
        return arg_getPtr(self);
    }
#if !PIKA_NANO_ENABLE
    if (arg_getType(self) == ARG_TYPE_STRING) {
        PikaObj* New_PikaStdData_String(Args * args);
        PikaObj* obj = newNormalObj(New_PikaStdData_String);
        obj_setStr(obj, "str", arg_getStr(self));
        if (NULL != pIsTemp) {
            *pIsTemp = pika_true;
        }
        return obj;
    }
    if (arg_getType(self) == ARG_TYPE_BYTES) {
        PikaObj* New_PikaStdData_ByteArray(Args * args);
        PikaObj* obj = newNormalObj(New_PikaStdData_ByteArray);
        obj_setArg(obj, "raw", self);
        if (NULL != pIsTemp) {
            *pIsTemp = pika_true;
        }
        return obj;
    }
#endif
    return NULL;
}

static PikaObj* _obj_getObjDirect(PikaObj* self,
                                  char* name,
                                  pika_bool* pIsTemp) {
    *pIsTemp = pika_false;
    if (NULL == self) {
        return NULL;
    }
    /* finded object, check type*/
    Arg* aObj = args_getArg(self->list, name);
    ArgType type = ARG_TYPE_NONE;
    if (NULL == aObj) {
        aObj = _obj_getPropArg(self, name);
    }
    if (NULL == aObj) {
        return NULL;
    }
    type = arg_getType(aObj);
    /* found meta Object */
    if (type == ARG_TYPE_OBJECT_META) {
        return _obj_initMetaObj(self, name);
    }
    /* found Objcet */
    if (argType_isObject(type)) {
        return arg_getPtr(aObj);
    }
#if !PIKA_NANO_ENABLE
    /* found class */
    if (type == ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR ||
        type == ARG_TYPE_METHOD_CONSTRUCTOR) {
        *pIsTemp = pika_true;
        PikaObj* oMethodArgs = New_TinyObj(NULL);
        Arg* aClsObj = obj_runMethodArg(self, oMethodArgs, aObj);
        obj_deinit(oMethodArgs);
        Args* args = New_args(NULL);
        if (type == ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR) {
            obj_runNativeMethod(arg_getPtr(aClsObj), "__init__", args);
        }
        PikaObj* res = arg_getPtr(aClsObj);
        arg_deinit(aClsObj);
        args_deinit(args);
        return res;
    }
#endif
    return _arg_to_obj(aObj, pIsTemp);
}

static PikaObj* _obj_getObjWithKeepDeepth(PikaObj* self,
                                          char* objPath,
                                          pika_bool* pIsTemp,
                                          int32_t keepDeepth) {
    char objPath_buff[PIKA_PATH_BUFF_SIZE];
    char* objPath_ptr = objPath_buff;
    pika_assert(NULL != objPath);
    if ('.' == objPath[0] && '\0' == objPath[1]) {
        return self;
    }
    pika_assert(strGetSize(objPath) < PIKA_PATH_BUFF_SIZE);
    strcpy(objPath_buff, objPath);
    int32_t token_num = strGetTokenNum(objPath, '.');
    PikaObj* objThis = self;
    PikaObj* objNext = NULL;
    pika_bool bThisIsTemp = pika_false;
    for (int32_t i = 0; i < token_num - keepDeepth; i++) {
        char* token = strPopFirstToken(&objPath_ptr, '.');
        objNext = _obj_getObjDirect(objThis, token, pIsTemp);
        if (objNext == NULL) {
            objThis = NULL;
            goto __exit;
        }
        if (bThisIsTemp) {
            if (*pIsTemp == pika_false) {
                obj_refcntInc(objNext);
                *pIsTemp = pika_true;
            }
            obj_deinit(objThis);
        }
        pika_assert_obj_alive(objNext);
        objThis = objNext;
        bThisIsTemp = *pIsTemp;
    }
    goto __exit;
__exit:
    if (NULL != objThis) {
        pika_assert(obj_checkAlive(objThis));
    }
    return objThis;
}

PikaObj* obj_getObj(PikaObj* self, char* objPath) {
    pika_assert(NULL != objPath);
    pika_bool is_temp = pika_false;
    return _obj_getObjWithKeepDeepth(self, objPath, &is_temp, 0);
}

PikaObj* obj_getHostObj(PikaObj* self, char* objPath) {
    pika_assert(NULL != objPath);
    pika_bool is_temp = pika_false;
    return _obj_getObjWithKeepDeepth(self, objPath, &is_temp, 1);
}

PikaObj* obj_getHostObjWithIsTemp(PikaObj* self,
                                  char* objPath,
                                  pika_bool* pIsTemp) {
    return _obj_getObjWithKeepDeepth(self, objPath, pIsTemp, 1);
}

int32_t obj_removeArg(PikaObj* self, char* argPath) {
    PikaObj* objHost = obj_getHostObj(self, argPath);
    char* argName;
    int32_t res;
    int32_t err = 0;
    if (NULL == objHost) {
        /* [error] object no found */
        err = 1;
        goto __exit;
    }
    argName = strPointToLastToken(argPath, '.');
    res = args_removeArg(objHost->list, args_getArg(objHost->list, argName));
    if (1 == res) {
        /*[error] not found arg*/
        err = 2;
        goto __exit;
    }
    goto __exit;
__exit:
    return err;
}

pika_bool obj_isArgExist(PikaObj* self, char* argPath) {
    if (NULL == argPath) {
        return 0;
    }
    PikaObj* obj_host = obj_getHostObj(self, argPath);
    if (obj_host == NULL) {
        return pika_false;
    }
    int32_t res = 0;
    char* argName;
    Arg* arg;
    pika_assert(NULL != obj_host);
    argName = strPointToLastToken(argPath, '.');
    arg = args_getArg(obj_host->list, argName);
    if (NULL == arg) {
        /* no found arg */
        res = 0;
        goto __exit;
    }
    /* found arg */
    res = 1;
    goto __exit;

__exit:
    return res;
}

VMParameters* obj_run(PikaObj* self, char* cmd) {
    return pikaVM_run(self, cmd);
}

PIKA_RES obj_runNativeMethod(PikaObj* self, char* method_name, Args* args) {
    Method native_method = obj_getNativeMethod(self, method_name);
    if (NULL == native_method) {
        return PIKA_RES_ERR_ARG_NO_FOUND;
    }
    native_method(self, args);
    return PIKA_RES_OK;
}

static void __clearBuff(ShellConfig* shell) {
    memset(shell->lineBuff, 0, PIKA_LINE_BUFF_SIZE);
    shell->line_curpos = 0;
    shell->line_position = 0;
}

enum PIKA_SHELL_STATE {
    PIKA_SHELL_STATE_NORMAL,
    PIKA_SHELL_STATE_WAIT_SPEC_KEY,
    PIKA_SHELL_STATE_WAIT_FUNC_KEY,
};

static void _obj_runChar_beforeRun(PikaObj* self, ShellConfig* shell) {
    /* create the line buff for the first time */
    shell->inBlock = pika_false;
    shell->stat = PIKA_SHELL_STATE_NORMAL;
    shell->line_position = 0;
    shell->line_curpos = 0;
    __clearBuff(shell);
    pika_platform_printf("%s", shell->prefix);
}

void _putc_cmd(char KEY_POS, int pos) {
    const char cmd[] = {0x1b, 0x5b, KEY_POS, 0x00};
    for (int i = 0; i < pos; i++) {
        pika_platform_printf((char*)cmd);
    }
}

#if PIKA_SHELL_FILTER_ENABLE
typedef enum {
    __FILTER_NO_RESULT,
    __FILTER_FAIL_DROP_ONE,
    __FILTER_SUCCESS_GET_ALL_PEEKED,
    __FILTER_SUCCESS_DROP_ALL_PEEKED
} FilterReturn;

pika_bool _filter_msg_hi_pika_handler(FilterItem* msg,
                                      PikaObj* self,
                                      ShellConfig* shell) {
    pika_platform_printf("Yes, I am here\r\n");
    return pika_true;
}

pika_bool _filter_msg_bye_pika_handler(FilterItem* msg,
                                       PikaObj* self,
                                       ShellConfig* shell) {
    pika_platform_printf("OK, see you\r\n");
    return pika_true;
}

#define __MSG_DECLARE
#include "__default_filter_msg_table.h"

static const FilterItem g_default_filter_messages[] = {
#define __MSG_TABLE
#include "__default_filter_msg_table.h"
};

static FilterReturn _do_message_filter(PikaObj* self,
                                       ShellConfig* shell,
                                       FilterItem* msg,
                                       uint_fast16_t count) {
    pika_assert(NULL != msg);
    pika_assert(count > 0);
    ByteQueue* queue = &shell->filter_fifo.queue;
    FilterReturn result = __FILTER_FAIL_DROP_ONE;

    do {
        do {
            if (msg->ignore_mask & shell->filter_fifo.ignore_mask) {
                /* this message should be ignored */
                break;
            }

            if (NULL == msg->message) {
                break;
            }

            uint_fast16_t message_size = msg->size;
            if (!message_size) {
                break;
            }

            byteQueue_resetPeek(queue);

            /* do message comparison */
            uint8_t* src = (uint8_t*)msg->message;

            if (msg->is_case_insensitive) {
                do {
                    uint8_t byte;
                    if (!byteQueue_peekOne(queue, &byte)) {
                        result = __FILTER_NO_RESULT;
                        break;
                    }
                    char letter = *src++;

                    if (letter >= 'A' && letter <= 'Z') {
                        letter += 'a' - 'A';
                    }

                    if (byte >= 'A' && byte <= 'Z') {
                        byte += 'a' - 'A';
                    }

                    if (letter != byte) {
                        break;
                    }
                } while (--message_size);
            } else {
                do {
                    uint8_t byte;
                    if (!byteQueue_peekOne(queue, &byte)) {
                        result = __FILTER_NO_RESULT;
                        break;
                    }
                    if (*src++ != byte) {
                        break;
                    }
                } while (--message_size);
            }

            if (0 == message_size) {
                /* message match */
                if (NULL != msg->handler) {
                    if (!msg->handler(msg, self, shell)) {
                        break;
                    }
                }
                /* message is handled */
                if (msg->is_visible) {
                    return __FILTER_SUCCESS_GET_ALL_PEEKED;
                }
                return __FILTER_SUCCESS_DROP_ALL_PEEKED;
            }
        } while (0);

        msg++;
    } while (--count);

    return result;
}

int16_t _do_stream_filter(PikaObj* self, ShellConfig* shell) {
    ByteQueue* queue = &shell->filter_fifo.queue;

    FilterReturn result =
        _do_message_filter(self, shell, (FilterItem*)g_default_filter_messages,
                           dimof(g_default_filter_messages));
    int16_t drop_count = 0;

    switch (result) {
        case __FILTER_NO_RESULT:
            break;
        case __FILTER_FAIL_DROP_ONE:
            drop_count = 1;
            break;
        case __FILTER_SUCCESS_DROP_ALL_PEEKED:
            byteQueue_dropAllPeeked(queue);
            return 0;
        case __FILTER_SUCCESS_GET_ALL_PEEKED:
            drop_count = byteQueue_getPeekedNumber(queue);
            return drop_count;
    }

    /* user registered message filter */
    if (NULL != shell->messages && shell->message_count) {
        result = _do_message_filter(self, shell, shell->messages,
                                    shell->message_count);
        switch (result) {
            case __FILTER_NO_RESULT:
                break;
            case __FILTER_FAIL_DROP_ONE:
                drop_count = 1;
                break;
            case __FILTER_SUCCESS_DROP_ALL_PEEKED:
                byteQueue_dropAllPeeked(&shell->filter_fifo.queue);
                return 0;
            case __FILTER_SUCCESS_GET_ALL_PEEKED:
                drop_count =
                    byteQueue_getPeekedNumber(&shell->filter_fifo.queue);
                return drop_count;
        }
    }

    return drop_count;
}
#endif

#define PIKA_BACKSPACE() pika_platform_printf(" \b")

extern void handle_history_navigation(char inputChar, ShellConfig* shell, pika_bool bIsUp);

enum shellCTRL _inner_do_obj_runChar(PikaObj* self,
                                     char inputChar,
                                     ShellConfig* shell) {
    char* input_line = NULL;
    enum shellCTRL ctrl = SHELL_CTRL_CONTINUE;
    if (inputChar == 0x7F) {
        inputChar = '\b';
    }
    if (g_REPL.no_echo == pika_false) {
#if __linux
        printf("%c", inputChar);
#elif !(defined(_WIN32))
        pika_platform_printf("%c", inputChar);
#endif
    }
    if (inputChar == '\n' && shell->lastChar == '\r') {
        ctrl = SHELL_CTRL_CONTINUE;
        goto __exit;
    }
    if (inputChar == 0x1b) {
        shell->stat = PIKA_SHELL_STATE_WAIT_SPEC_KEY;
        ctrl = SHELL_CTRL_CONTINUE;
        goto __exit;
    }
    if (shell->stat == PIKA_SHELL_STATE_WAIT_SPEC_KEY) {
        if (inputChar == 0x5b) {
            shell->stat = PIKA_SHELL_STATE_WAIT_FUNC_KEY;
            ctrl = SHELL_CTRL_CONTINUE;
            goto __exit;
        } else {
            shell->stat = PIKA_SHELL_STATE_NORMAL;
        }
    }
    if (shell->stat == PIKA_SHELL_STATE_WAIT_FUNC_KEY) {
        shell->stat = PIKA_SHELL_STATE_NORMAL;
        if (inputChar == PIKA_KEY_LEFT) {
            if (shell->line_curpos) {
                shell->line_curpos--;
            } else {
                pika_platform_printf(" ");
            }
            ctrl = SHELL_CTRL_CONTINUE;
            goto __exit;
        }
        if (inputChar == PIKA_KEY_RIGHT) {
            if (shell->line_curpos < shell->line_position) {
                // pika_platform_printf("%c",
                // shell->lineBuff[shell->line_curpos]);
                shell->line_curpos++;
            } else {
                pika_platform_printf("\b");
            }
            ctrl = SHELL_CTRL_CONTINUE;
            goto __exit;
        }
        if (inputChar == PIKA_KEY_UP) {
            _putc_cmd(PIKA_KEY_DOWN, 1);
            ctrl = SHELL_CTRL_CONTINUE;
            handle_history_navigation(inputChar, shell, pika_true);
            goto __exit;
        }
        if (inputChar == PIKA_KEY_DOWN) {
            ctrl = SHELL_CTRL_CONTINUE;
            handle_history_navigation(inputChar, shell, pika_false);
            goto __exit;
        }
    }
    if (inputChar == '\b') {
        if (shell->line_curpos == 0) {
#if __linux
            printf(" ");
#else
            pika_platform_printf(" ");
#endif
            ctrl = SHELL_CTRL_CONTINUE;
            goto __exit;
        }
        PIKA_BACKSPACE();
        shell->line_position--;
        shell->line_curpos--;
        pika_platform_memmove(shell->lineBuff + shell->line_curpos,
                              shell->lineBuff + shell->line_curpos + 1,
                              shell->line_position - shell->line_curpos);
        shell->lineBuff[shell->line_position] = 0;
        if (shell->line_curpos != shell->line_position) {
            /* update screen */
            pika_platform_printf("%s", shell->lineBuff + shell->line_curpos);
            pika_platform_printf(" ");
            _putc_cmd(PIKA_KEY_LEFT,
                      shell->line_position - shell->line_curpos + 1);
        }
        ctrl = SHELL_CTRL_CONTINUE;
        goto __exit;
    }
    if ((inputChar != '\r') && (inputChar != '\n')) {
        if (shell->line_position + 1 >= PIKA_LINE_BUFF_SIZE) {
            pika_platform_printf(
                "\r\nError: line buff overflow, please use bigger "
                "'PIKA_LINE_BUFF_SIZE'\r\n");
            ctrl = SHELL_CTRL_EXIT;
            __clearBuff(shell);
            goto __exit;
        }
        if ('\0' != inputChar) {
            pika_platform_memmove(shell->lineBuff + shell->line_curpos + 1,
                                  shell->lineBuff + shell->line_curpos,
                                  shell->line_position - shell->line_curpos);
            shell->lineBuff[shell->line_position + 1] = 0;
            if (shell->line_curpos != shell->line_position) {
                pika_platform_printf("%s",
                                     shell->lineBuff + shell->line_curpos + 1);
                _putc_cmd(PIKA_KEY_LEFT,
                          shell->line_position - shell->line_curpos);
            }
            shell->lineBuff[shell->line_curpos] = inputChar;
            shell->line_position++;
            shell->line_curpos++;
        }
        ctrl = SHELL_CTRL_CONTINUE;
        goto __exit;
    }
    if ((inputChar == '\r') || (inputChar == '\n')) {
#if !(defined(__linux) || defined(_WIN32) || PIKA_SHELL_NO_NEWLINE)
        pika_platform_printf("\r\n");
#endif
#if PIKA_SHELL_HISTORY_ENABLE
        if (NULL == shell->history) {
            shell->history = shHistory_create(PIKA_SHELL_HISTORY_NUM);
        }
        if (shell->lineBuff[0] != '\0') {
            shHistory_add(shell->history, shell->lineBuff);
        }
        shell->history->last_offset = 0;
        shell->history->cached_current = 0;
#endif
        /* still in block */
        if (shell->blockBuffName != NULL && shell->inBlock) {
            /* load new line into buff */
            Args buffs = {0};
            char _n = '\n';
            strAppendWithSize(shell->lineBuff, &_n, 1);
            char* shell_buff_new =
                strsAppend(&buffs, obj_getStr(self, shell->blockBuffName),
                           shell->lineBuff);
            obj_setStr(self, shell->blockBuffName, shell_buff_new);
            strsDeinit(&buffs);
            /* go out from block */
            if ((shell->lineBuff[0] != ' ') && (shell->lineBuff[0] != '\t')) {
                shell->inBlock = pika_false;
                input_line = obj_getStr(self, shell->blockBuffName);
                ctrl = shell->handler(self, input_line, shell);
                __clearBuff(shell);
                pika_platform_printf(">>> ");
                goto __exit;
            } else {
                pika_platform_printf("... ");
            }
            __clearBuff(shell);
            ctrl = SHELL_CTRL_CONTINUE;
            goto __exit;
        }
        /* go in block */
        if (shell->blockBuffName != NULL && 0 != strGetSize(shell->lineBuff)) {
            if (shell->lineBuff[strGetSize(shell->lineBuff) - 1] == ':') {
                shell->inBlock = pika_true;
                char _n = '\n';
                strAppendWithSize(shell->lineBuff, &_n, 1);
                obj_setStr(self, shell->blockBuffName, shell->lineBuff);
                __clearBuff(shell);
                pika_platform_printf("... ");
                ctrl = SHELL_CTRL_CONTINUE;
                goto __exit;
            }
        }
        shell->lineBuff[shell->line_position] = '\0';
        ctrl = shell->handler(self, shell->lineBuff, shell);
        if (SHELL_CTRL_EXIT != ctrl) {
            pika_platform_printf("%s", shell->prefix);
        }
        __clearBuff(shell);
        goto __exit;
    }
__exit:
    shell->lastChar = inputChar;
    return ctrl;
}

PIKA_WEAK
enum shellCTRL _do_obj_runChar(PikaObj* self,
                               char inputChar,
                               ShellConfig* shell) {
#if PIKA_SHELL_FILTER_ENABLE
    ByteQueue* queue = &(shell->filter_fifo.queue);

    /* validation */
    if (NULL == queue->buffer) {
        /* need initialize first */
        byteQueue_init(queue, &shell->filter_fifo.buffer,
                       sizeof(shell->filter_fifo.buffer), pika_false);
    }

    pika_bool result = byteQueue_writeOne(queue, inputChar);
    (void)result;
    pika_assert(result != pika_false);

    int16_t byte_count;
    do {
        if (0 == byteQueue_peekAvailableCount(queue)) {
            break;
        }
        byte_count = _do_stream_filter(self, shell);
        int16_t n = byte_count;

        while (n--) {
            result = byteQueue_readOne(queue, (uint8_t*)&inputChar);
            pika_assert(pika_false != result);

            if (SHELL_CTRL_EXIT ==
                _inner_do_obj_runChar(self, inputChar, shell)) {
                return SHELL_CTRL_EXIT;
            }
        }
    } while (byte_count);

    return SHELL_CTRL_CONTINUE;
#else
    return _inner_do_obj_runChar(self, inputChar, shell);
#endif
}

enum shellCTRL obj_runChar(PikaObj* self, char inputChar) {
    ShellConfig* shell = args_getHeapStruct(self->list, "@shcfg");
    if (NULL == shell) {
        /* init the shell */
        ShellConfig newShell = {0};
        newShell.prefix = ">>> ";
        newShell.blockBuffName = "@sh1";
        newShell.handler = __obj_shellLineHandler_REPL;
        args_setHeapStruct(self->list, "@shcfg", newShell, shConfig_deinit);
        shell = args_getHeapStruct(self->list, "@shcfg");
        _obj_runChar_beforeRun(self, shell);
    }
    return _do_obj_runChar(self, inputChar, shell);
}

static void _save_file(char* file_name, uint8_t* buff, size_t size) {
    pika_platform_printf("[   Info] Saving file to '%s'...\r\n", file_name);
    FILE* fp = pika_platform_fopen(file_name, "wb+");
    if (NULL == fp) {
        pika_platform_printf("[  Error] Open file '%s' error!\r\n", file_name);
        pika_platform_fclose(fp);
    } else {
        if (pika_platform_fwrite(buff, 1, size, fp) != size) {
            pika_platform_printf("[  Error] Failed to write to '%s'...\r\n",
                                 file_name);
            pika_platform_fclose(fp);
            pika_platform_printf("[   Info] Removing '%s'...\r\n", file_name);
            pika_platform_remove(file_name);
            return;
        } else {
            pika_platform_printf("[   Info] Writing %d bytes to '%s'...\r\n",
                                 (int)(size), file_name);
        }
        pika_platform_fclose(fp);
        pika_platform_printf("[    OK ] Writing to '%s' succeed!\r\n",
                             file_name);
    }
}

char _await_getchar(sh_getchar fn_getchar) {
    vm_gil_exit();
    char ret = fn_getchar();
    vm_gil_enter();
    return ret;
}

#define PIKA_MAGIC_CODE_LEN 4

/* return file size */
PIKA_WEAK uint32_t _pikaShell_recv_file(ShellConfig* cfg,
                                        uint8_t* magic_code,
                                        uint8_t** pbuff) {
    uint32_t size = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t* size_byte = (uint8_t*)&size;
        size_byte[i] = cfg->fn_getchar();
    }
    if (magic_code[3] == 'o') {
        size += sizeof(uint32_t) * 2;
    }
    uint8_t* buff = pikaMalloc(size);
    /* save magic code and size */
    memcpy(buff, magic_code, PIKA_MAGIC_CODE_LEN);
    memcpy(buff + PIKA_MAGIC_CODE_LEN, &size, sizeof(size));

    for (uint32_t i = sizeof(uint32_t) * 2; i < size; i++) {
        buff[i] = cfg->fn_getchar();
    }
    *pbuff = buff;
    return size;
}

void _do_pikaScriptShell(PikaObj* self, ShellConfig* cfg) {
    /* init the shell */
    _obj_runChar_beforeRun(self, cfg);

    /* getchar and run */
    char inputChar[2] = {0};
    while (1) {
        inputChar[1] = inputChar[0];
        inputChar[0] = _await_getchar(cfg->fn_getchar);
#if !PIKA_NANO_ENABLE
        /* run python script */
        if (inputChar[0] == '!' && inputChar[1] == '#') {
            /* #! xxx */
            /* start */
            char* buff = pikaMalloc(PIKA_READ_FILE_BUFF_SIZE);
            char input[2] = {0};
            int buff_i = 0;
            pika_bool is_exit = pika_false;
            pika_bool is_first_line = pika_true;
            while (1) {
                input[1] = input[0];
                input[0] = cfg->fn_getchar();
                if (input[0] == '!' && input[1] == '#') {
                    buff[buff_i - 1] = 0;
                    for (int i = 0; i < 4; i++) {
                        /* eat 'pika' */
                        cfg->fn_getchar();
                    }
                    break;
                }
                if ('\r' == input[0]) {
                    continue;
                }
                if (is_first_line) {
                    if ('\n' == input[0]) {
                        is_first_line = pika_false;
                    }
                    continue;
                }
                buff[buff_i++] = input[0];
            }
            /* end */
            pika_platform_printf(
                "\r\n=============== [Code] ===============\r\n");
            size_t len = strGetSize(buff);
            for (size_t i = 0; i < len; i++) {
                if (buff[i] == '\r') {
                    continue;
                }
                if (buff[i] == '\n') {
                    pika_platform_printf("\r\n");
                    continue;
                }
                pika_platform_printf("%c", buff[i]);
            }
            pika_platform_printf("\r\n");
            pika_platform_printf("=============== [File] ===============\r\n");
            pika_platform_printf(
                "[   Info] File buff used: %d/%d (%0.2f%%)\r\n", (int)len,
                (int)PIKA_READ_FILE_BUFF_SIZE,
                ((float)len / (float)PIKA_READ_FILE_BUFF_SIZE));
#if PIKA_SHELL_SAVE_FILE_ENABLE
            _save_file(PIKA_SHELL_SAVE_FILE_PATH, (uint8_t*)buff, len);
#endif
            pika_platform_printf("=============== [ Run] ===============\r\n");
            obj_run(self, (char*)buff);
            if (NULL != strstr(buff, "exit()")) {
                is_exit = pika_true;
            }
            pikaFree(buff, PIKA_READ_FILE_BUFF_SIZE);
            if (is_exit) {
                return;
            }
            pika_platform_printf("%s", cfg->prefix);
            continue;
        }

        /* run xx.py.o */
        if (inputChar[0] == 'p' && inputChar[1] == 0x0f) {
            uint8_t magic_code[PIKA_MAGIC_CODE_LEN] = {0x0f, 'p', 0x00, 0x00};
            for (int i = 0; i < 2; i++) {
                /* eat 'yo' */
                magic_code[2 + i] = cfg->fn_getchar();
            }
            uint8_t* recv = NULL;
            uint32_t size = _pikaShell_recv_file(cfg, magic_code, &recv);
            pika_platform_printf(
                "\r\n=============== [File] ===============\r\n");
            pika_platform_printf("[   Info] Recived size: %d\r\n", size);
            if (magic_code[3] == 'o') {
#if PIKA_SHELL_SAVE_BYTECODE_ENABLE
                _save_file(PIKA_SHELL_SAVE_BYTECODE_PATH, (uint8_t*)recv, size);
#endif
                pika_platform_printf(
                    "=============== [ RUN] ===============\r\n");
                pikaVM_runByteCodeInconstant(self, recv);
                pikaFree(recv, size);
                return;
            }
            if (magic_code[3] == 'a') {
                _save_file(PIKA_SHELL_SAVE_APP_PATH, (uint8_t*)recv, size);
                pika_platform_printf(
                    "=============== [REBOOT] ===============\r\n");
                pika_platform_reboot();
                pikaFree(recv, size);
                return;
            }
        }
#endif
        if (SHELL_CTRL_EXIT == _do_obj_runChar(self, inputChar[0], cfg)) {
            break;
        }
    }
}

void _temp__do_pikaScriptShell(PikaObj* self, ShellConfig* shell) {
    /* init the shell */
    _obj_runChar_beforeRun(self, shell);

    /* getchar and run */
    while (1) {
        char inputChar = shell->fn_getchar();
        if (SHELL_CTRL_EXIT == _do_obj_runChar(self, inputChar, shell)) {
            break;
        }
    }
}

static enum shellCTRL __obj_shellLineHandler_REPL(PikaObj* self,
                                                  char* input_line,
                                                  ShellConfig* shell) {
    /* exit */
    if (strEqu("exit()", input_line)) {
        /* exit pika shell */
        return SHELL_CTRL_EXIT;
    }
    /* run single line */
    pikaVM_run_ex_cfg cfg = {0};
    cfg.in_repl = pika_true;
    pikaVM_run_ex(self, input_line, &cfg);
    return SHELL_CTRL_CONTINUE;
}

static volatile ShellConfig g_REPL = {
    .handler = __obj_shellLineHandler_REPL,
    .prefix = ">>> ",
    .blockBuffName = "@sh0",
#if PIKA_SHELL_HISTORY_ENABLE
    .history = NULL,
#endif
    .no_echo = PIKA_SHELL_NO_ECHO,
};

void pikaScriptShell_withGetchar(PikaObj* self, sh_getchar getchar_fn) {
    g_REPL.fn_getchar = getchar_fn;
    _do_pikaScriptShell(self, (ShellConfig*)&g_REPL);
}

int shConfig_deinit(ShellConfig* self) {
#if PIKA_SHELL_HISTORY_ENABLE
    if (NULL != self->history) {
        shHistory_destroy(self->history);
        self->history = NULL;
    }
#endif
    return 0;
}

char pika_repl_getchar(void) {
    char c = 0;
    pika_platform_repl_recv((uint8_t*)&c, 1, PIKA_TIMEOUT_FOREVER);
    return c;
}

void pikaPythonShell(PikaObj* self) {
    pikaScriptShell_withGetchar(self, pika_repl_getchar);
}

void pikaShellSetEcho(pika_bool enable_echo) {
    if (enable_echo) {
        g_REPL.no_echo = pika_false;
    } else {
        g_REPL.no_echo = pika_true;
    }
}

void obj_setErrorCode(PikaObj* self, int32_t errCode) {
    pika_assert(NULL != self->vmFrame);
    self->vmFrame->vm_thread->error_code = errCode;
}

Arg* arg_setObj(Arg* self, char* name, PikaObj* obj) {
    return arg_setPtr(self, name, ARG_TYPE_OBJECT, obj);
}

Arg* arg_setRef(Arg* self, char* name, PikaObj* obj) {
    pika_assert(NULL != obj);
    obj_refcntInc(obj);
    return arg_setObj(self, name, obj);
}

Arg* arg_setWeakRef(Arg* self, char* name, PikaObj* obj) {
    pika_assert(NULL != obj);
    return arg_setPtr(self, name, ARG_TYPE_OBJECT_WEAK, obj);
}

PikaObj* obj_importModuleWithByteCode(PikaObj* self,
                                      char* name,
                                      uint8_t* byteCode) {
    if (!obj_isArgExist((PikaObj*)__pikaMain, name)) {
        /* import to main module context */
        obj_newHostObj((PikaObj*)__pikaMain, name);
        obj_newDirectObj((PikaObj*)__pikaMain, name, New_TinyObj);
        PikaObj* module_obj = obj_getObj((PikaObj*)__pikaMain, name);
        pikaVM_runBytecode_ex_cfg cfg = {0};
        cfg.globals = module_obj;
        cfg.locals = module_obj;
        cfg.name = name;
        cfg.vm_thread = self->vmFrame->vm_thread;
        cfg.is_const_bytecode = pika_true;
        pikaVM_runByteCode_ex(module_obj, byteCode, &cfg);
    }
    if (self != (PikaObj*)__pikaMain) {
        /* import to other module context */
        Arg* aModule = obj_getArg((PikaObj*)__pikaMain, name);
        PikaObj* oModule = arg_getPtr(aModule);
        obj_newHostObj(self, name);
        obj_setArg(self, name, aModule);
        arg_setIsWeakRef(obj_getArg(self, name), pika_true);
        pika_assert(arg_isObject(aModule));
        /* decrase refcnt to avoid circle reference */
        obj_refcntDec(oModule);
    }
    return self;
}

PikaObj* obj_importModuleWithByteCodeFrame(PikaObj* self,
                                           char* name,
                                           ByteCodeFrame *bc_frame) {
    PikaObj* New_PikaStdLib_SysObj(Args * args);
    obj_newDirectObj(self, name, New_PikaStdLib_SysObj);
    pikaVM_runByteCodeFrame(obj_getObj(self, name), bc_frame);
    return self;
}

int obj_linkLibraryFile(PikaObj* self, char* input_file_name) {
    obj_newMetaObj(self, "@lib", New_LibObj);
    LibObj* lib = obj_getObj(self, "@lib");
    return LibObj_loadLibraryFile(lib, input_file_name);
}

PikaObj* obj_linkLibrary(PikaObj* self, uint8_t* library_bytes) {
    obj_newMetaObj(self, "@lib", New_LibObj);
    LibObj* lib = obj_getObj(self, "@lib");
    LibObj_loadLibrary(lib, library_bytes);
    return self;
}

void obj_printModules(PikaObj* self) {
    LibObj* lib = obj_getObj(self, "@lib");
    if (lib == NULL) {
        pika_platform_printf(
            "Error: Not found LibObj, please execute obj_linkLibrary()\r\n");
        return;
    }
    pika_platform_printf(arg_getStr((Arg*)g_PikaObjState.helpModulesCmodule));
    LibObj_printModules(lib);
}

PikaObj* obj_linkLibObj(PikaObj* self, LibObj* library) {
    obj_setRef(self, "@lib", library);
    return self;
}

LibObj* pika_getLibObj(void) {
    // Ensure __pikaMain exists
    if (__pikaMain == NULL) {
        return NULL;
    }

    // Cast __pikaMain to PikaObj and fetch library
    PikaObj* self = (PikaObj*)__pikaMain;
    return obj_getPtr(self, "@lib");
}

uint8_t* pika_getByteCodeFromModule(char* module_name) {
    // Check if module_name is not NULL
    pika_assert(NULL != module_name);

    // Fetch library using pika_getLibObj
    LibObj* lib = pika_getLibObj();

    // Exit if there's no library
    if (NULL == lib) {
        return NULL;
    }

    // Fetch the module from the library
    PikaObj* module = LibObj_getModule(lib, module_name);

    // Check if module exists
    if (NULL == module) {
        return NULL;
    }

    // Return bytecode of the module
    return obj_getPtr(module, "bytecode");
}

int obj_runModule(PikaObj* self, char* module_name) {
    uint8_t* bytecode = pika_getByteCodeFromModule(module_name);
    if (NULL == bytecode) {
        return 1;
    }

    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    pikaVM_runBytecode_ex_cfg cfg = {0};
    cfg.globals = self;
    cfg.locals = self;
    cfg.name = module_name;
    cfg.vm_thread = &vm_thread;
    cfg.is_const_bytecode = pika_true;
    pikaVM_runByteCode_ex(self, bytecode, &cfg);
    return 0;
}

PikaObj* obj_runFile(PikaObj* self, char* file_name) {
    return pikaVM_runFile(self, file_name);
}

PikaObj* obj_runSingleFile(PikaObj* self, char* file_name) {
    return pikaVM_runSingleFile(self, file_name);
}

int obj_importModule(PikaObj* self, char* module_name) {
    if (NULL == module_name) {
        return -1;
    }
    /* import bytecode of the module */
    uint8_t* bytecode = pika_getByteCodeFromModule(module_name);
    if (NULL == bytecode) {
        return -1;
    }
    obj_importModuleWithByteCode(self, module_name, bytecode);
    return 0;
}

PikaObj* arg_getObj(Arg* self) {
    return (PikaObj*)arg_getPtr(self);
}

char* obj_toStr(PikaObj* self) {
    /* check method arg */
    Arg* aMethod = obj_getMethodArgWithFullPath(self, "__str__");
    if (NULL != aMethod) {
        Arg* aStr = obj_runMethodArg0(self, aMethod);
        char* str_res = obj_cacheStr(self, arg_getStr(aStr));
        arg_deinit(aStr);
        return str_res;
    }

    /* normal object */
    Args buffs = {0};
    char* str_res =
        strsFormat(&buffs, PIKA_SPRINTF_BUFF_SIZE, "<object at %p>", self);
    obj_setStr(self, "@res_str", str_res);
    strsDeinit(&buffs);
    return obj_getStr(self, "@res_str");
}

Arg* pika_runFunction1_ex(PikaObj* host, Arg* functionArg, Arg* arg1) {
    obj_setArg(host, "@d", arg1);
    obj_setArg(host, "@f", functionArg);
    /* clang-format off */
    PIKA_PYTHON(
    @r = @f(@d)
    )
    /* clang-format on */
    const uint8_t bytes[] = {
        0x0c, 0x00, 0x00, 0x00, /* instruct array size */
        0x10, 0x81, 0x01, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x04, 0x07, 0x00,
        /* instruct array */
        0x0a, 0x00, 0x00, 0x00, /* const pool size */
        0x00, 0x40, 0x64, 0x00, 0x40, 0x66, 0x00, 0x40, 0x72,
        0x00, /* const pool */
    };
    Arg* res = pikaVM_runByteCodeReturn(host, bytes, "@r");
    arg_deinit(arg1);
    arg_deinit(functionArg);
    return res;
}

Arg* pika_runFunction0_ex(PikaObj* host, Arg* functionArg) {
    obj_setArg(host, "@f", functionArg);
    /* clang-format off */
    PIKA_PYTHON(
    @r = @f()
    )
    /* clang-format on */
    const uint8_t bytes[] = {
        0x08, 0x00, 0x00, 0x00, /* instruct array size */
        0x00, 0x82, 0x01, 0x00, 0x00, 0x04, 0x04, 0x00, /* instruct array */
        0x07, 0x00, 0x00, 0x00,                         /* const pool size */
        0x00, 0x40, 0x66, 0x00, 0x40, 0x72, 0x00,       /* const pool */
    };
    Arg* res = pikaVM_runByteCodeReturn(host, bytes, "@r");
    arg_deinit(functionArg);
    return res;
}

Arg* pika_runFunction1(Arg* functionArg, Arg* arg1) {
    PikaObj* locals = New_TinyObj(NULL);
    Arg* res = pika_runFunction1_ex(locals, functionArg, arg1);
    obj_deinit(locals);
    return res;
}

Arg* pika_runFunction0(Arg* functionArg) {
    PikaObj* locals = New_TinyObj(NULL);
    Arg* res = pika_runFunction0_ex(locals, functionArg);
    obj_deinit(locals);
    return res;
}

Arg* obj_runMethod0(PikaObj* self, char* methodName) {
    Arg* aMethod = obj_getMethodArg(self, methodName);
    if (NULL == aMethod) {
        return NULL;
    }
    Arg* aRes = pika_runFunction0_ex(self, aMethod);
    return aRes;
}

Arg* obj_runMethod1(PikaObj* self, char* methodName, Arg* arg1) {
    Arg* aMethod = obj_getMethodArg(self, methodName);
    if (NULL == aMethod) {
        return NULL;
    }
    Arg* aRes = pika_runFunction1_ex(self, aMethod, arg1);
    return aRes;
}

Arg* obj_runMethodArg0(PikaObj* self, Arg* methodArg) {
    return pika_runFunction0_ex(self, methodArg);
}

Arg* obj_runMethodArg1(PikaObj* self, Arg* methodArg, Arg* arg1) {
    return pika_runFunction1_ex(self, methodArg, arg1);
}

void pika_debug_bytes(uint8_t* buff, size_t len) {
    pika_debug_raw("[");
    for (size_t i = 0; i < len; i++) {
        pika_debug_raw("0x%02X", buff[i]);
        if (i < len - 1) {
            pika_debug_raw(",");
        }
    }
    pika_debug_raw("]\n");
}

/* print major version info */
void pika_printVersion(void) {
    pika_platform_printf("pikascript-core==v%d.%d.%d (%s)\r\n",
                         PIKA_VERSION_MAJOR, PIKA_VERSION_MINOR,
                         PIKA_VERSION_MICRO, PIKA_EDIT_TIME);
}

void pika_getVersion(char* buff) {
    pika_sprintf(buff, "%d.%d.%d", PIKA_VERSION_MAJOR, PIKA_VERSION_MINOR,
                 PIKA_VERSION_MICRO);
}

void* obj_getStruct(PikaObj* self, char* name) {
    pika_assert(self != NULL);
    return args_getStruct(self->list, name);
}

char* obj_cacheStr(PikaObj* self, char* str) {
    pika_assert(self != NULL);
    return args_cacheStr(self->list, str);
}

void _obj_updateProxyFlag(PikaObj* self) {
    if (!obj_getFlag(self, OBJ_FLAG_PROXY_GETATTRIBUTE)) {
        if (NULL != _obj_getPropArg(self, "__getattribute__")) {
            obj_setFlag(self, OBJ_FLAG_PROXY_GETATTRIBUTE);
        }
    }
    if (!obj_getFlag(self, OBJ_FLAG_PROXY_GETATTR)) {
        if (NULL != _obj_getPropArg(self, "__getattr__")) {
            obj_setFlag(self, OBJ_FLAG_PROXY_GETATTR);
        }
    }
    if (!obj_getFlag(self, OBJ_FLAG_PROXY_SETATTR)) {
        if (NULL != _obj_getPropArg(self, "__setattr__")) {
            obj_setFlag(self, OBJ_FLAG_PROXY_SETATTR);
        }
    }
};

PIKA_RES _transeInt(Arg* arg, int base, int64_t* res) {
    ArgType type = arg_getType(arg);
    if (ARG_TYPE_INT == type) {
        *res = arg_getInt(arg);
        return PIKA_RES_OK;
    }
    if (ARG_TYPE_BOOL == type) {
        *res = (int64_t)arg_getBool(arg);
        return PIKA_RES_OK;
    }
    if (ARG_TYPE_FLOAT == type) {
        *res = (int64_t)arg_getFloat(arg);
        return PIKA_RES_OK;
    }
    if (ARG_TYPE_STRING == type) {
        char* end = NULL;
        char* str = arg_getStr(arg);
        *res = strtoll(str, &end, base);
        if (NULL != end && strGetSize(end) != 0) {
            return PIKA_RES_ERR_INVALID_PARAM;
        }
        return PIKA_RES_OK;
    }
    if (ARG_TYPE_BYTES == type) {
        size_t size = arg_getBytesSize(arg);
        if (size != 1) {
            return PIKA_RES_ERR_INVALID_PARAM;
        }
        uint8_t val = *arg_getBytes(arg);
        *res = val;
        return PIKA_RES_OK;
    }
    return PIKA_RES_ERR_INVALID_PARAM;
}

PIKA_RES _transeBool(Arg* arg, pika_bool* res) {
    int64_t iRes = 0;
    if (arg_getType(arg) == ARG_TYPE_BOOL) {
        *res = arg_getBool(arg);
        return PIKA_RES_OK;
    }
    if (arg_getType(arg) == ARG_TYPE_NONE) {
        *res = pika_false;
        return PIKA_RES_OK;
    }
    if (arg_getType(arg) == ARG_TYPE_STRING ||
        arg_getType(arg) == ARG_TYPE_BYTES) {
        if (arg_getStr(arg)[0] == '\0') {
            *res = pika_false;
            return PIKA_RES_OK;
        }
        *res = pika_true;
        return PIKA_RES_OK;
    }
    if (arg_isObject(arg)) {
        int64_t len = obj_getSize(arg_getObj(arg));
        if (len < 0) {
            *res = pika_true;
            return PIKA_RES_OK;
        }
        if (len == 0) {
            *res = pika_false;
            return PIKA_RES_OK;
        }
        *res = pika_true;
        return PIKA_RES_OK;
    }
    if (_transeInt(arg, 10, &iRes) == PIKA_RES_OK) {
        *res = iRes ? pika_true : pika_false;
        return PIKA_RES_OK;
    }
    return PIKA_RES_ERR_INVALID_PARAM;
}

pika_bool arg_isList(Arg* arg) {
    if (!arg_isObject(arg)) {
        return pika_false;
    }
    return arg_getObj(arg)->constructor == New_PikaStdData_List;
}

pika_bool arg_isDict(Arg* arg) {
    if (!arg_isObject(arg)) {
        return pika_false;
    }
    return arg_getObj(arg)->constructor == New_PikaStdData_Dict;
}

pika_bool arg_isTuple(Arg* arg) {
    if (!arg_isObject(arg)) {
        return pika_false;
    }
    return arg_getObj(arg)->constructor == New_PikaStdData_Tuple;
}

int64_t obj_getSize(PikaObj* arg_obj) {
    Arg* aRes = obj_runMethod0(arg_obj, "__len__");
    if (NULL == aRes) {
        return -1;
    }
    int64_t res = arg_getInt(aRes);
    arg_deinit(aRes);
    return res;
}

Arg* _type(Arg* arg) {
    Arg* result;
    PikaObj* oBuiltins = NULL;

    ArgType type = arg_getType(arg);
    oBuiltins = obj_getBuiltins();

    if (ARG_TYPE_INT == type) {
        result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "int"));
        goto __exit;
    }

    if (ARG_TYPE_FLOAT == type) {
        result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "float"));
        goto __exit;
    }

    if (ARG_TYPE_STRING == type) {
        result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "str"));
        goto __exit;
    }

    if (ARG_TYPE_BOOL == type) {
        result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "bool"));
        goto __exit;
    }

    if (ARG_TYPE_BYTES == type) {
        result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "bytes"));
        goto __exit;
    }

    if (argType_isObject(type)) {
        PikaObj* obj = arg_getPtr(arg);
        NewFun clsptr = obj_getClass(obj);
        PikaObj* New_PikaStdData_List(Args * args);

        if (clsptr == New_PikaStdData_List) {
            result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "list"));
            goto __exit;
        }

        PikaObj* New_PikaStdData_Dict(Args * args);

        if (clsptr == New_PikaStdData_Dict) {
            result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "dict"));
            goto __exit;
        }

        PikaObj* New_PikaStdData_Tuple(Args * args);

        if (clsptr == New_PikaStdData_Tuple) {
            result = arg_copy(obj_getMethodArgWithFullPath(oBuiltins, "tuple"));
            goto __exit;
        }

#if PIKA_TYPE_FULL_FEATURE_ENABLE
        Arg* aMethod = obj_getArg(obj, "__class__");

        if (NULL != aMethod) {
            result = arg_copy(aMethod);
            goto __exit;
        }
#endif
        result = arg_newStr("<class 'object'>");
        goto __exit;
    }

    if (ARG_TYPE_OBJECT_META == type) {
        result = arg_newStr("<class 'meta object'>");
        goto __exit;
    }

    if (ARG_TYPE_METHOD_OBJECT == type) {
        result = arg_newStr("<class 'method'>");
        goto __exit;
    }

    if (ARG_TYPE_METHOD_STATIC == type) {
        result = arg_newStr("<class 'function'>");
        goto __exit;
    }

    if (ARG_TYPE_NONE == type) {
        result = arg_newStr("<class 'NoneType'>");
        goto __exit;
    }

    result = arg_newStr("<class 'buitin_function_or_method'>");

__exit:
    if (NULL != oBuiltins) {
        obj_deinit(oBuiltins);
    }
    return result;
}

PikaObj* New_PikaStdData_Tuple(Args* args);
/* clang-format off */
PIKA_PYTHON(
@res_max = @list[0]
for @item in @list:
    if @item > @res_max:
        @res_max = @item
)
PIKA_PYTHON(
@res_max = @list[0]
for @item in @list:
    if @item < @res_max:
        @res_max = @item

)
/* clang-format on */

void _do_vsysOut(char* fmt, va_list args);
void obj_setSysOut(PikaObj* self, char* fmt, ...) {
    if (NULL != self->vmFrame) {
        if (self->vmFrame->vm_thread->error_code == 0) {
            self->vmFrame->vm_thread->error_code = PIKA_RES_ERR_RUNTIME_ERROR;
        }
        if (self->vmFrame->vm_thread->try_state == TRY_STATE_INNER) {
            return;
        }
    }
    va_list args;
    va_start(args, fmt);
    _do_vsysOut(fmt, args);
    va_end(args);
}

pika_bool _bytes_contains(Arg* self, Arg* others) {
    ArgType type = arg_getType(others);
    if (type == ARG_TYPE_BYTES) {
        uint8_t* bytes1 = arg_getBytes(self);
        uint8_t* bytes2 = arg_getBytes(others);
        size_t size1 = arg_getBytesSize(self);
        size_t size2 = arg_getBytesSize(others);

        if (size1 > size2) {
            return pika_false;
        }

        size_t i = 0, j = 0, start_j = 0;
        while (j < size2) {
            if (bytes1[i] == bytes2[j]) {
                i++;
                j++;
                if (i == size1) {
                    return pika_true;
                }
            } else {
                j = ++start_j;  // Move `j` to the next start position
                i = 0;          // Reset `i`
            }
        }
    }
    return pika_false;
}

void pika_sleep_ms(uint32_t ms) {
    int64_t tick = pika_platform_get_tick();
    while (1) {
        pika_platform_thread_yield();
#if PIKA_EVENT_ENABLE
        if (!vm_gil_is_init()) {
            _VMEvent_pickupEvent();
        }
#endif
        if (pika_platform_get_tick() - tick >= ms) {
            break;
        }
    }
}

PIKA_RES insert_label_at_line(const char* filename,
                              int N,
                              char* buff,
                              int buff_size) {
    FILE* file = pika_platform_fopen(filename, "r");
    if (!file) {
        return PIKA_RES_ERR_IO_ERROR;
    }

    int line_count = 1;
    int buff_index = 0;
    char ch;

    while ((pika_platform_fread(&ch, 1, 1, file)) != 0) {
        if (line_count == N) {
            char* label = "#!label\n";
            int label_length = strGetSize(label);

            if (buff_index + label_length >= buff_size) {
                pika_platform_fclose(file);
                return PIKA_RES_ERR_IO_ERROR;  // Insufficient buffer size
            }

            strCopy(buff + buff_index, label);
            buff_index += label_length;
            line_count++;  // Skip this line since we're adding a label
        }

        if (ch == '\n') {
            line_count++;
        }

        if (buff_index >=
            buff_size - 1) {  // -1 to leave space for null terminator
            pika_platform_fclose(file);
            return PIKA_RES_ERR_IO_ERROR;  // Insufficient buffer size
        }

        buff[buff_index] = ch;
        buff_index++;
    }

    buff[buff_index] = '\0';  // Null terminate the buffer

    pika_platform_fclose(file);
    return PIKA_RES_OK;
}

int32_t pika_debug_find_break_point_pc(char* pyFile, uint32_t pyLine) {
    ByteCodeFrame bytecode_frame;
    bc_frame_init(&bytecode_frame);
    char* file_buff = pikaMalloc(PIKA_READ_FILE_BUFF_SIZE);
    FILE* file = pika_platform_fopen(pyFile, "r");
    if (!file) {
        goto __exit;
    }
    if (PIKA_RES_OK != insert_label_at_line(pyFile, pyLine, file_buff,
                                            PIKA_READ_FILE_BUFF_SIZE)) {
        goto __exit;
    }
    pika_lines2Bytes(&bytecode_frame, file_buff);
__exit:
    if (NULL != file) {
        pika_platform_fclose(file);
    }
    bc_frame_deinit(&bytecode_frame);
    pikaFree(file_buff, PIKA_READ_FILE_BUFF_SIZE);
    return bytecode_frame.label_pc;
}
