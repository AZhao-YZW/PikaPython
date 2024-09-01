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

extern char* Cursor_splitCollect(Args* buffs, char* stmt, char* devide, int index);
extern char* Parser_popSubStmt(Args* outbuffs, char** stmt_p, char* sDelimiter);

extern char* Cursor_popToken(Args* buffs, char** pStmt, char* devide);
extern bool Cursor_isContain(char* stmt, TokenType type, char* pyload);
extern void Cursor_iterStart(struct Cursor* cs);
extern void Cursor_iterEnd(struct Cursor* cs);
extern void Cursor_deinit(struct Cursor* cs);

extern char* _Parser_popLastSubStmt(Args* outbuffs, char** sStmt_p, char* sDelimiter, pika_bool bSkipBracket);
extern uint8_t Parser_checkIsDirect(char* sStr);
extern char* Parser_popLastSubStmt(Args* outbuffs, char** sStmt_p, char* sDelimiter);
extern int Parser_getSubStmtNum(char* sSubStmts, char* sDelimiter);

extern uint8_t Suger_selfOperator(Args* outbuffs, char* sStmt, char** sRight_p, char** sLeft_p);
extern char* Suger_leftSlice(Args* outBuffs, char* sRight, char** sLeft_p);
extern char* Suger_format(Args* outBuffs, char* sRight);
extern char* Suger_not_in(Args* out_buffs, char* sLine);
char* Suger_is_not(Args* out_buffs, char* sLine);

extern StmtType Lexer_matchStmtType(char* right);
extern char* Lexer_getOperator(Args* outBuffs, char* sStmt);
extern char* _remove_sub_stmt(Args* outBuffs, char* sStmt);

AST* ast_parse_stmt(AST* ast, char* stmt);
PIKA_RES ast_parse_substmt(AST* ast, char* node_content);

AST* ast_create(void)
{
    return New_queueObj();
}

int32_t ast_deinit(AST* ast)
{
    return obj_deinit(ast);
}

PIKA_RES ast_set_node_attr(AST* ast, char* attr_name, char* attr_val)
{
    return obj_setStr(ast, attr_name, attr_val);
}

PIKA_RES ast_set_node_blk(AST *ast, char* node_content) {
    return ast_set_node_attr(ast, "block", node_content);
}

/**********************************************************
 *                  ast_parse_stmt
 **********************************************************/
static void ast_parse_list(AST* ast, Args* buffs, char* stmt)
{
#if !PIKA_BUILTIN_STRUCT_ENABLE
    return;
#endif
    ast_set_node_attr(ast, (char*)"list", "list");
    char* substmts = strsCut(buffs, stmt, '[', ']');
    substmts = strsAppend(buffs, substmts, ",");
    while (1) {
        char* substmt = Parser_popSubStmt(buffs, &substmts, ",");
        ast_parse_substmt(ast, substmt);
        if (strEqu(substmts, "")) {
            break;
        }
    }
    return;
}

static void ast_parse_comprehension(AST* ast, Args* out_buffs, char* stmt)
{
#if PIKA_NANO_ENABLE
    return;
#endif
    /* [ substmt1 for substmt2 in substmt3 ] */
    Args buffs = {0};
    ast_set_node_attr(ast, (char*)"comprehension", "");
    char* substmts = strsCut(&buffs, stmt, '[', ']');
    char* substmts1 = Cursor_splitCollect(&buffs, substmts, " for ", 0);
    char* substmts23 = Cursor_splitCollect(&buffs, substmts, " for ", 1);
    char* substmts2 = Cursor_splitCollect(&buffs, substmts23, " in ", 0);
    char* substmts3 = Cursor_splitCollect(&buffs, substmts23, " in ", 1);
    ast_set_node_attr(ast, (char*)"substmt1", substmts1);
    ast_set_node_attr(ast, (char*)"substmt2", substmts2);
    ast_set_node_attr(ast, (char*)"substmt3", substmts3);
    strsDeinit(&buffs);
    return;
}

static void ast_parse_list_comprehension(AST *ast, Args *buffs, char *stmt)
{
    if (Cursor_isContain(stmt, TOKEN_keyword, " for ")) {
        ast_parse_comprehension(ast, buffs, stmt);
        return;
    }
    ast_parse_list(ast, buffs, stmt);
}

static void ast_parse_dict(AST *ast, Args *buffs, char *stmt)
{
#if !PIKA_BUILTIN_STRUCT_ENABLE
    return;
#endif
    ast_set_node_attr(ast, (char*)"dict", "dict");
    char* subStmts = strsCut(buffs, stmt, '{', '}');
    subStmts = strsAppend(buffs, subStmts, ",");
    while (1) {
        char* substmt = Parser_popSubStmt(buffs, &subStmts, ",");
        char* sKey = Parser_popSubStmt(buffs, &substmt, ":");
        char* sValue = substmt;
        ast_parse_substmt(ast, sKey);
        ast_parse_substmt(ast, sValue);
        if (strEqu(subStmts, "")) {
            break;
        }
    }
}

static void ast_parse_slice(AST *ast, Args *buffs, char *stmt)
{
#if !PIKA_SYNTAX_SLICE_ENABLE
    return;
#endif
    ast_set_node_attr(ast, (char*)"slice", "slice");
    stmt = strsCopy(buffs, stmt);
    char* last_stmt = _Parser_popLastSubStmt(buffs, &stmt, "[", false);
    ast_parse_substmt(ast, stmt);
    char* slice_list = strsCut(buffs, last_stmt, '[', ']');
    pika_assert(slice_list != NULL);
    slice_list = strsAppend(buffs, slice_list, ":");
    int index = 0;
    while (1) {
        char* slice = Parser_popSubStmt(buffs, &slice_list, ":");
        if (index == 0 && strEqu(slice, "")) {
            ast_parse_substmt(ast, "0");
        } else if (index == 1 && strEqu(slice, "")) {
            ast_parse_substmt(ast, "-99999");
        } else {
            ast_parse_substmt(ast, slice);
        }
        index++;
        if (strEqu("", slice_list)) {
            break;
        }
    }
}

PIKA_RES ast_parse_substmt(AST* ast, char* node_content)
{
    queueObj_pushObj(ast, (char*)"stmt");
    ast_parse_stmt(queueObj_getCurrentObj(ast), node_content);
    return PIKA_RES_OK;
}

int solve_operator_stmt(AST *ast, Args *buffs, char *right)
{
    right = Suger_not_in(buffs, right);
    right = Suger_is_not(buffs, right);
    char* right_without_substmt = _remove_sub_stmt(buffs, right);
    char* operator= Lexer_getOperator(buffs, right_without_substmt);
    if (NULL == operator) {
        return PIKA_RES_ERR_SYNTAX_ERROR;
    }
    ast_set_node_attr(ast, (char*)"operator", operator);
    char* right_buff = strsCopy(buffs, right);
    char* substmt2 = Cursor_popLastToken(buffs, &right_buff, operator);
    char* substmt1 = right_buff;
    ast_parse_substmt(ast, substmt1);
    ast_parse_substmt(ast, substmt2);
    return PIKA_RES_OK;
}

int solve_list_stmt(AST *ast, Args *buffs, char *right)
{
    ast_parse_list_comprehension(ast, buffs, right);
    return PIKA_RES_OK;
}

int solve_dict_stmt(AST *ast, Args *buffs, char *right)
{
    ast_parse_dict(ast, buffs, right);
    return PIKA_RES_OK;
}

int solve_chain_stmt(AST *ast, Args *buffs, char *right)
{
    char* host = strsCopy(buffs, right);
    char* method_stmt = Parser_popLastSubStmt(buffs, &host, ".");
    ast_parse_substmt(ast, host);
    ast_parse_stmt(ast, method_stmt);
    return PIKA_RES_OK;
}

int solve_slice_stmt(AST *ast, Args *buffs, char *right)
{
    ast_parse_slice(ast, buffs, right);
    return PIKA_RES_OK;
}

int solve_method_stmt(AST *ast, Args *buffs, char *right)
{
    char* method = NULL;
    char* real_type = "method";
    char* method_stmt = strsCopy(buffs, right);
    char* last_stmt = method_stmt;
    /* for method()() */
    int bracket_num =
        _Cursor_count(method_stmt, TOKEN_devider, "(", true) +
        _Cursor_count(method_stmt, TOKEN_devider, "[", true);
    if (bracket_num > 1) {
        last_stmt =
            _Parser_popLastSubStmt(buffs, &method_stmt, "(", false);
        /* for (...) */
        if (_Cursor_count(last_stmt, TOKEN_devider, "(", false) == 1) {
            char* method_check = strsGetFirstToken(buffs, last_stmt, '(');
            if (strEqu(method_check, "")) {
                last_stmt = strsAppend(buffs, ".", last_stmt);
            }
        }
        ast_parse_substmt(ast, method_stmt);
    }
    method = strsGetFirstToken(buffs, last_stmt, '(');
    char* substmts = strsCut(buffs, last_stmt, '(', ')');
    if (NULL == substmts) {
        return PIKA_RES_ERR_SYNTAX_ERROR;
    }
    /* add ',' at the end */
    substmts = strsAppend(buffs, substmts, ",");
    int substmts_num = Parser_getSubStmtNum(substmts, ",");
    for (int i = 0; i < substmts_num; i++) {
        char* substmt = Parser_popSubStmt(buffs, &substmts, ",");
        ast_parse_substmt(ast, substmt);
        if (strOnly(substmts, ',')) {
            if (i < substmts_num - 2) {
                return PIKA_RES_ERR_SYNTAX_ERROR;
            }
            if (i == substmts_num - 2 && strEqu(method, "")) {
                real_type = "tuple";
            }
            break;
        }
        if (strEqu("", substmts)) {
            if (i != substmts_num - 1) {
                return PIKA_RES_ERR_SYNTAX_ERROR;
            }
            break;
        }
    }
    ast_set_node_attr(ast, (char*)real_type, method);
    return PIKA_RES_OK;
}

int solve_reference_stmt(AST *ast, Args *buffs, char *right)
{
    char* ref = right;
    /* filter for type hint */
    ref = Cursor_splitCollect(buffs, ref, ":", 0);
    if (!strEqu(ref, right)) {
        return PIKA_RES_OK;
    }
    ast_set_node_attr(ast, (char*)"ref", ref);
    return PIKA_RES_OK;
}

int solve_import_stmt(AST *ast, Args *buffs, char *right)
{
    char* import = strsGetLastToken(buffs, right, ' ');
    ast_set_node_attr(ast, (char*)"import", import);
    return PIKA_RES_OK;
}

/* solve @inh stmt (from <module> import *) */
int solve_inherit_stmt(AST *ast, Args *buffs, char *right)
{
    char* inhert = strsGetLastToken(buffs, right, ' ');
    ast_set_node_attr(ast, (char*)"inhert", inhert);
    return PIKA_RES_OK;
}

int solve_string_stmt(AST *ast, Args *buffs, char *right)
{
    char* str = strsCopy(buffs, right);
    /* remove the first char */
    char firstChar = str[0];
    str = str + 1;
    /* remove the last char */
    str[strGetSize(str) - 1] = '\0';
    /* replace */
    if (strIsContain(str, '\\')) {
        switch (firstChar) {
            case '\'':
                str = strsReplace(buffs, str, "\\\'", "\'");
                break;
            case '\"':
                str = strsReplace(buffs, str, "\\\"", "\"");
                break;
        }
    }
    ast_set_node_attr(ast, (char*)"string", str);
    return PIKA_RES_OK;
}

int solve_bytes_stmt(AST *ast, Args *buffs, char *right)
{
    char* str = strsCopy(buffs, right);
    /* remove the first char */
    char firstChar = str[0];
    str = str + 2;
    /* remove the last char */
    str[strGetSize(str) - 1] = '\0';
    /* replace */
    if (strIsContain(str, '\\')) {
        switch (firstChar) {
            case '\'':
                str = strsReplace(buffs, str, "\\\'", "\'");
                break;
            case '\"':
                str = strsReplace(buffs, str, "\\\"", "\"");
                break;
        }
    }
    ast_set_node_attr(ast, (char*)"bytes", str);
    return PIKA_RES_OK;
}

int solve_number_stmt(AST *ast, Args *buffs, char *right)
{
    ast_set_node_attr(ast, (char*)"num", right);
    return PIKA_RES_OK;
}

int ast_solve_stmt(StmtType stmt_type, AST *ast, Args *buffs, char *right)
{
    static struct {
        uint8_t stmt_type;
        int (*solve_stmt_func)(AST *ast, Args *buffs, char *right);
    } solve_stmt_map[] = {
        { STMT_OPERATOR,  solve_operator_stmt  },
        { STMT_LIST,      solve_list_stmt      },
        { STMT_DICT,      solve_dict_stmt      },
        { STMT_CHAIN,     solve_chain_stmt     },
        { STMT_SLICE,     solve_slice_stmt     },
        { STMT_METHOD,    solve_method_stmt    },
        { STMT_REFERENCE, solve_reference_stmt },
        { STMT_IMPORT,    solve_import_stmt    },
        { STMT_INHERIT,   solve_inherit_stmt   },
        { STMT_STRING,    solve_string_stmt    },
        { STMT_BYTES,     solve_bytes_stmt     },
        { STMT_NUMBER,    solve_number_stmt    },
    };

    for (uint8_t i = 0; i < sizeof(solve_stmt_map) / sizeof(solve_stmt_map[0]); i++) {
        if (stmt_type == solve_stmt_map[i].stmt_type) {
            return solve_stmt_map[i].solve_stmt_func(ast, buffs, right);
        }
    }
    return PIKA_RES_OK;
}

AST* ast_parse_stmt(AST* ast, char* stmt)
{
    Args buffs = {0};
    char* assignment = Cursor_splitCollect(&buffs, stmt, "(", 0);
    char* left = NULL;
    char* right = NULL;
    StmtType stmt_type = STMT_NONE;
    PIKA_RES result = PIKA_RES_OK;

    right = stmt;
    /* solve check direct */
    bool is_left_exist = false;
    if (Parser_checkIsDirect(assignment)) {
        is_left_exist = true;
        left = strsCopy(&buffs, "");
        right = strsCopy(&buffs, "");
        bool is_meet_equ = 0;
        Cursor_forEach(cs, stmt) {
            Cursor_iterStart(&cs);
            if (!is_meet_equ && strEqu(cs.token1.pyload, "=") &&
                cs.token1.type == TOKEN_operator) {
                is_meet_equ = 1;
                Cursor_iterEnd(&cs);
                continue;
            }
            if (!is_meet_equ) {
                left = strsAppend(&buffs, left, cs.token1.pyload);
            }
            if (is_meet_equ) {
                right = strsAppend(&buffs, right, cs.token1.pyload);
            }
            Cursor_iterEnd(&cs);
        }
        Cursor_deinit(&cs);
    }
    /* solve the += -= /= *= stmt */
    if (!is_left_exist) {
        is_left_exist = Suger_selfOperator(&buffs, stmt, &right, &left);
    }

    /* remove hint */
    if (is_left_exist) {
        left = Cursor_splitCollect(&buffs, left, ":", 0);
    }

    /* solve the [] stmt */
    right = Suger_leftSlice(&buffs, right, &left);
    right = Suger_format(&buffs, right);

    /* set left */
    if (is_left_exist) {
        if (strEqu(left, "")) {
            result = PIKA_RES_ERR_SYNTAX_ERROR;
            goto __exit;
        }
        ast_set_node_attr(ast, (char*)"left", left);
    }
    /* match statment type */
    stmt_type = Lexer_matchStmtType(right);
    if (STMT_TUPLE == stmt_type) {
        right = strsFormat(&buffs, PIKA_LINE_BUFF_SIZE, "(%s)", right);
        stmt_type = STMT_METHOD;
    }
    result = ast_solve_stmt(stmt_type, ast, &buffs, right);

__exit:
    strsDeinit(&buffs);
    if (result != PIKA_RES_OK) {
        ast_deinit(ast);
        return NULL;
    }
    return ast;
}

/**********************************************************
 *                  parser_line_to_ast
 **********************************************************/
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

static uint8_t expr_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    *stmt = strsCut(buffs, *stmt, ' ', ':');
    ast_set_node_blk(ast, kw);
    stack_pushStr(blk_state->stack, kw);
    return BLK_MATCHED;
}

static uint8_t no_expr_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    uint8_t kw_len = strlen(kw);
    if ((*stmt)[kw_len] == ':' || strsCut(buffs, *stmt, ' ', ':') != NULL) {
        ast_set_node_blk(ast, kw);
        stack_pushStr(blk_state->stack, kw);
        *stmt = "";
    }
    return BLK_MATCHED;
}

static uint8_t control_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    ast_set_node_attr(ast, kw, "");
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
    ast_set_node_attr(ast, "arg_in", arg_in);
    strsPopToken(list_buffs, &line_buff, ' ');
    char* list_in = Cursor_splitCollect(list_buffs, line_buff, ":", 0);
    list_in = strsAppend(list_buffs, "iter(", list_in);
    list_in = strsAppend(list_buffs, list_in, ")");
    list_in = strsCopy(buffs, list_in);
    args_deinit(list_buffs);
    ast_set_node_blk(ast, "for");
    ast_set_node_attr(ast, "list_in", list_in);
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
    ast_set_node_attr(ast, "return", "");
    return BLK_MATCHED;
}

uint8_t raise_kw_to_ast(char *kw, char **stmt, AST *ast, BlockState *blk_state, Args *buffs)
{
    char* line_buff = strsCopy(buffs, *stmt);
    ast_set_node_attr(ast, "raise", "");
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
    ast_set_node_attr(ast, "assert", "");
    /* assert expr [, msg] */
    while (1) {
        char* sub_stmt = Parser_popSubStmt(buffs, &line_buff, ",");
        ast_parse_substmt(ast, sub_stmt);
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
    ast_set_node_attr(ast, "global", global_list);
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
    ast_set_node_attr(ast, "del", del_dir);
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
    ast_set_node_attr(ast, "raw", declare);
    if (!strIsContain(declare, '(') || !strIsContain(declare, ')')) {
        return BLK_EXIT;
    }
    char* default_stmt = get_def_default_stmt(buffs, &declare);
    ast_set_node_blk(ast, "def");
    ast_set_node_attr(ast, "declare", declare);
    if (default_stmt[0] != '\0') {
        ast_set_node_attr(ast, "default", default_stmt);
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
    ast_set_node_blk(ast, "class");
    ast_set_node_attr(ast, "declare", declare);
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
    AST *ast = ast_create();
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
        ast_set_node_attr(ast, "docstring", line + sizeof("@docstring"));
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
        ast_deinit(ast);
        ast = NULL;
        goto __exit;
    }
    stmt = Cursor_getCleanStmt(&buffs, stmt);
    ast = ast_parse_stmt(ast, stmt);
    goto __exit;
__exit:
    strsDeinit(&buffs);
    return ast;
}