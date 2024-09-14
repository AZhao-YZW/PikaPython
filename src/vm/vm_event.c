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
#include "vm_event.h"
#include "PikaVM.h"

extern volatile VMState g_PikaVMState;

int vm_event_get_vm_cnt(void)
{
    return g_PikaVMState.vm_cnt;
}

int vm_event_get_event_pickup_cnt(void)
{
#if !PIKA_EVENT_ENABLE
    return -1;
#else
    return g_PikaVMState.event_pickup_cnt;
#endif
}

#if PIKA_EVENT_ENABLE
static pika_bool _ecq_isEmpty(volatile PikaEventQueue* cq)
{
    return (pika_bool)(cq->head == cq->tail);
}

static pika_bool _ecq_isFull(volatile PikaEventQueue* cq)
{
    return (pika_bool)((cq->tail + 1) % PIKA_EVENT_LIST_SIZE == cq->head);
}
#endif

void poiner_set_NULL(void **p)
{
    if(NULL != *p) {
        arg_deinit(*p);
        *p = NULL;
    }
}

void pika_event_not_enable(void)
{
    pika_platform_printf("PIKA_EVENT_ENABLE is not enable\r\n");
    pika_platform_panic_handle();
}

void vm_event_deinit(void)
{
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
#else
    for (int i = 0; i < PIKA_EVENT_LIST_SIZE; i++) {
        poiner_set_NULL((void**)&g_PikaVMState.cq.res[i]);
        poiner_set_NULL((void**)&g_PikaVMState.cq.data[i].arg);
        poiner_set_NULL((void**)&g_PikaVMState.sq.res[i]);
        g_PikaVMState.cq.id[i] = 0;
        g_PikaVMState.cq.listener[i] = NULL;
        g_PikaVMState.sq.id[i] = 0;
        g_PikaVMState.sq.data[i].signal = 0;
        g_PikaVMState.sq.listener[i] = NULL;
    }
    g_PikaVMState.cq.head = 0;
    g_PikaVMState.cq.tail = 0;
    g_PikaVMState.sq.head = 0;
    g_PikaVMState.sq.tail = 0;
    if (NULL != g_PikaVMState.event_thread) {
        g_PikaVMState.event_thread_exit = 1;
        pika_platform_thread_destroy(g_PikaVMState.event_thread);
        g_PikaVMState.event_thread = NULL;
        vm_gil_exit();
        while (!g_PikaVMState.event_thread_exit_done) {
            pika_platform_thread_yield();
        }
        g_PikaVMState.event_thread_exit = 0;
        g_PikaVMState.event_thread_exit_done = 0;
        vm_gil_enter();
    }
#endif
}

PIKA_RES event_listener_push_event(PikaEventListener* lisener, uintptr_t eventId, Arg* eventData)
{
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    if (arg_getType(eventData) == ARG_TYPE_OBJECT_NEW) {
        arg_setType(eventData, ARG_TYPE_OBJECT);
    }
    /* push to event_cq_buff */
    if (_ecq_isFull(&g_PikaVMState.cq)) {
        // pika_debug("event_cq_buff is full");
        arg_deinit(eventData);
        return PIKA_RES_ERR_SIGNAL_EVENT_FULL;
    }
    poiner_set_NULL((void**)&g_PikaVMState.cq.res[g_PikaVMState.cq.tail]);
    poiner_set_NULL((void**)&g_PikaVMState.cq.data[g_PikaVMState.cq.tail].arg);
    g_PikaVMState.cq.id[g_PikaVMState.cq.tail] = eventId;
    g_PikaVMState.cq.data[g_PikaVMState.cq.tail].arg = eventData;
    g_PikaVMState.cq.listener[g_PikaVMState.cq.tail] = lisener;
    g_PikaVMState.cq.tail = (g_PikaVMState.cq.tail + 1) % PIKA_EVENT_LIST_SIZE;
    return PIKA_RES_OK;
#endif
}

PIKA_RES event_listener_push_signal(PikaEventListener* lisener, uintptr_t eventId, int signal)
{
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    /* push to event_cq_buff */
    if (_ecq_isFull(&g_PikaVMState.sq)) {
        // pika_debug("event_cq_buff is full");
        return PIKA_RES_ERR_SIGNAL_EVENT_FULL;
    }
    g_PikaVMState.sq.id[g_PikaVMState.sq.tail] = eventId;
    g_PikaVMState.sq.data[g_PikaVMState.sq.tail].signal = signal;
    g_PikaVMState.sq.listener[g_PikaVMState.sq.tail] = lisener;
    g_PikaVMState.sq.tail = (g_PikaVMState.sq.tail + 1) % PIKA_EVENT_LIST_SIZE;
    if (vm_gil_is_first_lock()) {
        vm_gil_enter();
        poiner_set_NULL((void**)&g_PikaVMState.sq.res[g_PikaVMState.sq.tail]);
        vm_gil_exit();
    }
    return PIKA_RES_OK;
#endif
}

PIKA_RES event_listener_pop_event(PikaEventListener** lisener_p, uintptr_t* id,
    Arg** data, int* signal, int* head)
{
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    PikaEventQueue* cq = NULL;
    /* pop from event_cq_buff */
    if (!_ecq_isEmpty(&g_PikaVMState.cq)) {
        cq = (PikaEventQueue*)&g_PikaVMState.cq;
    } else if (!_ecq_isEmpty(&g_PikaVMState.sq)) {
        cq = (PikaEventQueue*)&g_PikaVMState.sq;
    }
    if (NULL == cq) {
        return PIKA_RES_ERR_SIGNAL_EVENT_EMPTY;
    }
    *id = cq->id[g_PikaVMState.cq.head];
    if (cq == &g_PikaVMState.cq) {
        *data = cq->data[g_PikaVMState.cq.head].arg;
    } else {
        *signal = cq->data[g_PikaVMState.cq.head].signal;
        *data = NULL;
    }
    *lisener_p = cq->listener[g_PikaVMState.cq.head];
    *head = cq->head;
    cq->head = (cq->head + 1) % PIKA_EVENT_LIST_SIZE;
    return PIKA_RES_OK;
#endif
}

PIKA_RES event_listener_pop_signal_event(PikaEventListener** lisener_p,
    uintptr_t* id, int* signal, int* head)
{
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    /* pop from event_cq_buff */
    if (_ecq_isEmpty(&g_PikaVMState.sq)) {
        return PIKA_RES_ERR_SIGNAL_EVENT_EMPTY;
    }
    *id = g_PikaVMState.sq.id[g_PikaVMState.sq.head];
    *signal = g_PikaVMState.sq.data[g_PikaVMState.sq.head].signal;
    *lisener_p = g_PikaVMState.sq.listener[g_PikaVMState.sq.head];
    *head = g_PikaVMState.sq.head;
    g_PikaVMState.sq.head = (g_PikaVMState.sq.head + 1) % PIKA_EVENT_LIST_SIZE;
    return PIKA_RES_OK;
#endif
}

void vm_event_pickup_event(char *info)
{
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
#else
    int evt_pickup_cnt = vm_event_get_event_pickup_cnt();
    if (evt_pickup_cnt >= PIKA_EVENT_PICKUP_MAX) {
        return;
    }
    PikaObj *event_listener;
    uintptr_t event_id;
    Arg* event_data;
    int signal;
    int head;
    if (PIKA_RES_OK == event_listener_pop_event(&event_listener, &event_id,
                                                &event_data, &signal, &head)) {
        g_PikaVMState.event_pickup_cnt++;
        pika_debug("pickup_info: %s", info);
        pika_debug("pickup_cnt: %d", g_PikaVMState.event_pickup_cnt);
        Arg* res = NULL;
        if (NULL != event_data) {
            res =
                __eventListener_runEvent(event_listener, event_id, event_data);
        } else {
            event_data = arg_newInt(signal);
            res =
                __eventListener_runEvent(event_listener, event_id, event_data);
            arg_deinit(event_data);
            event_data = NULL;
        }
        // only on muti thread, keep the res
        if (vm_gil_is_first_lock()) {
            if (NULL == event_data) {
                // from signal
                g_PikaVMState.sq.res[head] = res;
            } else {
                // from event
                g_PikaVMState.cq.res[head] = res;
            }
        } else {
            if (NULL != res) {
                arg_deinit(res);
            }
        }
        g_PikaVMState.event_pickup_cnt--;
    }
#endif
}