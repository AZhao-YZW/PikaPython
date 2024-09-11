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
#include "inst.h"
#include "const_pool.h"
#include "PikaVM.h"
#include "inst_proc.h"

#if PIKA_INSTRUCT_EXTENSION_ENABLE
const VMInstructionSet VM_default_instruction_set = {
#define __INS_OPCODE
    .instructions =
        (const VMInstruction[]){
#include "__instruction_table.h"
        },
    .count = __INSTRUCTION_CNT,
    .op_idx_start = PIKA_INS(NON),
    .op_idx_end = PIKA_INS(NON) + __INSTRUCTION_CNT - 1,
};

#ifndef PIKA_INSTRUCT_SIGNATURE_DICT
#define PIKA_INSTRUCT_SIGNATURE_DICT 0
#endif

typedef struct VMInstructionSetItem VMInstructionSetItem;
struct VMInstructionSetItem {
    VMInstructionSetItem* next;
    const VMInstructionSet* ins_set;
};

static struct {
    const VMInstructionSetItem default_ins_set;
    VMInstructionSetItem* list;
    VMInstructionSetItem* recent;
#if PIKA_INSTRUCT_SIGNATURE_DICT_COUNT > 0
    const uint16_t signature_dict[PIKA_INSTRUCT_SIGNATURE_DICT_COUNT];
#endif
} g_PikaVMInsSet = {
    .default_ins_set =
        {
            .ins_set = &VM_default_instruction_set,
            .next = NULL,
        },
    .list = (VMInstructionSetItem*)&g_PikaVMInsSet.default_ins_set,
    .recent = (VMInstructionSetItem*)&g_PikaVMInsSet.default_ins_set,
#if PIKA_INSTRUCT_SIGNATURE_DICT_COUNT > 0
    .signature_dict = {PIKA_INSTRUCT_SIGNATURE_DICT},
#endif
};

pika_bool pikaVM_registerInstructionSet(VMInstructionSet* ins_set) {
    pika_assert(NULL != ins_set);

#if PIKA_INSTRUCT_SIGNATURE_DICT_COUNT > 0
    uint16_t signature = ins_set->signature;

    pika_bool ins_set_valid = pika_false;
    for (int n = 0;
         n < sizeof(g_PikaVMInsSet.signature_dict) / sizeof(uint16_t); n++) {
        if (g_PikaVMInsSet.signature_dict[n] == signature) {
            ins_set_valid = pika_true;
            break;
        }
    }
    if (!ins_set_valid) {
        return pika_false;
    }
#endif

    /* check whether the target instruction set exists or not */
    VMInstructionSetItem* list_item = g_PikaVMInsSet.list;
    do {
        if (list_item->ins_set->signature == signature) {
            return pika_true; /* already exist */
        }

        list_item = list_item->next;
    } while (NULL != list_item->next);

    VMInstructionSetItem* item =
        pika_platform_malloc(sizeof(VMInstructionSetItem));
    if (NULL == item) {
        return pika_false;
    }
    item->ins_set = ins_set;
    item->next = NULL;

    /* add item to the tail of VM.list */
    list_item->next = item;

    return pika_true;
}

const VMInstruction* inst_unit_get_inst(enum InstructIndex ins_idx)
{
    VMInstructionSetItem* item = g_PikaVMInsSet.recent;

    if ((ins_idx >= item->ins_set->op_idx_start) &&
        (ins_idx <= item->ins_set->op_idx_end)) {
        return &(
            item->ins_set->instructions[ins_idx - item->ins_set->op_idx_start]);
    }

    /* search list */
    item = g_PikaVMInsSet.list;
    do {
        if ((ins_idx >= item->ins_set->op_idx_start) &&
            (ins_idx <= item->ins_set->op_idx_end)) {
            g_PikaVMInsSet.recent = item;
            return &(item->ins_set
                         ->instructions[ins_idx - item->ins_set->op_idx_start]);
        }
        item = item->next;
    } while (NULL != item->next);

    return NULL;
}

static enum InstructIndex __find_ins_idx_in_ins_set(
    char *ins_str,
    const VMInstructionSet* set) {
    const VMInstruction* ins = set->instructions;
    uint_fast16_t count = set->count;

    do {
        if (0 == strncmp(ins_str, ins->op_str, ins->op_str_len)) {
            return (enum InstructIndex)ins->op_idx;
        }
        ins++;
    } while (--count);
    return __INSTRUCTION_UNKNOWN;
}

enum InstructIndex inst_get_index_from_asm(char *ins_str)
{
    enum InstructIndex ins_idx =
        __find_ins_idx_in_ins_set(ins_str, g_PikaVMInsSet.recent->ins_set);

    if (__INSTRUCTION_UNKNOWN == ins_idx) {
        VMInstructionSetItem* item = g_PikaVMInsSet.list;

        do {
            ins_idx = __find_ins_idx_in_ins_set(ins_str, item->ins_set);
            if (__INSTRUCTION_UNKNOWN != ins_idx) {
                g_PikaVMInsSet.recent = item;
                return ins_idx;
            }
            item = item->next;
        } while (NULL != item->next);

        return PIKA_INS(NON);
    }

    return ins_idx;
}

#else

pika_bool pikaVM_registerInstructionSet(VMInstructionSet* ins_set) {
    return pika_false;
}

enum InstructIndex inst_get_index_from_asm(char *ins_str) {
#define __INS_COMPARE
#include "__instruction_table.h"
    return NON;
}

const VM_instruct_handler VM_instruct_handler_table[__INSTRUCTION_CNT] = {
#define __INS_TABLE
#include "__instruction_table.h"
};
#endif

void inst_array_init(InstructArray *self)
{
    self->arg_buff = NULL;
    self->content_start = NULL;
    self->size = 0;
    self->content_offset_now = 0;
    self->output_redirect_fun = NULL;
    self->output_f = NULL;
}

void inst_array_deinit(InstructArray *self)
{
    if (NULL != self->arg_buff) {
        arg_deinit(self->arg_buff);
    }
}

static void inst_array_update(InstructArray *self)
{
    self->content_start = (void*)arg_getContent(self->arg_buff);
}

void inst_array_append(InstructArray *self, InstructUnit *ins_unit)
{
    if (NULL == self->arg_buff) {
        self->arg_buff = arg_newNone();
    }
    if (NULL == self->output_redirect_fun) {
        self->arg_buff =
            arg_append(self->arg_buff, ins_unit, inst_unit_get_size());
    } else {
        self->output_redirect_fun(self, ins_unit);
    };
    inst_array_update(self);
    self->size += inst_unit_get_size();
}

void inst_unit_init(InstructUnit *ins_unit)
{
    ins_unit->depth = 0;
    ins_unit->const_pool_index = 0;
    ins_unit->isNewLine_instruct = 0;
}

InstructUnit *inst_array_get_now(InstructArray *self)
{
    if (self->content_offset_now >= self->size) {
        /* is the end */
        return NULL;
    }
    return (InstructUnit *)((uintptr_t)inst_array_get_start(self) +
                           (uintptr_t)(self->content_offset_now));
}

InstructUnit *inst_array_get_next(InstructArray *self)
{
    self->content_offset_now += inst_unit_get_size();
    return inst_array_get_now(self);
}

#if PIKA_INSTRUCT_EXTENSION_ENABLE

static const char *__find_ins_str_in_ins_set(enum InstructIndex op_idx, const VMInstructionSet* set)
{
    const VMInstruction* ins = set->instructions;
    uint_fast16_t count = set->count;

    do {
        if (ins->op_idx == op_idx) {
            return ins->op_str;
        }
        ins++;
    } while (--count);
    return NULL;
}

static char *inst_unit_get_inst_str(InstructUnit *self)
{
    enum InstructIndex op_idx = inst_unit_get_inst_index(self);
    const char *ins_str =
        __find_ins_str_in_ins_set(op_idx, g_PikaVMInsSet.recent->ins_set);
    if (NULL != ins_str) {
        return (char*)ins_str;
    }
    VMInstructionSetItem* item = g_PikaVMInsSet.list;
    do {
        ins_str = __find_ins_str_in_ins_set(op_idx, item->ins_set);
        if (NULL != ins_str) {
            g_PikaVMInsSet.recent = item;
            return (char*)ins_str;
        }
        item = item->next;
    } while (NULL != item->next);
    return "NON";
}
#else
static char *inst_unit_get_inst_str(InstructUnit *self) {
#define __INS_GET_INS_STR
#include "__instruction_table.h"
    return "NON";
}
#endif

void inst_unit_print(InstructUnit *self)
{
    if (inst_unit_get_is_new_line(self)) {
        pika_platform_printf("B%d\r\n", inst_unit_get_blk_deepth(self));
    }
    pika_platform_printf("%d %s #%d\r\n", inst_unit_get_invoke_deepth(self),
                         inst_unit_get_inst_str(self),
                         self->const_pool_index);
}

void inst_unit_print_with_const_pool(InstructUnit *self, ConstPool *const_pool)
{
    // if (inst_unit_get_is_new_line(self)) {
    //     pika_platform_printf("B%d\r\n",
    //     inst_unit_get_blk_deepth(self));
    // }
    pika_platform_printf(
        "%s %s \t\t(#%d)\r\n", inst_unit_get_inst_str(self),
        const_pool_get_by_offset(const_pool, self->const_pool_index),
        self->const_pool_index);
}

void inst_array_print_with_const_pool(InstructArray *self, ConstPool *const_pool)
{
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    while (1) {
        InstructUnit *ins_unit = inst_array_get_now(self);
        if (NULL == ins_unit) {
            goto __exit;
        }
        inst_unit_print_with_const_pool(ins_unit, const_pool);
        inst_array_get_next(self);
    }
__exit:
    self->content_offset_now = offset_befor;
    return;
}

void inst_array_print(InstructArray *self)
{
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    while (1) {
        InstructUnit *ins_unit = inst_array_get_now(self);
        if (NULL == ins_unit) {
            goto __exit;
        }
        inst_unit_print(ins_unit);
        inst_array_get_next(self);
    }
__exit:
    self->content_offset_now = offset_befor;
    return;
}

void inst_array_print_as_array(InstructArray *self)
{
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    uint8_t line_num = 12;
    uint16_t g_i = 0;
    uint8_t* ins_size_p = (uint8_t*)&self->size;
    pika_platform_printf("0x%02x, ", *(ins_size_p));
    pika_platform_printf("0x%02x, ", *(ins_size_p + (uintptr_t)1));
    pika_platform_printf("0x%02x, ", *(ins_size_p + (uintptr_t)2));
    pika_platform_printf("0x%02x, ", *(ins_size_p + (uintptr_t)3));
    pika_platform_printf("/* instruct array size */\n");
    while (1) {
        InstructUnit *ins_unit = inst_array_get_now(self);
        if (NULL == ins_unit) {
            goto __exit;
        }
        for (int i = 0; i < (int)inst_unit_get_size(); i++) {
            g_i++;
            pika_platform_printf("0x%02x, ", *((uint8_t*)ins_unit + (uintptr_t)i));
            if (g_i % line_num == 0) {
                pika_platform_printf("\n");
            }
        }
        inst_array_get_next(self);
    }
__exit:
    pika_platform_printf("/* instruct array */\n");
    self->content_offset_now = offset_befor;
    return;
}
