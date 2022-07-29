#include "BaseObj.h"
#include "PikaVM.h"

void PikaStdTask_Task___init__(PikaObj* self) {
    obj_setInt(self, "is_period", 0);
}

void PikaStdTask_Task_call_always(PikaObj* self, Arg* fun_todo) {
    obj_setArg(self, "fun_todo", fun_todo);
    PIKA_PYTHON_BEGIN
    /* clang-format off */
    PIKA_PYTHON(
        calls.append('always')
        calls.append(fun_todo)
    )
    /* clang-format on */
    const uint8_t bytes[] = {
        0x10, 0x00, /* instruct array size */
        0x10, 0x83, 0x01, 0x00, 0x00, 0x02, 0x08, 0x00, 0x10, 0x81, 0x15,
        0x00, 0x00, 0x02, 0x08, 0x00, /* instruct array */
        0x1e, 0x00,                   /* const pool size */
        0x00, 0x61, 0x6c, 0x77, 0x61, 0x79, 0x73, 0x00, 0x63, 0x61, 0x6c,
        0x6c, 0x73, 0x2e, 0x61, 0x70, 0x70, 0x65, 0x6e, 0x64, 0x00, 0x66,
        0x75, 0x6e, 0x5f, 0x74, 0x6f, 0x64, 0x6f, 0x00, /* const pool */
    };
    PIKA_PYTHON_END
    pikaVM_runByteCode(self, (uint8_t*)bytes);
}

void PikaStdTask_Task_call_when(PikaObj* self, Arg* fun_todo, Arg* fun_when) {
    obj_setArg(self, "fun_todo", fun_todo);
    obj_setArg(self, "fun_when", fun_when);
    PIKA_PYTHON_BEGIN
    /* clang-format off */
    PIKA_PYTHON(
        calls.append('when')
        calls.append(fun_when)
        calls.append(fun_todo)
    )
    /* clang-format on */
    const uint8_t bytes[] = {
        0x18, 0x00, /* instruct array size */
        0x10, 0x83, 0x01, 0x00, 0x00, 0x02, 0x06, 0x00, 0x10, 0x81, 0x13, 0x00,
        0x00, 0x02, 0x06, 0x00, 0x10, 0x81, 0x1c, 0x00, 0x00, 0x02, 0x06, 0x00,
        /* instruct array */
        0x25, 0x00, /* const pool size */
        0x00, 0x77, 0x68, 0x65, 0x6e, 0x00, 0x63, 0x61, 0x6c, 0x6c, 0x73, 0x2e,
        0x61, 0x70, 0x70, 0x65, 0x6e, 0x64, 0x00, 0x66, 0x75, 0x6e, 0x5f, 0x77,
        0x68, 0x65, 0x6e, 0x00, 0x66, 0x75, 0x6e, 0x5f, 0x74, 0x6f, 0x64, 0x6f,
        0x00,
        /* const pool */
    };
    PIKA_PYTHON_END
    pikaVM_runByteCode(self, (uint8_t*)bytes);
}

void PikaStdTask_Task_call_period_ms(PikaObj* self,
                                     Arg* fun_todo,
                                     int period_ms) {
    obj_setArg(self, "fun_todo", fun_todo);
    obj_setInt(self, "period_ms", period_ms);
    PIKA_PYTHON_BEGIN
    /* clang-format off */
    PIKA_PYTHON(
        calls.append('period_ms')
        calls.append(period_ms)
        calls.append(fun_todo)
        calls.append(0)
        is_period = 1
    )
    /* clang-format on */
    const uint8_t bytes[] =
        {
            0x28, 0x00, /* instruct array size */
            0x10, 0x83, 0x01, 0x00, 0x00, 0x02, 0x0b, 0x00, 0x10, 0x81, 0x01,
            0x00, 0x00, 0x02, 0x0b, 0x00, 0x10, 0x81, 0x18, 0x00, 0x00, 0x02,
            0x0b, 0x00, 0x10, 0x85, 0x21, 0x00, 0x00, 0x02, 0x0b, 0x00, 0x00,
            0x85, 0x23, 0x00, 0x00, 0x04, 0x25, 0x00, /* instruct array */
            0x2f, 0x00,                               /* const pool size */
            0x00, 0x70, 0x65, 0x72, 0x69, 0x6f, 0x64, 0x5f, 0x6d, 0x73, 0x00,
            0x63, 0x61, 0x6c, 0x6c, 0x73, 0x2e, 0x61, 0x70, 0x70, 0x65, 0x6e,
            0x64, 0x00, 0x66, 0x75, 0x6e, 0x5f, 0x74, 0x6f, 0x64, 0x6f, 0x00,
            0x30, 0x00, 0x31, 0x00, 0x69, 0x73, 0x5f, 0x70, 0x65, 0x72, 0x69,
            0x6f, 0x64, 0x00, /* const pool */
        };
    PIKA_PYTHON_END
    pikaVM_runByteCode(self, (uint8_t*)bytes);
}

void PikaStdTask_Task_run_once(PikaObj* self) {
    /* clang-format off */
    PIKA_PYTHON(
    len = calls.len()
    mode = 'none'
    info_index = 0
    for i in range(0, len):
        if len == 0:
            break
        if info_index == 0:
            mode = calls[i]
            info_index = 1
        elif info_index == 1:
            if mode == 'always':
                todo = calls[i]
                todo()
                info_index = 0
            elif mode == 'when':
                when = calls[i]
                info_index = 2
            elif mode == 'period_ms':
                period_ms = calls[i]
                info_index = 2
        elif info_index == 2:
            if mode == 'when':
                if when():
                    todo = calls[i]
                    todo()
                info_index = 0
            elif mode == 'period_ms':
                todo = calls[i]
                info_index = 3
        elif info_index == 3:
            if mode == 'period_ms':
                if tick > calls[i]:
                    todo()
                    calls[i] = tick + period_ms
                info_index = 0
    )
    /* clang-format on */
    const uint8_t bytes[] = {
        0xf0, 0x01, /* instruct array size */
        0x00, 0x82, 0x01, 0x00, 0x00, 0x04, 0x0b, 0x00, 0x00, 0x83, 0x0f, 0x00,
        0x00, 0x04, 0x14, 0x00, 0x00, 0x85, 0x19, 0x00, 0x00, 0x04, 0x1b, 0x00,
        0x20, 0x85, 0x19, 0x00, 0x20, 0x01, 0x0b, 0x00, 0x10, 0x02, 0x26, 0x00,
        0x00, 0x02, 0x2c, 0x00, 0x00, 0x04, 0x31, 0x00, 0x00, 0x82, 0x35, 0x00,
        0x00, 0x04, 0x42, 0x00, 0x00, 0x0d, 0x42, 0x00, 0x00, 0x07, 0x44, 0x00,
        0x11, 0x81, 0x0b, 0x00, 0x11, 0x05, 0x19, 0x00, 0x01, 0x08, 0x46, 0x00,
        0x01, 0x07, 0x49, 0x00, 0x02, 0x8e, 0x00, 0x00, 0x11, 0x81, 0x1b, 0x00,
        0x11, 0x05, 0x19, 0x00, 0x01, 0x08, 0x46, 0x00, 0x01, 0x07, 0x49, 0x00,
        0x12, 0x81, 0x4b, 0x00, 0x12, 0x01, 0x42, 0x00, 0x02, 0x1d, 0x00, 0x00,
        0x02, 0x04, 0x14, 0x00, 0x02, 0x85, 0x49, 0x00, 0x02, 0x04, 0x1b, 0x00,
        0x01, 0x8b, 0x49, 0x00, 0x11, 0x01, 0x1b, 0x00, 0x11, 0x05, 0x49, 0x00,
        0x01, 0x08, 0x46, 0x00, 0x01, 0x07, 0x49, 0x00, 0x12, 0x81, 0x14, 0x00,
        0x12, 0x03, 0x51, 0x00, 0x02, 0x08, 0x46, 0x00, 0x02, 0x07, 0x49, 0x00,
        0x13, 0x81, 0x4b, 0x00, 0x13, 0x01, 0x42, 0x00, 0x03, 0x1d, 0x00, 0x00,
        0x03, 0x04, 0x58, 0x00, 0x03, 0x82, 0x58, 0x00, 0x03, 0x85, 0x19, 0x00,
        0x03, 0x04, 0x1b, 0x00, 0x02, 0x8b, 0x49, 0x00, 0x12, 0x01, 0x14, 0x00,
        0x12, 0x03, 0x5d, 0x00, 0x02, 0x08, 0x46, 0x00, 0x02, 0x07, 0x49, 0x00,
        0x13, 0x81, 0x4b, 0x00, 0x13, 0x01, 0x42, 0x00, 0x03, 0x1d, 0x00, 0x00,
        0x03, 0x04, 0x5d, 0x00, 0x03, 0x85, 0x44, 0x00, 0x03, 0x04, 0x1b, 0x00,
        0x02, 0x8b, 0x49, 0x00, 0x12, 0x01, 0x14, 0x00, 0x12, 0x03, 0x62, 0x00,
        0x02, 0x08, 0x46, 0x00, 0x02, 0x07, 0x49, 0x00, 0x13, 0x81, 0x4b, 0x00,
        0x13, 0x01, 0x42, 0x00, 0x03, 0x1d, 0x00, 0x00, 0x03, 0x04, 0x62, 0x00,
        0x03, 0x85, 0x44, 0x00, 0x03, 0x04, 0x1b, 0x00, 0x01, 0x8b, 0x49, 0x00,
        0x11, 0x01, 0x1b, 0x00, 0x11, 0x05, 0x44, 0x00, 0x01, 0x08, 0x46, 0x00,
        0x01, 0x07, 0x49, 0x00, 0x12, 0x81, 0x14, 0x00, 0x12, 0x03, 0x5d, 0x00,
        0x02, 0x08, 0x46, 0x00, 0x02, 0x07, 0x49, 0x00, 0x03, 0x82, 0x5d, 0x00,
        0x03, 0x07, 0x49, 0x00, 0x14, 0x81, 0x4b, 0x00, 0x14, 0x01, 0x42, 0x00,
        0x04, 0x1d, 0x00, 0x00, 0x04, 0x04, 0x58, 0x00, 0x04, 0x82, 0x58, 0x00,
        0x03, 0x85, 0x19, 0x00, 0x03, 0x04, 0x1b, 0x00, 0x02, 0x8b, 0x49, 0x00,
        0x12, 0x01, 0x14, 0x00, 0x12, 0x03, 0x62, 0x00, 0x02, 0x08, 0x46, 0x00,
        0x02, 0x07, 0x49, 0x00, 0x13, 0x81, 0x4b, 0x00, 0x13, 0x01, 0x42, 0x00,
        0x03, 0x1d, 0x00, 0x00, 0x03, 0x04, 0x58, 0x00, 0x03, 0x85, 0x6c, 0x00,
        0x03, 0x04, 0x1b, 0x00, 0x01, 0x8b, 0x49, 0x00, 0x11, 0x01, 0x1b, 0x00,
        0x11, 0x05, 0x6c, 0x00, 0x01, 0x08, 0x46, 0x00, 0x01, 0x07, 0x49, 0x00,
        0x12, 0x81, 0x14, 0x00, 0x12, 0x03, 0x62, 0x00, 0x02, 0x08, 0x46, 0x00,
        0x02, 0x07, 0x49, 0x00, 0x13, 0x81, 0x6e, 0x00, 0x23, 0x01, 0x4b, 0x00,
        0x23, 0x01, 0x42, 0x00, 0x13, 0x1d, 0x00, 0x00, 0x03, 0x08, 0x73, 0x00,
        0x03, 0x07, 0x49, 0x00, 0x04, 0x82, 0x58, 0x00, 0x14, 0x81, 0x4b, 0x00,
        0x14, 0x01, 0x42, 0x00, 0x24, 0x01, 0x6e, 0x00, 0x24, 0x01, 0x62, 0x00,
        0x14, 0x08, 0x75, 0x00, 0x04, 0x02, 0x77, 0x00, 0x04, 0x04, 0x4b, 0x00,
        0x03, 0x85, 0x19, 0x00, 0x03, 0x04, 0x1b, 0x00, 0x00, 0x86, 0x83, 0x00,
        0x00, 0x8c, 0x31, 0x00, /* instruct array */
        0x86, 0x00,             /* const pool size */
        0x00, 0x63, 0x61, 0x6c, 0x6c, 0x73, 0x2e, 0x6c, 0x65, 0x6e, 0x00, 0x6c,
        0x65, 0x6e, 0x00, 0x6e, 0x6f, 0x6e, 0x65, 0x00, 0x6d, 0x6f, 0x64, 0x65,
        0x00, 0x30, 0x00, 0x69, 0x6e, 0x66, 0x6f, 0x5f, 0x69, 0x6e, 0x64, 0x65,
        0x78, 0x00, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x00, 0x69, 0x74, 0x65, 0x72,
        0x00, 0x5f, 0x6c, 0x30, 0x00, 0x5f, 0x6c, 0x30, 0x2e, 0x5f, 0x5f, 0x6e,
        0x65, 0x78, 0x74, 0x5f, 0x5f, 0x00, 0x69, 0x00, 0x32, 0x00, 0x3d, 0x3d,
        0x00, 0x31, 0x00, 0x63, 0x61, 0x6c, 0x6c, 0x73, 0x00, 0x61, 0x6c, 0x77,
        0x61, 0x79, 0x73, 0x00, 0x74, 0x6f, 0x64, 0x6f, 0x00, 0x77, 0x68, 0x65,
        0x6e, 0x00, 0x70, 0x65, 0x72, 0x69, 0x6f, 0x64, 0x5f, 0x6d, 0x73, 0x00,
        0x33, 0x00, 0x74, 0x69, 0x63, 0x6b, 0x00, 0x3e, 0x00, 0x2b, 0x00, 0x5f,
        0x5f, 0x73, 0x65, 0x74, 0x69, 0x74, 0x65, 0x6d, 0x5f, 0x5f, 0x00, 0x2d,
        0x31, 0x00, /* const pool */
    };
    pikaVM_runByteCode(self, (uint8_t*)bytes);
}

void __Task_update_tick(PikaObj* self) {
    if (obj_getInt(self, "is_perod")) {
        obj_runNativeMethod(self, "platformGetTick", NULL);
    }
}

void PikaStdTask_Task_run_forever(PikaObj* self) {
    while (1) {
        __Task_update_tick(self);
        PikaStdTask_Task_run_once(self);
    }
}

void PikaStdTask_Task_run_until_ms(PikaObj* self, int until_ms) {
    while (1) {
        __Task_update_tick(self);
        PikaStdTask_Task_run_once(self);
        if (obj_getInt(self, "tick") > until_ms) {
            return;
        }
    }
}

void PikaStdTask_Task_platformGetTick(PikaObj* self) {
    obj_setErrorCode(self, 1);
    obj_setSysOut(self, "[error] platform method need to be override.");
}
