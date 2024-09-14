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
#include "bc_frame.h"
#include "const_pool.h"
#include "inst.h"
#include "PikaVM.h"

static InstructUnit *bc_frame_find_inst_unit(ByteCodeFrame *bcframe, int32_t iPcStart,
    enum InstructIndex index, int32_t* iOffset_p, pika_bool bIsForward)
{
    /* find instruct unit */
    uint32_t instructArray_size = inst_array_get_size(&(bcframe->instruct_array));
    while (1) {
        *iOffset_p += (bIsForward ? -1 : 1) * inst_unit_get_size();
        if (iPcStart + *iOffset_p >= instructArray_size) {
            return NULL;
        }
        if (iPcStart + *iOffset_p < 0) {
            return NULL;
        }
        InstructUnit *unit = inst_array_get_by_offset(
            &(bcframe->instruct_array), iPcStart + *iOffset_p);
        if (index == inst_unit_get_inst_index(unit)) {
            return unit;
        }
    }
}

static InstructUnit *bc_frame_find_inst_forward(ByteCodeFrame *bcframe, int32_t pc_start,
    enum InstructIndex index, int32_t* p_offset)
{
    return bc_frame_find_inst_unit(bcframe, pc_start, index, p_offset,
                                          pika_true);
}

static InstructUnit *bc_frame_find_inst_unit_backward(ByteCodeFrame *bcframe, int32_t pc_start,
    enum InstructIndex index, int32_t* p_offset)
{
    return bc_frame_find_inst_unit(bcframe, pc_start, index, p_offset, pika_false);
}

Hash bc_frame_get_name_hash(ByteCodeFrame *bcframe)
{
    if (0 == bcframe->name_hash) {
        bcframe->name_hash = hash_time33(bcframe->name);
    }
    return bcframe->name_hash;
}

void bc_frame_init(ByteCodeFrame *self)
{
    /* init to support append, if only load static bytecode, can not init */
    const_pool_init(&(self->const_pool));
    inst_array_init(&(self->instruct_array));
    self->name = NULL;
    self->label_pc = -1;
}

void bc_frame_deinit(ByteCodeFrame *self)
{
    const_pool_deinit(&(self->const_pool));
    inst_array_deinit(&(self->instruct_array));
    if (NULL != self->name) {
        pika_platform_free(self->name);
    }
}

void bc_frame_set_name(ByteCodeFrame *self, char *name)
{
    if (name != NULL && self->name == NULL) {
        self->name = pika_platform_malloc(strGetSize(name) + 1);
        pika_platform_memcpy(self->name, name, strGetSize(name) + 1);
    }
}

extern const char magic_code_pyo[4];
void bc_frame_load_bytecode(ByteCodeFrame *self, uint8_t* bytes, char *name, pika_bool is_const)
{
    if (bytes[0] == magic_code_pyo[0] && bytes[1] == magic_code_pyo[1] &&
        bytes[2] == magic_code_pyo[2] && bytes[3] == magic_code_pyo[3]) {
        /* load from file, found magic code, skip head */
        bytes = bytes + sizeof(magic_code_pyo) + sizeof(uint32_t);
    }
    uint32_t* ins_size_p = (uint32_t*)bytes;
    void* ins_start_p = (uint32_t*)((uintptr_t)bytes + sizeof(*ins_size_p));
    uint32_t* const_size_p =
        (uint32_t*)((uintptr_t)ins_start_p + (uintptr_t)(*ins_size_p));
    self->instruct_array.size = *ins_size_p;
    self->instruct_array.content_start = ins_start_p;
    self->const_pool.size = *const_size_p;
    self->const_pool.content_start =
        (char*)((uintptr_t)const_size_p + sizeof(*const_size_p));
    bc_frame_set_name(self, name);
    if (!is_const) {
        pika_assert(NULL == self->instruct_array.arg_buff);
        pika_assert(NULL == self->instruct_array.arg_buff);
        self->instruct_array.arg_buff = arg_newBytes(ins_start_p, *ins_size_p);
        self->const_pool.arg_buff =
            arg_newBytes(self->const_pool.content_start, *const_size_p);
        self->instruct_array.content_start =
            arg_getBytes(self->instruct_array.arg_buff);
        self->const_pool.content_start =
            arg_getBytes(self->const_pool.arg_buff);
    }
    pika_assert(NULL != self->const_pool.content_start);
}

void bc_frame_load_bytecode_default(ByteCodeFrame *self, uint8_t* bytes)
{
    bc_frame_load_bytecode(self, bytes, NULL, pika_true);
}

size_t bc_frame_get_size(ByteCodeFrame *bf)
{
    return bf->const_pool.size + bf->instruct_array.size;
}

void bc_frame_print(ByteCodeFrame *self)
{
    const_pool_print(&(self->const_pool));
    inst_array_print_with_const_pool(&(self->instruct_array), &(self->const_pool));
    pika_platform_printf("---------------\r\n");
    pika_platform_printf("byte code size: %d\r\n",
                         self->const_pool.size + self->instruct_array.size);
}

void bc_frame_print_as_array(ByteCodeFrame *self)
{
    pika_platform_printf("const uint8_t bytes[] = {\n");
    inst_array_print_as_array(&(self->instruct_array));
    const_pool_get_print_as_array(&(self->const_pool));
    pika_platform_printf("};\n");
    pika_platform_printf("pikaVM_runByteCode(self, (uint8_t*)bytes);\n");
}

static char *_get_data_from_bytecode2(uint8_t* bytecode,
                                      enum InstructIndex ins1,
                                      enum InstructIndex ins2) {
    ByteCodeFrame bf = {0};
    char *res = NULL;
    bc_frame_init(&bf);
    bc_frame_load_bytecode_default(&bf, bytecode);
    while (1) {
        InstructUnit *ins_unit = inst_array_get_now(&bf.instruct_array);
        if (NULL == ins_unit) {
            goto __exit;
        }
        enum InstructIndex ins = inst_unit_get_inst_index(ins_unit);
        if (ins == ins1 || ins == ins2) {
            res = const_pool_get_by_offset(&bf.const_pool,
                                        ins_unit->const_pool_index);
            goto __exit;
        }
        inst_array_get_next(&bf.instruct_array);
    }
__exit:
    bc_frame_deinit(&bf);
    return res;
}

static ByteCodeFrame *_cache_bytecodeframe(PikaObj *self) {
    ByteCodeFrame bytecode_frame_stack = {0};
    ByteCodeFrame *res = NULL;
    if (!obj_isArgExist(self, "@bcn")) {
        /* start form @bc0 */
        obj_setInt(self, "@bcn", 0);
    }
    int bcn = obj_getInt(self, "@bcn");
    char bcn_str[] = "@bcx";
    bcn_str[3] = '0' + bcn;
    /* load bytecode to heap */
    args_setHeapStruct(self->list, bcn_str, bytecode_frame_stack,
                       bc_frame_deinit);
    /* get bytecode_ptr from heap */
    res = args_getHeapStruct(self->list, bcn_str);
    obj_setInt(self, "@bcn", bcn + 1);
    return res;
}

ByteCodeFrame *_cache_bcf_fn_bc(PikaObj *self, uint8_t* bytecode)
{
    /* save 'def' and 'class' to heap */
    if (NULL ==
        _get_data_from_bytecode2(bytecode, PIKA_INS(DEF), PIKA_INS(CLS))) {
        return NULL;
    }
    return _cache_bytecodeframe(self);
}

ByteCodeFrame *_cache_bcf_fn(PikaObj *self, char *py_lines)
{
    /* cache 'def' and 'class' to heap */
    if ((NULL == strstr(py_lines, "def ")) &&
        (NULL == strstr(py_lines, "class "))) {
        return NULL;
    }
    return _cache_bytecodeframe(self);
}

char *_find_super_class_name(ByteCodeFrame *bcframe, int32_t pc_start) {
    /* find super class */
    int32_t offset = 0;
    char *super_class_name = NULL;
    bc_frame_find_inst_forward(bcframe, pc_start, PIKA_INS(CLS), &offset);
    InstructUnit *unit_run = bc_frame_find_inst_unit_backward(
        bcframe, pc_start, PIKA_INS(RUN), &offset);
    super_class_name = const_pool_get_by_offset(
        &(bcframe->const_pool), inst_unit_get_const_pool_index(unit_run));
    return super_class_name;
}

ByteCodeFrame *bc_frame_append_from_asm(ByteCodeFrame *self, char *sPikaAsm)
{
    Asmer asmer = {
        .asm_code = sPikaAsm,
        .block_deepth_now = 0,
        .is_new_line = 0,
        .line_pointer = sPikaAsm,
    };
    uint16_t uConstPoolOffset;
    uint16_t uExistOffset;
    for (int i = 0; i < strCountSign(sPikaAsm, '\n'); i++) {
        Args buffs = {0};
        char *sLine = strsGetLine(&buffs, asmer.line_pointer);
        char *sData = NULL;
        char sIns[4] = "";
        char sInvokeDeepth[3] = "";
        uint8_t uSpaceNum = 0;
        uint8_t iInvokeDeepth = 0;
        uint8_t iInsStr = 0;
        Arg* aLineBuff = arg_newStr(sLine);
        strsDeinit(&buffs);
        sLine = arg_getStr(aLineBuff);
        InstructUnit ins_unit = {0};
        /* remove '\r' */
        if (sLine[strGetSize(sLine) - 1] == '\r') {
            sLine[strGetSize(sLine) - 1] = 0;
        }
        /* process block depth flag*/
        if ('B' == sLine[0]) {
            asmer.block_deepth_now = fast_atoi(sLine + 1);
            asmer.is_new_line = 1;
            goto __next_line;
        }

        /* process each ins */

        /* get constPool offset */
        uConstPoolOffset = 0;

        for (int i = 0; i < (int)strGetSize(sLine); i++) {
            if (uSpaceNum < 2) {
                if (sLine[i] == ' ') {
                    uSpaceNum++;
                    if (uSpaceNum == 2) {
                        sData = sLine + i + 1;
                        break;
                    }
                    continue;
                }
            }
            if (uSpaceNum == 0) {
                sInvokeDeepth[iInvokeDeepth++] = sLine[i];
                continue;
            }
            if (uSpaceNum == 1) {
                sIns[iInsStr++] = sLine[i];
                continue;
            }
        }

        uExistOffset = const_pool_get_offset_by_data(&(self->const_pool), sData);

        /* get const offset */
        if (strEqu(sData, "")) {
            /* not need const value */
            uConstPoolOffset = 0;
        } else if (65535 == uExistOffset) {
            /* push new const value */
            uConstPoolOffset = const_pool_get_last_offset(&(self->const_pool));
            /* load const to const pool buff */
            const_pool_append(&(self->const_pool), sData);
        } else {
            /* use exist const value */
            uConstPoolOffset = uExistOffset;
        }

        iInvokeDeepth = fast_atoi(sInvokeDeepth);
        /* load Asm to byte code unit */
        inst_unit_set_blk_deepth(&ins_unit, asmer.block_deepth_now);
        inst_unit_set_invoke_deepth(&ins_unit, iInvokeDeepth);
        inst_unit_set_const_pool_index(&ins_unit, uConstPoolOffset);
        inst_unit_set_inst_index(&ins_unit, inst_get_index_from_asm(sIns));
        if (asmer.is_new_line) {
            inst_unit_set_is_new_line(&ins_unit, 1);
            asmer.is_new_line = 0;
        }

        /* append instructUnit to instructArray */
        inst_array_append(&(self->instruct_array), &ins_unit);

    __next_line:
        /* point to next line */
        asmer.line_pointer += strGetLineSize(asmer.line_pointer) + 1;
        arg_deinit(aLineBuff);
    }
    return self;
}
