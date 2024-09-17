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
#include "obj_event.h"
#include "PikaObj.h"
#include "BaseObj.h"

extern volatile VMState g_PikaVMState;

void pika_eventListener_registEventHandler(PikaEventListener* self,
                                           uintptr_t eventId,
                                           PikaObj* eventHandleObj) {
    Args buffs = {0};
    char* event_name =
        strsFormat(&buffs, PIKA_SPRINTF_BUFF_SIZE, "%ld", eventId);
    obj_newDirectObj(self, event_name, New_TinyObj);
    PikaObj* event_item = obj_getObj(self, event_name);
    obj_setRef(event_item, "eventHandleObj", eventHandleObj);
    strsDeinit(&buffs);
}

PIKA_RES obj_setEventCallback(PikaObj* self,
                              uintptr_t eventId,
                              Arg* eventCallback,
                              PikaEventListener* listener) {
    obj_setArg(self, "eventCallBack", eventCallback);
    if (argType_isObjectMethodActive(arg_getType(eventCallback))) {
        PikaObj* method_self = methodArg_getHostObj(eventCallback);
        obj_setRef(self, "eventCallBackHost", method_self);
    }
    pika_eventListener_registEventHandler(listener, eventId, self);
    return PIKA_RES_OK;
}

void pika_eventListener_registEventCallback(PikaEventListener* listener,
                                            uintptr_t eventId,
                                            Arg* eventCallback) {
    pika_assert(NULL != listener);
    char hash_str[32] = {0};
    pika_sprintf(hash_str, "C%lu", eventId);
    obj_newDirectObj(listener, hash_str, New_TinyObj);
    PikaObj* oHandle = obj_getPtr(listener, hash_str);
    obj_setEventCallback(oHandle, eventId, eventCallback, listener);
}

void pika_eventListener_removeEvent(PikaEventListener* self,
                                    uintptr_t eventId) {
    Args buffs = {0};
    char* event_name = strsFormat(&buffs, PIKA_SPRINTF_BUFF_SIZE, "%ld", eventId);
    obj_removeArg(self, event_name);
    strsDeinit(&buffs);
}

PikaObj* pika_eventListener_getEventHandleObj(PikaEventListener* self,
                                              uintptr_t eventId) {
    Args buffs = {0};
    char* event_name = strsFormat(&buffs, PIKA_SPRINTF_BUFF_SIZE, "%ld", eventId);
    PikaObj* event_item = obj_getObj(self, event_name);
    PikaObj* eventHandleObj = obj_getPtr(event_item, "eventHandleObj");
    strsDeinit(&buffs);
    return eventHandleObj;
}

void pika_eventListener_init(PikaEventListener** p_self) {
    *p_self = newNormalObj(New_TinyObj);
}

void pika_eventListener_deinit(PikaEventListener** p_self) {
    if (NULL != *p_self) {
        obj_deinit(*p_self);
        *p_self = NULL;
    }
}

Arg* __eventListener_runEvent(PikaEventListener* listener,
                              uintptr_t eventId,
                              Arg* eventData) {
    PikaObj* handler = pika_eventListener_getEventHandleObj(listener, eventId);
    pika_debug("event handler: %p", handler);
    if (NULL == handler) {
        pika_platform_printf(
            "Error: can not find event handler by id: [0x%02lx]\r\n", eventId);
        return NULL;
    }
    Arg* eventCallBack = obj_getArg(handler, "eventCallBack");
    pika_debug("run event handler: %p", handler);
    Arg* res = pika_runFunction1(arg_copy(eventCallBack), arg_copy(eventData));
    return res;
}

static void _thread_event(void* arg) {
    pika_assert(vm_gil_is_first_lock());
    while (1) {
        vm_gil_enter();
#if PIKA_EVENT_ENABLE
        if (g_PikaVMState.event_thread_exit) {
            g_PikaVMState.event_thread_exit_done = 1;
            break;
        }
#endif
        _VMEvent_pickupEvent();
        vm_gil_exit();
        pika_platform_thread_yield();
    }
    vm_gil_exit();
}

PIKA_RES _do_pika_eventListener_send(PikaEventListener* self,
                                     uintptr_t eventId,
                                     Arg* eventData,
                                     int eventSignal,
                                     PIKA_BOOL pickupWhenNoVM) {
#if !PIKA_EVENT_ENABLE
    pika_platform_printf("PIKA_EVENT_ENABLE is not enable");
    while (1) {
    };
#else
    if (NULL != eventData && !vm_gil_is_first_lock()) {
#if PIKA_EVENT_THREAD_ENABLE
        vm_gil_init();
#else
        pika_platform_printf(
            "Error: can not send arg event data without thread support\r\n");
        arg_deinit(eventData);
        return PIKA_RES_ERR_RUNTIME_ERROR;
#endif
    }
    if (NULL == eventData) {
        // for event signal
        if (PIKA_RES_OK !=
            event_listener_push_signal(self, eventId, eventSignal)) {
            return PIKA_RES_ERR_RUNTIME_ERROR;
        }
    }
    /* using multi thread */
    if (vm_gil_is_init()) {
        /* python thread is running */
        /* wait python thread get first lock */
        while (1) {
            if (vm_gil_is_first_lock()) {
                break;
            }
            if (g_PikaVMState.vm_cnt == 0) {
                break;
            }
            if (vm_gil_get_bare_lock() == 0) {
                break;
            }
            pika_platform_thread_yield();
        }
        vm_gil_enter();
#if PIKA_EVENT_THREAD_ENABLE
        if (!g_PikaVMState.event_thread) {
            // avoid _VMEvent_pickupEvent() in _time.c as soon as
            // possible
            g_PikaVMState.event_thread = pika_platform_thread_init(
                "pika_event", _thread_event, NULL, PIKA_EVENT_THREAD_STACK_SIZE,
                PIKA_THREAD_PRIO, PIKA_THREAD_TICK);
            pika_debug("event thread init");
        }
#endif

        if (NULL != eventData) {
            if (PIKA_RES_OK !=
                event_listener_push_event(self, eventId, eventData)) {
                goto __gil_exit;
            }
        }

        if (pickupWhenNoVM) {
            int vmCnt = vm_event_get_vm_cnt();
            if (0 == vmCnt) {
                /* no vm running, pick up event imediately */
                pika_debug("vmCnt: %d, pick up imediately", vmCnt);
                _VMEvent_pickupEvent();
            }
        }
    __gil_exit:
        vm_gil_exit();
    }
    return (PIKA_RES)0;
#endif
}

PIKA_RES pika_eventListener_send(PikaEventListener* self,
                                 uintptr_t eventId,
                                 Arg* eventData) {
    return _do_pika_eventListener_send(self, eventId, eventData, 0, pika_false);
}

PIKA_RES pika_eventListener_sendSignal(PikaEventListener* self,
                                       uintptr_t eventId,
                                       int eventSignal) {
    return _do_pika_eventListener_send(self, eventId, NULL, eventSignal,
                                       pika_false);
}

Arg* pika_eventListener_sendSignalAwaitResult(PikaEventListener* self,
                                              uintptr_t eventId,
                                              int eventSignal) {
    /*
     * Await result from event.
     * need implement `pika_platform_thread_delay()` to support thread switch */
#if !PIKA_EVENT_ENABLE
    pika_platform_printf("PIKA_EVENT_ENABLE is not enable");
    while (1) {
    };
#else
    extern volatile VMState g_PikaVMState;
    int tail = g_PikaVMState.cq.tail;
    pika_eventListener_sendSignal(self, eventId, eventSignal);
    while (1) {
        Arg* res = g_PikaVMState.cq.res[tail];
        pika_platform_thread_yield();
        if (NULL != res) {
            return res;
        }
    }
#endif
}

Arg* pika_eventListener_syncSendAwaitResult(PikaEventListener* self,
                                            uintptr_t eventId,
                                            Arg* eventData) {
    return __eventListener_runEvent(self, eventId, eventData);
}

PIKA_RES pika_eventListener_syncSend(PikaEventListener* self,
                                     uintptr_t eventId,
                                     Arg* eventData) {
    Arg* res = __eventListener_runEvent(self, eventId, eventData);
    PIKA_RES ret = PIKA_RES_OK;
    if (NULL == res) {
        ret = PIKA_RES_ERR_RUNTIME_ERROR;
    } else {
        arg_deinit(res);
    }
    arg_deinit(eventData);
    return ret;
}

PIKA_RES pika_eventListener_syncSendSignal(PikaEventListener* self,
                                           uintptr_t eventId,
                                           int eventSignal) {
    Arg* eventData = arg_newInt(eventSignal);
    PIKA_RES res = pika_eventListener_syncSend(self, eventId, eventData);
    return res;
}

Arg* pika_eventListener_syncSendSignalAwaitResult(PikaEventListener* self,
                                                  uintptr_t eventId,
                                                  int eventSignal) {
    Arg* eventData = arg_newInt(eventSignal);
    Arg* ret = pika_eventListener_syncSendAwaitResult(self, eventId, eventData);
    arg_deinit(eventData);
    return ret;
}
