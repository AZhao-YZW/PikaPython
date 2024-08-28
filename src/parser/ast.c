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
#include "ast.h"
#include "PikaPlatform.h"
#include "parser.h"
#include "PikaParser.h"
#include "TinyObj.h"

extern int32_t Parser_getPyLineBlockDeepth(char* sLine);
extern char* Suger_multiReturn(Args* out_buffs, char* sLine);
extern PIKA_RES AST_setNodeAttr(AST *ast, char* sAttrName, char* sAttrVal);
PIKA_RES AST_setNodeBlock(AST *ast, char* sNodeContent) {
    return AST_setNodeAttr(ast, "block", sNodeContent);
}
extern char* Cursor_splitCollect(Args* buffs, char* stmt, char* devide, int index);
extern char* Parser_popSubStmt(Args* outbuffs, char** sStmt_p, char* sDelimiter);
extern PIKA_RES AST_parseSubStmt(AST *ast, char* sNodeContent);

extern char* Cursor_popToken(Args* buffs, char** pStmt, char* devide);
extern pika_bool Cursor_isContain(char* stmt, TokenType type, char* pyload);

static char* get_def_default_stmt(Args* out_buffs, char** declare_out) {
#if PIKA_NANO_ENABLE
    return "";
#else
    Args buffs = {0};
    char* declare_str = strsCopy(&buffs, *declare_out);
    char* fn_name = strsGetFirstToken(&buffs, declare_str, '(');
    Arg* declare_arg = arg_strAppend(arg_newStr(fn_name), "(");
    Arg* default_arg = arg_newStr("");
    char* arg_list = strsCut(&buffs, declare_str, '(', ')');
    char* default_out = NULL;
    pika_assert(NULL != arg_list);
    int iArgNum = _Cursor_count(arg_list, TOKEN_devider, ",", true) + 1;
    for (int i = 0; i < iArgNum; i++) {
        char* item = Cursor_popToken(&buffs, &arg_list, ",");
        char* default_val = NULL;
        char* default_key = NULL;
        bool is_default = false;
        if (Cursor_isContain(item, TOKEN_operator, "=")) {
            /* has default value */
            default_val = Cursor_splitCollect(&buffs, item, "=", 1);
            default_key = Cursor_splitCollect(&buffs, item, "=", 0);
            default_key = Cursor_splitCollect(&buffs, default_key, ":", 0);
            default_arg = arg_strAppend(default_arg, default_key);
            default_arg = arg_strAppend(default_arg, "=");
            default_arg = arg_strAppend(default_arg, default_val);
            default_arg = arg_strAppend(default_arg, ",");
            is_default = true;
        } else {
            default_key = item;
        }
        declare_arg = arg_strAppend(declare_arg, default_key);
        if (is_default) {
            declare_arg = arg_strAppend(declare_arg, "=");
        }
        declare_arg = arg_strAppend(declare_arg, ",");
    }
    strPopLastToken(arg_getStr(declare_arg), ',');
    declare_arg = arg_strAppend(declare_arg, ")");
    *declare_out = strsCopy(out_buffs, arg_getStr(declare_arg));
    default_out = strsCopy(out_buffs, arg_getStr(default_arg));
    strPopLastToken(default_out, ',');
    arg_deinit(declare_arg);
    arg_deinit(default_arg);
    strsDeinit(&buffs);
    return default_out;
#endif
}

/**
 * parser_line_to_ast
 */
static uint8_t expr_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    *stmt = strsCut(buffs, *stmt, ' ', ':');
    AST_setNodeBlock(ast, kw);
    stack_pushStr(blk_state->stack, kw);
    return BLK_MATCHED;
}

static uint8_t no_expr_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    uint8_t kw_len = strlen(kw);
    if ((*stmt)[kw_len] == ':' || strsCut(buffs, *stmt, ' ', ':') != NULL) {
        AST_setNodeBlock(ast, kw);
        stack_pushStr(blk_state->stack, kw);
        *stmt = "";
    }
    return BLK_MATCHED;
}

static uint8_t control_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    AST_setNodeAttr(ast, kw, "");
    if (strcmp(kw, "raise") == 0) {
        *stmt = "RuntimeError";
    } else {
        *stmt = "";
    }
    return BLK_MATCHED;
}

uint8_t for_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    Args* list_buffs = New_strBuff();
    char* line_buff = strsCopy(list_buffs, *stmt + 4);
    line_buff = Cursor_getCleanStmt(list_buffs, line_buff);
    if (strCountSign(line_buff, ':') < 1) {
        args_deinit(list_buffs);
        return BLK_EXIT;
    }
    char* arg_in = strsPopToken(list_buffs, &line_buff, ' ');
    AST_setNodeAttr(ast, "arg_in", arg_in);
    strsPopToken(list_buffs, &line_buff, ' ');
    char* list_in = Cursor_splitCollect(list_buffs, line_buff, ":", 0);
    list_in = strsAppend(list_buffs, "iter(", list_in);
    list_in = strsAppend(list_buffs, list_in, ")");
    list_in = strsCopy(buffs, list_in);
    args_deinit(list_buffs);
    AST_setNodeBlock(ast, "for");
    AST_setNodeAttr(ast, "list_in", list_in);
    stack_pushStr(blk_state->stack, "for");
    *stmt = list_in;
    return BLK_MATCHED;
}

uint8_t return_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char *line_buff = strsCopy(buffs, *stmt);
    strsPopToken(buffs, &line_buff, ' ');
    *stmt = line_buff;
    *stmt = Suger_multiReturn(buffs, *stmt);
    AST_setNodeAttr(ast, "return", "");
    return BLK_MATCHED;
}

uint8_t raise_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char* line_buff = strsCopy(buffs, *stmt);
    AST_setNodeAttr(ast, "raise", "");
    strsPopToken(buffs, &line_buff, ' ');
    *stmt = line_buff;
    if (strEqu("", *stmt)) {
        *stmt = "RuntimeError";
    }
    return BLK_MATCHED;
}

uint8_t assert_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char* line_buff = strsCopy(buffs, *stmt + 7);
    AST_setNodeAttr(ast, "assert", "");
    /* assert expr [, msg] */
    while (1) {
        char* sub_stmt = Parser_popSubStmt(buffs, &line_buff, ",");
        AST_parseSubStmt(ast, sub_stmt);
        if (strEqu(line_buff, "")) {
            break;
        }
    }
    *stmt = "";
    return BLK_MATCHED;
}

uint8_t global_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char* global_list = *stmt + 7;
    global_list = Cursor_getCleanStmt(buffs, global_list);
    AST_setNodeAttr(ast, "global", global_list);
    *stmt = "";
    return BLK_MATCHED;
}

uint8_t del_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char* del_dir = strsCut(buffs, *stmt, '(', ')');
    if (!del_dir) {
        del_dir = *stmt + sizeof("del ") - 1;
    }
    del_dir = Cursor_getCleanStmt(buffs, del_dir);
    AST_setNodeAttr(ast, "del", del_dir);
    *stmt = "";
    return BLK_MATCHED;
}

uint8_t def_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char* declare = strsCut(buffs, *stmt, ' ', ':');
    *stmt = "";
    if (NULL == declare) {
        return BLK_EXIT;
    }
    declare = Cursor_getCleanStmt(buffs, declare);
    AST_setNodeAttr(ast, "raw", declare);
    if (!strIsContain(declare, '(') || !strIsContain(declare, ')')) {
        return BLK_EXIT;
    }
    char* default_stmt = get_def_default_stmt(buffs, &declare);
    AST_setNodeBlock(ast, "def");
    AST_setNodeAttr(ast, "declare", declare);
    if (default_stmt[0] != '\0') {
        AST_setNodeAttr(ast, "default", default_stmt);
    }
    stack_pushStr(blk_state->stack, "def");
    return BLK_MATCHED;
}

uint8_t class_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char* declare = strsCut(buffs, *stmt, ' ', ':');
    *stmt = "";
    if (NULL == declare) {
        return BLK_EXIT;
    }
    declare = Cursor_getCleanStmt(buffs, declare);
    AST_setNodeBlock(ast, "class");
    AST_setNodeAttr(ast, "declare", declare);
    stack_pushStr(blk_state->stack, "class");
    return BLK_MATCHED;
}

uint8_t keyword_to_ast(char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    static struct {
        char *kw;
        uint8_t match_type;
        uint8_t (*kw_to_ast)(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs);
    } kw_map[] = {
        { "while ",    KW_START, expr_kw_to_ast      },
        { "if ",       KW_START, expr_kw_to_ast      },
        { "elif ",     KW_START, expr_kw_to_ast      },
        { "else",      KW_START, no_expr_kw_to_ast   },
        { "break",     KW_EQUAL, control_kw_to_ast   },
        { "break ",    KW_START, control_kw_to_ast   },
        { "continue",  KW_EQUAL, control_kw_to_ast   },
        { "continue ", KW_START, control_kw_to_ast   },
        { "return",    KW_EQUAL, control_kw_to_ast   },
        { "return ",   KW_START, return_kw_to_ast    },
        { "for ",      KW_START, for_kw_to_ast       },
        { "global ",   KW_START, global_kw_to_ast    },
        { "del ",      KW_START, del_kw_to_ast       },
        { "del(",      KW_START, del_kw_to_ast       },
        { "def ",      KW_START, def_kw_to_ast       },
        { "class ",    KW_START, class_kw_to_ast     },
    #if PIKA_SYNTAX_EXCEPTION_ENABLE
        { "try",       KW_START, no_expr_kw_to_ast   },
        { "except",    KW_START, no_expr_kw_to_ast   },
        { "raise",     KW_EQUAL, control_kw_to_ast   },
        { "raise ",    KW_START, raise_kw_to_ast     },
        { "assert ",   KW_START, assert_kw_to_ast    },
    #endif
    };
    char kw[15] = {0};
    uint8_t kw_len;

    for (uint8_t i = 0; i < sizeof(kw_map) / sizeof(kw_map[0]); i++) {
        if (kw_map[i].match_type == KW_START && !strIsStartWith(*stmt, kw_map[i].kw)) {
            continue;
        }
        if (kw_map[i].match_type == KW_EQUAL && !strEqu(*stmt, kw_map[i].kw)) {
            continue;
        }
        kw_len = strlen(kw_map[i].kw);
        strncpy(kw, kw_map[i].kw, sizeof(kw));
        if (kw[kw_len - 1] == ' ') {
            kw[kw_len - 1] = '\0';
        }
        return kw_map[i].kw_to_ast(kw, stmt, ast, blk_state, buffs);
    }
    return BLK_NOT_MATCHED;
}

AST* parser_line_to_ast(Parser* self, char* line)
{
    BlockState *blk_state = &self->blk_state;
    AST *ast = AST_create();
    int8_t blk_depth_now, blk_depth_last = -1;
    Args buffs = {0};
    char *stmt;
    uint8_t ret;

    /* line is not exist */
    if (line == NULL) {
        return NULL;
    }

    /* docsting */
    if (strIsStartWith(line, "@docstring")) {
        AST_setNodeAttr(ast, "docstring", line + sizeof("@docstring"));
        stmt = "";
        goto __block_matched;
    }

    /* get block depth */
    blk_depth_now = Parser_getPyLineBlockDeepth(line);
    if (self->blk_depth_origin == _VAL_NEED_INIT) {
        self->blk_depth_origin = blk_depth_now;
    }
    /* set block depth */
    if (blk_depth_now == -1) {
        /* get block_deepth error */
        pika_platform_printf(
            "IndentationError: unexpected indent, only support 4 "
            "spaces\r\n");
        obj_deinit(ast);
        ast = NULL;
        goto __exit;
    }
    blk_depth_now += blk_state->depth;
    obj_setInt(ast, "blockDeepth", blk_depth_now);

    /* check if exit block */
    if (0 != stack_getTop(blk_state->stack)) {
        blk_depth_last = stack_getTop(blk_state->stack) +
                           blk_state->depth + self->blk_depth_origin;
        /* exit each block */
        for (int i = 0; i < blk_depth_last - blk_depth_now; i++) {
            PikaObj* exit_block_queue = obj_getObj(ast, "exitBlock");
            /* create an exit_block queue */
            if (NULL == exit_block_queue) {
                obj_newObj(ast, "exitBlock", "", New_TinyObj);
                exit_block_queue = obj_getObj(ast, "exitBlock");
                queueObj_init(exit_block_queue);
            }
            char buff[10] = {0};
            char* blk_type = stack_popStr(blk_state->stack, buff);
            /* push exit block type to exit_block queue */
            queueObj_pushStr(exit_block_queue, blk_type);
        }
    }

    stmt = line + (blk_depth_now - blk_state->depth) * 4;

    ret = keyword_to_ast(&stmt, ast, blk_state, &buffs);
    if (ret == BLK_MATCHED) {
        goto __block_matched;
    } else if (ret == BLK_EXIT) {
        obj_deinit(ast);
        ast = NULL;
        goto __exit;
    }

__block_matched:
    if (NULL == stmt) {
        AST_deinit(ast);
        ast = NULL;
        goto __exit;
    }
    stmt = Cursor_getCleanStmt(&buffs, stmt);
    ast = AST_parseStmt(ast, stmt);
    goto __exit;
__exit:
    strsDeinit(&buffs);
    return ast;
}