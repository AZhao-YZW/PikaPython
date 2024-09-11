/*
 * MIT License
 *
 * Copyright (c) 2024 1milk11 原辰璟 1114534452@qq.com
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
#include "vm.h"
#include "inst.h"
#include "PikaVM.h"
#include "PikaParser.h"
#include "PikaCompiler.h"
#include "vm_frame.h"
#include "bc_frame.h"

extern volatile PikaObjState g_PikaObjState;

volatile VMState g_PikaVMState = {
    .signal_ctrl = VM_SIGNAL_CTRL_NONE,
    .vm_cnt = 0,
#if PIKA_EVENT_ENABLE
    .cq = {
        .head = 0,
        .tail = 0,
        .res = {0},
    },
    .sq = {
        .head = 0,
        .tail = 0,
        .res = {0},
    },
    .event_pickup_cnt = 0,
    .event_thread = NULL,
    .event_thread_exit = pika_false,
#endif
#if PIKA_DEBUG_BREAK_POINT_MAX > 0
    .break_module_hash = {0},
    .break_point_pc = {0},
    .break_point_cnt = 0,
#endif
};

VM_SIGNAL_CTRL vm_signal_get_ctrl(void)
{
    return g_PikaVMState.signal_ctrl;
}

void vm_signal_set_ctrl_clear(void) {
    g_PikaVMState.signal_ctrl = VM_SIGNAL_CTRL_NONE;
}

void pika_vm_exit(void) {
    g_PikaVMState.signal_ctrl = VM_SIGNAL_CTRL_EXIT;
}

void pika_vm_exit_await(void) {
    pika_vm_exit();
    while (1) {
        pika_platform_thread_yield();
        if (vm_event_get_vm_cnt() == 0) {
            return;
        }
    }
}

extern volatile PikaObj *__pikaMain;
static enum shellCTRL __obj_shellLineHandler_debug(PikaObj *self,
                                                   char *input_line,
                                                   struct ShellConfig* config) {
    /* continue */
    if (strEqu("c", input_line)) {
        return SHELL_CTRL_EXIT;
    }
    /* next */
    if (strEqu("n", input_line)) {
        return SHELL_CTRL_EXIT;
    }
    /* launch shell */
    if (strEqu("sh", input_line)) {
        /* exit pika shell */
        pikaScriptShell((PikaObj*)__pikaMain);
        return SHELL_CTRL_CONTINUE;
    }
    /* quit */
    if (strEqu("q", input_line)) {
        obj_setInt(self, "enable", 0);
        return SHELL_CTRL_EXIT;
    }
    /* print */
    if (strIsStartWith(input_line, "p ")) {
        char *path = input_line + 2;
        Arg* asm_buff = arg_newStr("print(");
        asm_buff = arg_strAppend(asm_buff, path);
        asm_buff = arg_strAppend(asm_buff, ")\n");
        pikaVM_run_ex_cfg cfg = {0};
        cfg.globals = config->globals;
        cfg.in_repl = pika_true;
        pikaVM_run_ex(config->locals, arg_getStr(asm_buff), &cfg);
        arg_deinit(asm_buff);
        return SHELL_CTRL_CONTINUE;
    }
    pikaVM_run_ex_cfg cfg = {0};
    cfg.globals = config->globals;
    cfg.in_repl = pika_true;
    pikaVM_run_ex(config->locals, input_line, &cfg);
    return SHELL_CTRL_CONTINUE;
}

void pika_debug_set_trace(PikaObj *self)
{
    if (!obj_getInt(self, "enable")) {
        return;
    }
    char *name = "stdin";
    pika_assert(NULL != self->vmFrame);
    if (NULL != self->vmFrame->bytecode_frame->name) {
        name = self->vmFrame->bytecode_frame->name;
    }
    pika_platform_printf("%s:%d\n", name, self->vmFrame->pc);
    struct ShellConfig cfg = {
        .prefix = "(Pdb-pika) ",
        .handler = __obj_shellLineHandler_debug,
        .fn_getchar = __platform_getchar,
        .locals = self->vmFrame->locals,
        .globals = self->vmFrame->globals,
    };
    _do_pikaScriptShell(self, &cfg);
    shConfig_deinit(&cfg);
}

static int pikaVM_runInstructUnit(PikaObj *self, PikaVMFrame *vm, InstructUnit *ins_unit)
{
    enum InstructIndex instruct = inst_unit_get_inst_index(ins_unit);
    arg_newReg(ret_reg, PIKA_ARG_BUFF_SIZE);
    Arg* return_arg = &ret_reg;
    // char invode_deepth1_str[2] = {0};
    int32_t pc_next = vm->pc + inst_unit_get_size();
    char *data = vm_frame_get_const_with_inst_unit(vm, ins_unit);
    /* run instruct */
    pika_assert(NULL != vm->vm_thread);
    if (vm_frame_check_break_point(vm)) {
        pika_debug_set_trace(self);
    }

#if PIKA_INSTRUCT_EXTENSION_ENABLE
    const VMInstruction* ins = inst_unit_get_inst(instruct);
    if (NULL == ins) {
        /* todo: unsupported instruction */
        pika_assert(NULL != ins);
    }
    pika_assert(NULL != ins->handler);

    return_arg = ins->handler(self, vm, data, &ret_reg);
#else
    return_arg = VM_instruct_handler_table[instruct](self, vm, data, &ret_reg);
#endif

    if (vm->vm_thread->error_code != PIKA_RES_OK ||
        vm_signal_get_ctrl() == VM_SIGNAL_CTRL_EXIT) {
        /* raise jmp */
        if (vm->vm_thread->try_state == TRY_STATE_INNER) {
            vm->jmp = VM_JMP_RAISE;
        } else {
            /* exit */
            vm->jmp = VM_JMP_EXIT;
        }
    }

#if PIKA_BUILTIN_STRUCT_ENABLE
    int invoke_deepth = vm_frame_get_invoke_deepth_now(vm);
    if (invoke_deepth > 0) {
        PikaObj *oReg = vm->oreg[invoke_deepth - 1];
        if (NULL != oReg && NULL != return_arg) {
            pikaList_append(oReg, return_arg);
            return_arg = NULL;
        }
    }
#endif

    if (NULL != return_arg) {
        stack_pushArg(&(vm->stack), return_arg);
    }
    goto __next_line;
__next_line:
    /* exit */
    if (VM_JMP_EXIT == vm->jmp) {
        pc_next = VM_PC_EXIT;
        goto __exit;
    }
    /* break */
    if (VM_JMP_BREAK == vm->jmp) {
        pc_next = vm->pc + vm_frame_get_break_addr_offset(vm);
        goto __exit;
    }
    /* continue */
    if (VM_JMP_CONTINUE == vm->jmp) {
        pc_next = vm->pc + vm_frame_get_continue_addr_offset(vm);
        goto __exit;
    }
/* raise */
#if !PIKA_NANO_ENABLE
    if (VM_JMP_RAISE == vm->jmp) {
        int offset = vm_frame_get_raise_addr_offset(vm);
        if (0 == offset) {
            /* can not found end of try, return */
            pc_next = VM_PC_EXIT;
            vm->vm_thread->try_result = TRY_RESULT_RAISE;
            goto __exit;
        }
        pc_next = vm->pc + offset;
        vm->vm_thread->try_result = TRY_RESULT_NONE;
        goto __exit;
    }
#endif
    /* static jmp */
    if (vm->jmp != 0) {
        pc_next = vm->pc + vm_frame_get_jmp_addr_offset(vm);
        goto __exit;
    }
    /* not jmp */
    pc_next = vm->pc + inst_unit_get_size();

    /* jump to next line */
    if (vm->vm_thread->error_code != 0) {
        while (1) {
            if (pc_next >= (int)vm_frame_get_inst_array_size(vm)) {
                pc_next = VM_PC_EXIT;
                goto __exit;
            }
            InstructUnit *ins_next = inst_array_get_by_offset(
                &vm->bytecode_frame->instruct_array, pc_next);
            if (inst_unit_get_is_new_line(ins_next)) {
                goto __exit;
            }
            pc_next = pc_next + inst_unit_get_size();
        }
    }

    goto __exit;
__exit:
    vm->jmp = 0;
    /* reach the end */
    if (pc_next >= (int)vm_frame_get_inst_array_size(vm)) {
        return VM_PC_EXIT;
    }
    return pc_next;
}

VMParameters* pikaVM_runAsm(PikaObj *self, char *pikaAsm) {
    ByteCodeFrame bytecode_frame;
    bc_frame_init(&bytecode_frame);
    bc_frame_append_from_asm(&bytecode_frame, pikaAsm);
    VMParameters* res = pikaVM_runByteCodeFrame(self, &bytecode_frame);
    bc_frame_deinit(&bytecode_frame);
    return res;
}

VMParameters* pikaVM_run_ex(PikaObj *self,
                            char *py_lines,
                            pikaVM_run_ex_cfg* cfg) {
    ByteCodeFrame bytecode_frame_stack = {0};
    ByteCodeFrame *bytecode_frame_p = NULL;
    uint8_t is_use_heap_bytecode = 0;
    PikaObj *globals = self;
    if (NULL != cfg->globals) {
        globals = cfg->globals;
    }
    /*
     * the first obj_run, cache bytecode to heap, to support 'def' and
     * 'class'
     */
    bytecode_frame_p = _cache_bcf_fn(self, py_lines);
    if (NULL == bytecode_frame_p) {
        is_use_heap_bytecode = 0;
        /* get bytecode_ptr from stack */
        bytecode_frame_p = &bytecode_frame_stack;
    }

    /* load or generate byte code frame */
    /* generate byte code */
    bc_frame_init(bytecode_frame_p);
    if (PIKA_RES_OK != pika_lines2Bytes(bytecode_frame_p, py_lines)) {
        obj_setSysOut(self, PIKA_ERR_STRING_SYNTAX_ERROR);
        globals = NULL;
        goto __exit;
    }
    /* run byteCode */
    if (NULL != cfg->module_name) {
        bc_frame_set_name(bytecode_frame_p, cfg->module_name);
    }
    globals = _pikaVM_runByteCodeFrameGlobals(self, globals, bytecode_frame_p,
                                              cfg->in_repl);
    goto __exit;
__exit:
    if (!is_use_heap_bytecode) {
        bc_frame_deinit(&bytecode_frame_stack);
    }
    return globals;
}

VMParameters* pikaVM_runByteCodeInconstant(PikaObj *self, uint8_t* bytecode) {
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    pikaVM_runBytecode_ex_cfg cfg = {0};
    cfg.locals = self;
    cfg.globals = self;
    cfg.vm_thread = &vm_thread;
    cfg.is_const_bytecode = pika_false;
    return pikaVM_runByteCode_ex(self, (uint8_t*)bytecode, &cfg);
}

VMParameters* pikaVM_runByteCodeFile(PikaObj *self, char *filename) {
    Args buffs = {0};
    Arg* file_arg = arg_loadFile(NULL, filename);
    pika_assert(NULL != file_arg);
    if (NULL == file_arg) {
        pika_platform_printf("Error: can not open file '%s'\n", filename);
        return NULL;
    }
    uint8_t* lines = arg_getBytes(file_arg);
    /* clear the void line */
    VMParameters* res = pikaVM_runByteCodeInconstant(self, lines);
    arg_deinit(file_arg);
    strsDeinit(&buffs);
    return res;
}

VMParameters* pikaVM_runSingleFile(PikaObj *self, char *filename) {
    Args buffs = {0};
    Arg* file_arg = arg_loadFile(NULL, filename);
    if (NULL == file_arg) {
        pika_platform_printf("FileNotFoundError: %s\n", filename);
        return NULL;
    }
    char *lines = (char*)arg_getBytes(file_arg);
    lines = strsFilePreProcess(&buffs, lines);
    /* clear the void line */
    pikaVM_run_ex_cfg cfg = {0};
    cfg.in_repl = pika_false;
    char *module_name = strsPathGetFileName(&buffs, filename);
    module_name = strsPopToken(&buffs, &module_name, '.');
    cfg.module_name = module_name;
    VMParameters* res = pikaVM_run_ex(self, lines, &cfg);
    arg_deinit(file_arg);
    strsDeinit(&buffs);
    return res;
}

VMParameters* pikaVM_run(PikaObj *self, char *py_lines) {
    pikaVM_run_ex_cfg cfg = {0};
    cfg.in_repl = pika_false;
    return pikaVM_run_ex(self, py_lines, &cfg);
}

VMParameters* pikaVM_runByteCode(PikaObj *self, const uint8_t* bytecode) {
    pika_assert(NULL != self);
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    pikaVM_runBytecode_ex_cfg cfg = {0};
    cfg.locals = self;
    cfg.globals = self;
    cfg.name = NULL;
    cfg.vm_thread = &vm_thread;
    cfg.is_const_bytecode = pika_true;
    return pikaVM_runByteCode_ex(self, (uint8_t*)bytecode, &cfg);
}

Arg* pikaVM_runByteCodeReturn(PikaObj *self,
                              const uint8_t* bytecode,
                              char *returnName) {
    pikaVM_runByteCode(self, bytecode);
    Arg* ret = args_getArg(self->list, returnName);
    if (NULL == ret) {
        return NULL;
    }
    ret = arg_copy(ret);
    /* set gc root to avoid be free */
    arg_setObjFlag(ret, OBJ_FLAG_GC_ROOT);
    obj_removeArg(self, returnName);
    return ret;
}

Arg* pikaVM_runByteCode_exReturn(PikaObj *self,
                                 VMParameters* locals,
                                 VMParameters* globals,
                                 uint8_t* bytecode,
                                 PikaVMThread* vm_thread,
                                 pika_bool is_const_bytecode,
                                 char *return_name) {
    pikaVM_runBytecode_ex_cfg cfg = {0};
    cfg.locals = locals;
    cfg.globals = globals;
    cfg.vm_thread = vm_thread;
    cfg.is_const_bytecode = is_const_bytecode;
    pikaVM_runByteCode_ex(self, bytecode, &cfg);
    Arg* ret = args_getArg(self->list, return_name);
    if (NULL == ret) {
        return NULL;
    }
    ret = arg_copy(ret);
    /* set gc root to avoid be free */
    arg_setObjFlag(ret, OBJ_FLAG_GC_ROOT);
    obj_removeArg(self, return_name);
    return ret;
}

void _pikaVM_yield(void) {
#if PIKA_EVENT_ENABLE
    if (!vm_gil_is_init()) {
        _VMEvent_pickupEvent();
    }
#endif
    /*
     * [Warning!] Can not use vm_gil_enter() and vm_gil_exit() here,
     * because yield() is called not atomically, and it will cause data
     * race.
     */
    // vm_gil_exit();
    // vm_gil_enter();
}

static VMParameters* __pikaVM_runByteCodeFrameWithState(
    PikaObj *self,
    VMParameters* locals,
    VMParameters* globals,
    ByteCodeFrame *bytecode_frame,
    uint16_t pc,
    PikaVMThread* vm_thread,
    pika_bool in_repl) {
    pika_assert(NULL != vm_thread);
    int size = bytecode_frame->instruct_array.size;
    /* locals is the local scope */

    if (g_PikaVMState.vm_cnt == 0) {
        vm_signal_set_ctrl_clear();
    }
    PikaVMFrame *vm =
        vm_frame_create(locals, globals, bytecode_frame, pc, vm_thread);
    vm->in_repl = in_repl;
    g_PikaVMState.vm_cnt++;
    while (vm->pc < size) {
        if (vm->pc == VM_PC_EXIT) {
            break;
        }
        InstructUnit *this_ins_unit = vm_frame_get_inst_now(vm);
        uint8_t is_new_line = inst_unit_get_is_new_line(this_ins_unit);
        if (is_new_line) {
            vm_frame_solve_unused_stack(vm);
            stack_reset(&(vm->stack));
            vm->vm_thread->error_code = 0;
            vm->vm_thread->line_error_code = 0;
        }
        self->vmFrame = vm;
        vm->pc = pikaVM_runInstructUnit(self, vm, this_ins_unit);
        vm->ins_cnt++;
#if PIKA_INSTRUCT_HOOK_ENABLE
        if (vm->ins_cnt % PIKA_INSTRUCT_HOOK_PERIOD == 0) {
            pika_hook_instruct();
        }
#endif
        if (vm->ins_cnt % PIKA_INSTRUCT_YIELD_PERIOD == 0) {
            _pikaVM_yield();
        }
        if (0 != vm->vm_thread->error_code) {
            vm->vm_thread->line_error_code = vm->vm_thread->error_code;
            InstructUnit *head_ins_unit = this_ins_unit;
            /* get first ins of a line */
            while (1) {
                if (inst_unit_get_is_new_line(head_ins_unit)) {
                    break;
                }
                head_ins_unit--;
            }
            if (vm->vm_thread->try_state) {
                vm->vm_thread->try_error_code = vm->vm_thread->error_code;
            }
            /* print inses of a line */
            if (!vm->vm_thread->try_state) {
                while (1) {
                    if (head_ins_unit != this_ins_unit) {
                        pika_platform_printf("   ");
                    } else {
                        pika_platform_printf(" -> ");
                    }
                    inst_unit_print_with_const_pool(head_ins_unit,
                                                &(bytecode_frame->const_pool));
                    head_ins_unit++;
                    if (head_ins_unit > this_ins_unit) {
                        break;
                    }
                }
            }
            pika_platform_error_handle();
        }
    }
    vm_frame_solve_unused_stack(vm);
    stack_deinit(&(vm->stack));
    g_PikaVMState.vm_cnt--;
    VMParameters* result = locals;
    pikaFree(vm, sizeof(PikaVMFrame));
    self->vmFrame = NULL;
    return result;
}

pika_bool pika_debug_check_break(char *module_name, int pc_break) {
    Hash h = hash_time33(module_name);
    for (int i = 0; i < g_PikaVMState.break_point_cnt; i++) {
        if (g_PikaVMState.break_module_hash[i] == h &&
            g_PikaVMState.break_point_pc[i] == pc_break) {
            return pika_true;
        }
    }
    return pika_false;
}

pika_bool pika_debug_check_break_hash(Hash module_hash, int pc_break) {
    for (int i = 0; i < g_PikaVMState.break_point_cnt; i++) {
        if (g_PikaVMState.break_module_hash[i] == module_hash &&
            g_PikaVMState.break_point_pc[i] == pc_break) {
            return pika_true;
        }
    }
    return pika_false;
}

PIKA_RES pika_debug_set_break(char *module_name, int pc_break) {
    if (pika_debug_check_break(module_name, pc_break)) {
        return PIKA_RES_OK;
    }
    if (g_PikaVMState.break_point_cnt >= PIKA_DEBUG_BREAK_POINT_MAX) {
        return PIKA_RES_ERR_RUNTIME_ERROR;
    }
    Hash h = hash_time33(module_name);
    g_PikaVMState.break_module_hash[g_PikaVMState.break_point_cnt] = h;
    g_PikaVMState.break_point_pc[g_PikaVMState.break_point_cnt] = pc_break;
    g_PikaVMState.break_point_cnt++;
    return PIKA_RES_OK;
}

PIKA_RES pika_debug_reset_break(char *module_name, int pc_break) {
    if (!pika_debug_check_break(module_name, pc_break)) {
        return PIKA_RES_OK;
    }
    Hash h = hash_time33(module_name);
    for (int i = 0; i < g_PikaVMState.break_point_cnt; i++) {
        if (g_PikaVMState.break_module_hash[i] == h &&
            g_PikaVMState.break_point_pc[i] == pc_break) {
            // Move subsequent break points one position forward
            for (int j = i; j < g_PikaVMState.break_point_cnt - 1; j++) {
                g_PikaVMState.break_module_hash[j] =
                    g_PikaVMState.break_module_hash[j + 1];
                g_PikaVMState.break_point_pc[j] =
                    g_PikaVMState.break_point_pc[j + 1];
            }
            // Decrease the count of break points
            g_PikaVMState.break_point_cnt--;
            return PIKA_RES_OK;
        }
    }
    return PIKA_RES_ERR_RUNTIME_ERROR;
}

VMParameters* _pikaVM_runByteCodeFrameWithState(PikaObj *self, VMParameters* locals,
    VMParameters* globals, ByteCodeFrame *bytecode_frame, uint16_t pc, PikaVMThread* vm_thread)
{
    return __pikaVM_runByteCodeFrameWithState(
        self, locals, globals, bytecode_frame, pc, vm_thread, pika_false);
}

VMParameters* _pikaVM_runByteCodeFrame(PikaObj *self,
                                       ByteCodeFrame *bc_frame,
                                       pika_bool in_repl) {
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    return __pikaVM_runByteCodeFrameWithState(self, self, self, bc_frame,
                                              0, &vm_thread, in_repl);
}

VMParameters* _pikaVM_runByteCodeFrameGlobals(PikaObj *self,
                                              PikaObj *globals,
                                              ByteCodeFrame *bc_frame,
                                              pika_bool in_repl) {
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    return __pikaVM_runByteCodeFrameWithState(
        self, self, globals, bc_frame, 0, &vm_thread, in_repl);
}

VMParameters* pikaVM_runByteCodeFrame(PikaObj *self,
                                      ByteCodeFrame *bc_frame) {
    return _pikaVM_runByteCodeFrame(self, bc_frame, pika_false);
}

PikaObj *pikaVM_runFile(PikaObj *self, char *file_name) {
    Args buffs = {0};
    char *module_name = strsPathGetFileName(&buffs, file_name);
    strPopLastToken(module_name, '.');
    char *pwd = strsPathGetFolder(&buffs, file_name);
    pika_platform_printf("(pikascript) pika compiler:\r\n");
    PikaMaker* maker = New_PikaMaker();
    pikaMaker_setPWD(maker, pwd);
    pikaMaker_compileModuleWithDepends(maker, module_name);
    _do_pikaMaker_linkCompiledModules(maker, "pikaModules_cache.py.a",
                                      pika_false);
    pikaMaker_deinit(maker);
    pika_platform_printf("(pikascript) all succeed.\r\n\r\n");

    pikaMemMaxReset();
    char *libfile_path =
        strsPathJoin(&buffs, pwd, "pikascript-api/pikaModules_cache.py.a");
    if (PIKA_RES_OK == obj_linkLibraryFile(self, libfile_path)) {
        obj_runModule(self, module_name);
    }
    strsDeinit(&buffs);
    return self;
}

VMParameters* pikaVM_runByteCode_ex(PikaObj *self,
                                    uint8_t* bytecode,
                                    pikaVM_runBytecode_ex_cfg* cfg) {
    ByteCodeFrame bytecode_frame_stack = {0};
    ByteCodeFrame *bytecode_frame_p = NULL;
    uint8_t is_use_heap_bytecode = 1;
    /*
     * the first obj_run, cache bytecode to heap, to support 'def' and
     * 'class'
     */
    bytecode_frame_p = _cache_bcf_fn_bc(self, bytecode);
    if (NULL == bytecode_frame_p) {
        is_use_heap_bytecode = 0;
        /* get bytecode_ptr from stack */
        bytecode_frame_p = &bytecode_frame_stack;
        /* no def/class ins, no need cache bytecode */
        cfg->is_const_bytecode = pika_true;
    }

    /* load or generate byte code frame */
    /* load bytecode */
    bc_frame_load_bytecode(bytecode_frame_p, bytecode, cfg->name,
                                   cfg->is_const_bytecode);

    /* run byteCode */

    cfg->globals = _pikaVM_runByteCodeFrameWithState(
        self, cfg->locals, cfg->globals, bytecode_frame_p, 0, cfg->vm_thread);
    goto __exit;
__exit:
    if (!is_use_heap_bytecode) {
        bc_frame_deinit(&bytecode_frame_stack);
    }
    return cfg->globals;
}