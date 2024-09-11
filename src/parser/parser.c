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
#include "parser.h"
#include "PikaVM.h"
#include "pika_config_valid.h"
#include "dataString.h"
#include "ast.h"
#include "lexer.h"
#include "cursor.h"

char* parser_remove_comment(char* line)
{
    bool is_comment_exit = false;
    bool is_in_single_quotes = false;
    bool is_in_double_quotes = false;
    bool is_backslash = false;

    for (uint32_t i = 0; i < strGetSize(line); i++) {
        // ignore '\'
        if (is_backslash) {
            is_backslash = false;
            continue;
        }

        if ('\\' == line[i]) {
            is_backslash = true;
            continue;
        }

        // ignore the '#' in quotes
        if ('\'' == line[i] && !is_in_double_quotes) {
            is_in_single_quotes = !is_in_single_quotes;
            continue;
        }
        if ('"' == line[i] && !is_in_single_quotes) {
            is_in_double_quotes = !is_in_double_quotes;
            continue;
        }
        if (is_in_single_quotes || is_in_double_quotes) {
            continue;
        }

        // remove contents after '#'
        if ('#' == line[i]) {
            line[i] = 0;
            is_comment_exit = true;
            break;
        }
    }

    if (!is_comment_exit) {
        return line;
    }

    for (uint32_t i = 0; i < strGetSize(line); i++) {
        if (' ' != line[i]) {
            return line;
        }
    }

    line = "@annotation";
    return line;
}

extern char* parser_ast2Asm(Parser* self, AST* ast);

Parser* parser_create(void)
{
    Parser* self = (Parser*)pikaMalloc(sizeof(Parser));
    pika_platform_memset(self, 0, sizeof(Parser));
    self->blk_state.stack = pikaMalloc(sizeof(Stack));
    /* generate asm as default */
    self->ast_to_target = parser_ast2Asm;
    pika_platform_memset(self->blk_state.stack, 0, sizeof(Stack));
    stack_init(self->blk_state.stack);
    /* -1 means not inited */
    self->blk_depth_origin = _VAL_NEED_INIT;
    return self;
}

#define IS_SPACE_OR_TAB(ch) ((ch) == ' ' || (ch) == '\t')
static pika_bool _strCheckCodeBlockFlag(char* sLine) {
    pika_bool bStart = pika_false, bEnd = pika_false;
    char *sStart = sLine, *pEnd = sLine + strlen(sLine) - 1;
    while (sStart <= pEnd && IS_SPACE_OR_TAB(*sStart)) {
        sStart++;
    }
    while (pEnd >= sStart && IS_SPACE_OR_TAB(*pEnd)) {
        pEnd--;
    }
    if (pEnd - sStart < 2) {
        return pika_false;
    }
    if (strncmp(sStart, "```", 3) == 0) {
        bStart = pika_true;
    }
    if (pEnd - sStart >= 5 && strncmp(pEnd - 2, "```", 3) == 0) {
        bEnd = pika_true;
    }
    if (bStart && bEnd) {
        return pika_false;
    }
    if (bStart || bEnd) {
        return pika_true;
    }
    return pika_false;
}

static char* _parser_fixDocStringIndent(Parser* self, char* sDocString, int iIndent) {
    Args buffs = {0};
    char* sBuff = strsCopy(&buffs, sDocString);
    Arg* aOut = arg_newStr("");
    char* sOut = NULL;
    uint32_t iLineNum = strCountSign(sBuff, '\n');
    pika_bool bInCodeBlock = pika_false;
    int iIndentCodeBlock = 0;
    for (int i = 0; i < iLineNum; i++) {
        char* sLine = strsPopToken(&buffs, &sBuff, '\n');
        if (strIsBlank(sLine)) {
            continue;
        }
        int iIndentThis = strGetIndent(sLine);
        int iIndentStrip = iIndentThis;
        pika_bool bCodeBlockFlag = _strCheckCodeBlockFlag(sLine);
        if (bCodeBlockFlag) {
            bInCodeBlock = !bInCodeBlock;
            iIndentCodeBlock = iIndentStrip;
        }
        if (bInCodeBlock) {
            iIndentStrip = iIndentCodeBlock;
        }
        if (strGetIndent(sLine) >= iIndentStrip) {
            sLine = sLine + iIndentStrip;
        }
        for (int k = 0; k < iIndent; k++) {
            aOut = arg_strAppend(aOut, " ");
        }
        aOut = arg_strAppend(aOut, sLine);
        aOut = arg_strAppend(aOut, "\n");
        if (!bInCodeBlock) {
            iIndentCodeBlock = 0;
            aOut = arg_strAppend(aOut, "\n");
        }
    }
    sOut = strsCopy(&self->line_buffs, arg_getStr(aOut));
    strsDeinit(&buffs);
    arg_deinit(aOut);
    return sOut;
}


char* parser_ast2Doc(Parser* self, AST* oAST) {
    if (strEqu(AST_getThisBlock(oAST), "def")) {
        char* sDeclare = AST_getNodeAttr(oAST, "raw");
        int blockDeepth = AST_getBlockDeepthNow(oAST);
        self->this_blk_depth = blockDeepth;
        char* sOut =
            strsFormat(&self->line_buffs, 2048, "def %s:...\r\n", sDeclare);
        for (int i = 0; i < blockDeepth; i++) {
            sOut = strsAppend(&self->line_buffs, "", sOut);
        }
        sOut =
            strsFormat(&self->line_buffs, 2048, "``` python\n%s```\n\n", sOut);
        return sOut;
    };
    if (strEqu(AST_getThisBlock(oAST), "class")) {
        char* sDeclare = obj_getStr(oAST, "declare");
        int blockDeepth = AST_getBlockDeepthNow(oAST);
        self->this_blk_depth = blockDeepth;
        return strsFormat(&self->line_buffs, 2048, "### class %s:\r\n",
                          sDeclare);
    };
    char* sDocString = AST_getNodeAttr(oAST, "docstring");
    if (NULL != sDocString) {
        sDocString = _parser_fixDocStringIndent(self, sDocString, 0);
        return strsAppend(&self->line_buffs, sDocString, "\n");
    }
    return "";
}

char* parser_ast2Asm(Parser* self, AST* ast) {
    return AST_genAsm_top(ast, &self->line_buffs);
}

static int Parser_isVoidLine(char* sLine) {
    for (uint32_t i = 0; i < strGetSize(sLine); i++) {
        if (sLine[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

static uint8_t Parser_checkIsDocstring(char* line,
                                       Args* outbuffs,
                                       uint8_t bIsInDocstring,
                                       uint8_t* pbIsSingleDocstring,
                                       char** psDocstring) {
    pika_bool bIsDocstring = 0;
    uint32_t i = 0;
    int32_t iDocstringStart = 0;
    int32_t iDocstringEnd = -1;
    uint32_t uLineSize = strGetSize(line);
    char* sDocstring = NULL;
    while (i + 2 < uLineSize) {
        /* not match ' or " */
        if ((line[i] != '\'') && (line[i] != '"')) {
            if (!bIsDocstring && !bIsInDocstring && !charIsBlank(line[i])) {
                break;
            }
            i++;
            continue;
        }
        /* not match ''' or """ */
        if (!((line[i + 1] == line[i]) && (line[i + 2] == line[i]))) {
            i++;
            continue;
        }
        if (bIsDocstring) {
            *pbIsSingleDocstring = 1;
            iDocstringEnd = i;
            break;
        }
        bIsDocstring = 1;
        if (bIsInDocstring) {
            iDocstringEnd = i;
        } else {
            iDocstringStart = i + 3;
        }
        i++;
    }
    if (bIsDocstring) {
        sDocstring = strsCopy(outbuffs, line + iDocstringStart);
        if (iDocstringEnd != -1) {
            sDocstring[iDocstringEnd - iDocstringStart] = '\0';
        }
        *psDocstring = sDocstring;
    }
    return bIsDocstring;
}

char* parser_lines2Target(Parser* self, char* sPyLines) {
    Arg* aBackendCode = arg_newStr("");
    Arg* aLineConnection = arg_newStr("");
    Arg* aDocstring = arg_newStr("");
    uint32_t uLinesOffset = 0;
    uint16_t uLinesNum = strCountSign(sPyLines, '\n') + 1;
    uint16_t uLinesIndex = 0;
    uint8_t bIsInDocstring = 0;
    uint8_t bIsSingleDocstring = 0;
    uint8_t bIsLineConnection = 0;
    uint8_t bIsLineConnectionForBracket = 0;
    char* sOut = NULL;
    char* sBackendCode = NULL;
    char* sDocstring = NULL;
    uint32_t uLineSize = 0;
    /* parse each line */
    while (1) {
        uLinesIndex++;
        char* sLineOrigin = NULL;
        char* sLine = NULL;

        /* add void line to the end */
        if (uLinesIndex >= uLinesNum + 1) {
            sLine = "";
            goto __parse_line;
        }

        /* get single line by pop multiline */
        sLineOrigin =
            strsGetFirstToken(&self->line_buffs, sPyLines + uLinesOffset, '\n');

        sLine = strsCopy(&self->line_buffs, sLineOrigin);

        /* line connection */
        if (bIsLineConnection) {
            if (bIsLineConnectionForBracket) {
                sLine = parser_remove_comment(sLine);
                if (strEqu(sLine, "@annotation")) {
                    sLine = "";
                }
            }
            aLineConnection = arg_strAppend(aLineConnection, sLine);
            sLine = strsCopy(&self->line_buffs, arg_getStr(aLineConnection));
            /* reflash the line_connection_arg */
            arg_deinit(aLineConnection);
            aLineConnection = arg_newStr("");
            bIsLineConnection = 0;
            bIsLineConnectionForBracket = 0;
        }

        /* check connection */
        if ('\\' == sLine[strGetSize(sLine) - 1]) {
            /* remove the '\\' */
            sLine[strGetSize(sLine) - 1] = '\0';
            bIsLineConnection = 1;
            aLineConnection = arg_strAppend(aLineConnection, sLine);
            goto __next_line;
        }

        /* filter for not end \n */
        if (Parser_isVoidLine(sLine)) {
            goto __next_line;
        }

        /* filter for docstring ''' or """ */
        if (Parser_checkIsDocstring(sLine, &self->line_buffs, bIsInDocstring,
                                    &bIsSingleDocstring, &sDocstring)) {
            bIsInDocstring = ~bIsInDocstring;
            if (sDocstring[0] != '\0') {
                aDocstring = arg_strAppend(aDocstring, sDocstring);
                aDocstring = arg_strAppend(aDocstring, "\n");
            }
            /* one line docstring */
            if (bIsSingleDocstring) {
                bIsInDocstring = 0;
                bIsSingleDocstring = 0;
            }
            if (!bIsInDocstring) {
                /* multi line docstring */
                sLine = strsAppend(&self->line_buffs, "@docstring\n",
                                   arg_getStr(aDocstring));
                /* reflash the docstring_arg */
                arg_deinit(aDocstring);
                aDocstring = arg_newStr("");
                goto __parse_line;
            }
            goto __next_line;
        }

        /* skip docstring */
        if (bIsInDocstring) {
            aDocstring = arg_strAppend(aDocstring, sLine);
            aDocstring = arg_strAppend(aDocstring, "\n");
            goto __next_line;
        }

        /* support Tab */
        sLine = strsReplace(&self->line_buffs, sLine, "\t", "    ");
        /* remove \r */
        sLine = strsReplace(&self->line_buffs, sLine, "\r", "");

        /* check auto connection */
        Cursor_forEach(c, sLine) {
            Cursor_iterStart(&c);
            Cursor_iterEnd(&c);
        }
        Cursor_deinit(&c);
        /* auto connection */
        if (uLinesIndex < uLinesNum) {
            if (c.bracket_deepth > 0) {
                aLineConnection = arg_strAppend(aLineConnection, sLine);
                bIsLineConnection = 1;
                bIsLineConnectionForBracket = 1;
                goto __next_line;
            }
        }

        /* bracket match failed */
        if (c.bracket_deepth != 0) {
            sBackendCode = NULL;
            goto __parse_after;
        }

    __parse_line:
        /* parse single Line to Asm */
        if (strEqu(sLine, "#!label")) {
            self->bytecode_frame->label_pc =
                self->bytecode_frame->instruct_array.size;
            goto __next_line;
        }
        sBackendCode = parser_line2Target(self, sLine);
    __parse_after:
        if (NULL == sBackendCode) {
            sOut = NULL;
            pika_platform_printf(
                "----------[%d]----------\r\n%s\r\n-------------------------"
                "\r\n",
                uLinesIndex, sLine);
            strsDeinit(&self->line_buffs);
            goto __exit;
        }

        if (self->is_gen_bytecode) {
            /* store ByteCode */
            bc_frame_append_from_asm(self->bytecode_frame, sBackendCode);
        } else {
            /* store ASM */
            aBackendCode = arg_strAppend(aBackendCode, sBackendCode);
        }

    __next_line:
        if (uLinesIndex < uLinesNum) {
            uLineSize = strGetSize(sLineOrigin);
            uLinesOffset = uLinesOffset + uLineSize + 1;
        }
        strsDeinit(&self->line_buffs);

        /* exit when finished */
        if (uLinesIndex >= uLinesNum + 1) {
            break;
        }
    }
    if (self->is_gen_bytecode) {
        /* generate bytecode success */
        sOut = (char*)1;
    } else {
        /* load stored ASM */
        sOut = strsCopy(&self->gen_buffs, arg_getStr(aBackendCode));
    }
    goto __exit;
__exit:
    if (NULL != aBackendCode) {
        arg_deinit(aBackendCode);
    }
    if (NULL != aLineConnection) {
        arg_deinit(aLineConnection);
    }
    if (NULL != aDocstring) {
        arg_deinit(aDocstring);
    }
    return sOut;
};

char* parser_lines2Asm(Parser* self, char* sPyLines) {
    self->ast_to_target = parser_ast2Asm;
    return parser_lines2Target(self, sPyLines);
}

char* parser_lines2Doc(Parser* self, char* sPyLines) {
    self->ast_to_target = parser_ast2Doc;
    return parser_lines2Target(self, sPyLines);
}

PIKA_RES pika_lines2Bytes(ByteCodeFrame *bf, char* py_lines) {
#if PIKA_BYTECODE_ONLY_ENABLE
    pika_platform_printf(
        "Error: In bytecode-only mode, can not parse python script.\r\n");
    pika_platform_printf(
        " Note: Please check PIKA_BYTECODE_ONLY_ENABLE config.\r\n");
    return PIKA_RES_ERR_SYNTAX_ERROR;
#else
    Parser* parser = parser_create();
    parser->is_gen_bytecode = pika_true;
    parser->bytecode_frame = bf;
    if (1 == (uintptr_t)parser_lines2Target(parser, py_lines)) {
        parser_deinit(parser);
        return PIKA_RES_OK;
    }
    parser_deinit(parser);
    return PIKA_RES_ERR_SYNTAX_ERROR;
#endif
}

char* pika_lines2Asm(Args* outBuffs, char* multi_line) {
    Parser* parser = parser_create();
    parser->is_gen_bytecode = pika_false;
    char* sAsm = parser_lines2Target(parser, multi_line);
    if (NULL == sAsm) {
        parser_deinit(parser);
        return NULL;
    }
    sAsm = strsCopy(outBuffs, sAsm);
    parser_deinit(parser);
    return sAsm;
}

char* pika_file2Target(Args* outBuffs,
                       char* filename,
                       fn_parser_Lines2Target fn) {
    Args buffs = {0};
    Arg* file_arg = arg_loadFile(NULL, filename);
    pika_assert(NULL != file_arg);
    if (NULL == file_arg) {
        pika_platform_printf("Error: Can not open file: %s\r\n", filename);
        return NULL;
    }
    char* lines = (char*)arg_getBytes(file_arg);
    /* replace the "\r\n" to "\n" */
    lines = strsReplace(&buffs, lines, "\r\n", "\n");
    /* clear the void line */
    lines = strsReplace(&buffs, lines, "\n\n", "\n");
    /* add '\n' at the end */
    lines = strsAppend(&buffs, lines, "\n\n");
    Parser* parser = parser_create();
    char* res = fn(parser, lines);
    if (NULL == res) {
        goto __exit;
    }
    res = strsCopy(outBuffs, res);
__exit:
    parser_deinit(parser);
    arg_deinit(file_arg);
    strsDeinit(&buffs);
    return res;
}

char* _comprehension2Asm(Args* outBuffs, int iBlockDeepth, char* sSubStmt1, char* sSbuStmt2, char* sSubStmt3) {
    Args buffs = {0};
    /*
     * generate code for comprehension:
     * $tmp = []
     * for <substmt2> in <substmt3>:
     *   $tmp.append(<substmt1>)
     */
    Arg* aLineOut = arg_newStr("$tmp = []\n");
    aLineOut = arg_strAppend(
        aLineOut, strsFormat(&buffs, PIKA_LINE_BUFF_SIZE, "for %s in %s:\n",
                             sSbuStmt2, sSubStmt3));
    aLineOut = arg_strAppend(
        aLineOut, strsFormat(&buffs, PIKA_LINE_BUFF_SIZE,
                             "    $tmp.append(%s)\npass\n", sSubStmt1));
    aLineOut = arg_strAddIndentMulti(aLineOut, 4 * iBlockDeepth);
    char* sLineOut = arg_getStr(aLineOut);
    Parser* parser = parser_create();
    char* sAsmOut = parser_lines2Asm(parser, sLineOut);
    size_t lenAsmOut = strGetSize(sAsmOut);
    /* strip B0 */
    sAsmOut[lenAsmOut - 3] = '\0';
    sAsmOut = strsAppend(&buffs, sAsmOut, "0 REF $tmp\n");
    /* skip B0\n */
    sAsmOut = strsCopy(outBuffs, sAsmOut + 3);
    parser_deinit(parser);
    arg_deinit(aLineOut);
    strsDeinit(&buffs);
    return sAsmOut;
}

char* pika_file2Asm(Args* outBuffs, char* filename) {
    return pika_file2Target(outBuffs, filename, parser_lines2Asm);
}

char* parser_file2Doc(Parser* self, char* sPyFile) {
    return pika_file2Target(&self->gen_buffs, sPyFile, parser_lines2Doc);
}

int parser_file2TargetFile(Parser* self,
                           char* sPyFile,
                           char* sDocFile,
                           fn_parser_Lines2Target fn) {
    char* sBackendCode = pika_file2Target(&self->gen_buffs, sPyFile, fn);
    FILE* fp = pika_platform_fopen(sDocFile, "wb");
    if (NULL == fp) {
        return -1;
    }
    pika_assert(NULL != sBackendCode);
    pika_platform_fwrite(sBackendCode, 1, strGetSize(sBackendCode), fp);
    pika_platform_fclose(fp);
    return 0;
}

int parser_file2DocFile(Parser* self, char* sPyFile, char* sDocFile) {
    return parser_file2TargetFile(self, sPyFile, sDocFile, parser_lines2Doc);
}


static char* Suger_multiAssign(Args* out_buffs, char* sLine) {
#if PIKA_NANO_ENABLE
    return sLine;
#endif
    if (!strIsContain(sLine, '=') || !strIsContain(sLine, ',')) {
        return sLine;
    }
    Args buffs = {0};
    char* sLineOut = sLine;
    int iIndent = strGetIndent(sLine);
    pika_bool bAssign = pika_false;
    Arg* aStmt = arg_newStr("");
    Arg* aOutList = arg_newStr("");
    Arg* aOutItem = arg_newStr("");
    Arg* aLineOut = arg_newStr("");
    char* sLineItem = NULL;
    char* sOutList = NULL;
    int iOutNum = 0;
    Cursor_forEach(cs, sLine) {
        Cursor_iterStart(&cs);
        if (cs.bracket_deepth == 0 && strEqu(cs.token1.pyload, "=")) {
            bAssign = pika_true;
            Cursor_iterEnd(&cs);
            continue;
        }
        if (bAssign) {
            aStmt = arg_strAppend(aStmt, cs.token1.pyload);
        }
        if (!bAssign) {
            aOutList = arg_strAppend(aOutList, cs.token1.pyload);
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    if (!bAssign) {
        sLineOut = sLine;
        goto __exit;
    }

    if (!_check_is_multi_assign(arg_getStr(aOutList))) {
        sLineOut = sLine;
        goto __exit;
    }

    sLineItem = strsFormat(&buffs, PIKA_LINE_BUFF_SIZE, "$tmp= %s\n",
                           arg_getStr(aStmt));

    /* add space */
    aLineOut = arg_strAppend(aLineOut, sLineItem);

    sOutList = arg_getStr(aOutList);
    while (1) {
        char* item = Cursor_popToken(&buffs, &sOutList, ",");
        if (item[0] == '\0') {
            break;
        }
        char* sLineItem = strsFormat(&buffs, PIKA_LINE_BUFF_SIZE,
                                     "%s = $tmp[%d]\n", item, iOutNum);
        /* add space */
        aLineOut = arg_strAppend(aLineOut, sLineItem);
        iOutNum++;
    }
    /* add space */
    aLineOut = arg_strAppend(aLineOut, "del $tmp");
    aLineOut = arg_strAddIndentMulti(aLineOut, iIndent);
    sLineOut = strsCopy(out_buffs, arg_getStr(aLineOut));
    goto __exit;
__exit:
    arg_deinit(aStmt);
    arg_deinit(aOutList);
    arg_deinit(aOutItem);
    arg_deinit(aLineOut);
    strsDeinit(&buffs);
    return sLineOut;
}

static char* Suger_from_import_as(Args* buffs_p, char* sLine) {
#if !PIKA_SYNTAX_IMPORT_EX_ENABLE
    return sLine;
#endif
    Args buffs = {0};
    Arg* aLineOut = NULL;
    char* sLineOut = sLine;
    char* sClass = NULL;
    char* sModule = NULL;
    char* sAlias = NULL;
    char* sStmt = sLine + 5;
    char* sClassAfter = "";

    if (!strIsStartWith(sLine, "from ")) {
        sLineOut = sLine;
        goto __exit;
    }

    sModule = Cursor_popToken(&buffs, &sStmt, " import ");
    if (!Cursor_isContain(sStmt, TOKEN_OPERATOR, " as ")) {
        sClass = sStmt;
    } else {
        sClass = Cursor_popToken(&buffs, &sStmt, " as ");
        sAlias = sStmt;
    }

    if (NULL == sAlias) {
        sAlias = sClass;
    }

    /* skip PikaObj */
    if (strEqu(sModule, "PikaObj")) {
        sLineOut = strsCopy(buffs_p, "");
        goto __exit;
    }

    /* solve from module import * */
    if (strEqu(sClass, "*")) {
        sLineOut =
            strsFormat(&buffs, PIKA_LINE_BUFF_SIZE, "@inh %s\n", sModule);
        sLineOut = strsCopy(buffs_p, sLineOut);
        goto __exit;
    }

    while (1) {
        char* sClassItem = Cursor_popToken(&buffs, &sClass, ",");
        if (sClassItem[0] == '\0') {
            break;
        }
        sClassItem = strsFormat(&buffs, PIKA_LINE_BUFF_SIZE, "%s.%s,", sModule,
                                sClassItem);
        sClassAfter = strsAppend(&buffs, sClassAfter, sClassItem);
    }
    sClassAfter[strGetSize(sClassAfter) - 1] = '\0';
    sClass = sClassAfter;
    aLineOut = arg_newStr("import ");
    aLineOut = arg_strAppend(aLineOut, sClass);
    aLineOut = arg_strAppend(aLineOut, "\n");
    aLineOut = arg_strAppend(aLineOut, sAlias);
    aLineOut = arg_strAppend(aLineOut, " = ");
    aLineOut = arg_strAppend(aLineOut, sClass);
    sLineOut = arg_getStr(aLineOut);
    sLineOut = strsCopy(buffs_p, sLineOut);
__exit:
    if (NULL != aLineOut) {
        arg_deinit(aLineOut);
    }
    strsDeinit(&buffs);
    return sLineOut;
}

static char* Suger_import_as(Args* out_buffs, char* sLine) {
#if !PIKA_SYNTAX_IMPORT_EX_ENABLE
    return sLine;
#endif
    Args buffs = {0};
    char* sLineOut = sLine;
    char* sAlias = NULL;
    char* sOrigin = NULL;
    char* sStmt = sLine + 7;

    /* not import, exit */
    if (!strIsStartWith(sLine, "import ")) {
        sLineOut = sLine;
        goto __exit;
    }

    if (!Cursor_isContain(sStmt, TOKEN_OPERATOR, " as ")) {
        sLineOut = sLine;
        goto __exit;
    }

    /* {origin} as {alias} */
    sOrigin = Cursor_popToken(&buffs, &sStmt, " as ");
    sAlias = sStmt;

    /* 'import' and 'as' */
    sLineOut = strsFormat(&buffs, PIKA_LINE_BUFF_SIZE, "import %s\n%s = %s",
                          sOrigin, sAlias, sOrigin);
__exit:
    return strsReturnOut(&buffs, out_buffs, sLineOut);
}

static char* Suger_import(Args* outbuffs, char* sLine) {
#if !PIKA_SYNTAX_IMPORT_EX_ENABLE
    return sLine;
#endif
    sLine = Suger_import_as(outbuffs, sLine);
    sLine = Suger_from_import_as(outbuffs, sLine);
    Arg* aLineBuff = arg_newStr("");
    while (1) {
        char* sSingleLine = strPopFirstToken(&sLine, '\n');
        if (sSingleLine[0] == '\0') {
            break;
        }
        if (strIsStartWith(sSingleLine, "import ")) {
            if (strIsContain(sSingleLine, ',')) {
                sSingleLine = sSingleLine + 7;
                while (1) {
                    char* single_import = strPopFirstToken(&sSingleLine, ',');
                    if (single_import[0] == '\0') {
                        break;
                    }
                    aLineBuff = arg_strAppend(aLineBuff, "import ");
                    aLineBuff = arg_strAppend(aLineBuff, single_import);
                    aLineBuff = arg_strAppend(aLineBuff, "\n");
                }
                char* sLineAfter = arg_getStr(aLineBuff);
                sLineAfter[strlen(sLineAfter) - 1] = '\0';
            }
        }
        aLineBuff = arg_strAppend(aLineBuff, sSingleLine);
        aLineBuff = arg_strAppend(aLineBuff, "\n");
    }
    char* sLineAfter = arg_getStr(aLineBuff);
    sLineAfter[strlen(sLineAfter) - 1] = '\0';
    sLineAfter = strsCopy(outbuffs, sLineAfter);
    arg_deinit(aLineBuff);
    return sLineAfter;
}

static char* Suger_semicolon(Args* outbuffs, char* sLine) {
    Args buffs = {0};
    char* sStmt = sLine;
    char* sStmtAfter = "";
    if (Cursor_count(sLine, TOKEN_DEVIDER, ";") < 1) {
        return sLine;
    }
    while (1) {
        char* sStmtItem = Cursor_popToken(&buffs, &sStmt, ";");
        if (sStmtItem[0] == '\0') {
            break;
        };
        sStmtItem = strsAppend(&buffs, sStmtItem, "\n");
        sStmtAfter = strsAppend(&buffs, sStmtAfter, sStmtItem);
    }
    sStmtAfter[strGetSize(sStmtAfter) - 1] = '\0';
    sStmtAfter = strsCopy(outbuffs, sStmtAfter);
    strsDeinit(&buffs);
    return sStmtAfter;
}

typedef char* (*Suger_processor)(Args*, char*);
const Suger_processor Suger_processor_list[] = {Suger_import, Suger_semicolon,
                                                Suger_multiAssign};

static char* Parser_sugerProcessOnce(Args* outbuffs, char* sLine) {
    for (uint32_t i = 0; i < sizeof(Suger_processor_list) / sizeof(void*);
         i++) {
        sLine = Suger_processor_list[i](outbuffs, sLine);
        if (strCountSign(sLine, '\n') > 1) {
            break;
        }
    }
    return sLine;
}

int32_t Parser_getPyLineBlockDeepth(char* sLine) {
    int32_t iSpaceNum = strGetIndent(sLine);
    if (0 == iSpaceNum % PIKA_BLOCK_SPACE) {
        return iSpaceNum / PIKA_BLOCK_SPACE;
    }
    /* space Num is not 4N, error*/
    return -1;
}

static char* Parser_sugerProcess(Args* outbuffs, char* sLine) {
    /* process import */
    int32_t block_deepth = Parser_getPyLineBlockDeepth(sLine);
    if (block_deepth < 0) {
        return NULL;
    }
    char* sLineOrigin = sLine;
    sLine = sLine + block_deepth * PIKA_BLOCK_SPACE;
    sLine = Parser_sugerProcessOnce(outbuffs, sLine);
    if (strEqu(sLineOrigin, sLine)) {
        return sLine;
    }
    /* process multi assign */
    int iLineNum = strCountSign(sLine, '\n') + 1;
    Arg* aLine = arg_newStr("");
    for (int i = 0; i < iLineNum; i++) {
        if (i > 0) {
            aLine = arg_strAppend(aLine, "\n");
        }
        char* sSingleLine = strsPopToken(outbuffs, &sLine, '\n');
        sSingleLine = Parser_sugerProcess(outbuffs, sSingleLine);
        aLine = arg_strAppend(aLine, sSingleLine);
    }
    sLine = strsCopy(outbuffs, arg_getStr(aLine));
    sLine =
        strsAddIndentation(outbuffs, sLine, block_deepth * PIKA_BLOCK_SPACE);
    if (NULL != aLine) {
        arg_deinit(aLine);
    }
    return sLine;
}

static uint8_t Lexer_isError(char* line) {
    Args buffs = {0};
    uint8_t uRes = 0; /* not error */
    char* sTokenStream = lexer_get_token_stream(&buffs, line);
    if (NULL == sTokenStream) {
        uRes = 1; /* lex error */
        goto __exit;
    }
    goto __exit;
__exit:
    strsDeinit(&buffs);
    return uRes;
}

static char* Parser_linePreProcess(Args* outbuffs, char* sLine) {
    sLine = parser_remove_comment(sLine);

    /* check syntex error */
    if (Lexer_isError(sLine)) {
        sLine = NULL;
        goto __exit;
    }
    /* process EOL */
    sLine = strsDeleteChar(outbuffs, sLine, '\r');
    /* process syntax sugar */
    sLine = Parser_sugerProcess(outbuffs, sLine);
__exit:
    return sLine;
}

char* parser_line2Target(Parser* self, char* sLine) {
    char* sOut = NULL;
    AST* oAst = NULL;
    uint8_t uLineNum = 0;
    /* docsting */
    if (strIsStartWith(sLine, "@docstring")) {
        oAst = parser_line_to_ast(self, sLine);
        char* sBackendCode = self->ast_to_target(self, oAst);
        if (NULL == oAst) {
            sOut = "";
            goto __exit;
        }
        ast_deinit(oAst);
        sOut = sBackendCode;
        goto __exit;
    }
    /* pre process */
    sLine = Parser_linePreProcess(&self->line_buffs, sLine);
    if (NULL == sLine) {
        /* preprocess error */
        goto __exit;
    }
    if (strEqu("@annotation", sLine)) {
        sOut = "";
        goto __exit;
    }
    /*
        solve more lines
        preprocess may generate more lines
    */
    uLineNum = strCountSign(sLine, '\n') + 1;
    for (int i = 0; i < uLineNum; i++) {
        char* sSingleLine = strsPopToken(&self->line_buffs, &sLine, '\n');
        /* parse line to AST */
        oAst = parser_line_to_ast(self, sSingleLine);
        if (NULL == oAst) {
            /* parse error */
            goto __exit;
        }
        /* gen ASM from AST */
        char* sBackendCode = self->ast_to_target(self, oAst);
        if (sOut == NULL) {
            sOut = sBackendCode;
        } else {
            sOut = strsAppend(&self->line_buffs, sOut, sBackendCode);
        }
        if (NULL != oAst) {
            ast_deinit(oAst);
        }
    }
__exit:
    return sOut;
}

char* pika_lines2Array(char* sLines) {
    ByteCodeFrame bytecode_frame;
    bc_frame_init(&bytecode_frame);
    pika_lines2Bytes(&bytecode_frame, sLines);
    /* do something */
    bc_frame_print(&bytecode_frame);

    pika_platform_printf("\n\n/* clang-format off */\n");
    pika_platform_printf("PIKA_PYTHON(\n");
    pika_platform_printf("%s\n", sLines);
    pika_platform_printf(")\n");
    pika_platform_printf("/* clang-format on */\n");
    bc_frame_print_as_array(&bytecode_frame);
    /* deinit */
    bc_frame_deinit(&bytecode_frame);
    pika_platform_printf("\n\n");
    return NULL;
}

int parser_deinit(Parser* self) {
    stack_deinit(self->blk_state.stack);
    strsDeinit(&self->gen_buffs);
    pikaFree(self->blk_state.stack, sizeof(Stack));
    pikaFree(self, sizeof(Parser));
    return 0;
}


uint8_t Parser_checkIsDirect(char* sStr) {
    Args buffs = {0};
    uint8_t uRes = 0;
    pika_assert(NULL != sStr);
    char* sLeft = Cursor_splitCollect(&buffs, sStr, "=", 1);
    if (!strEqu(sLeft, sStr)) {
        uRes = 1;
        goto __exit;
    }
__exit:
    strsDeinit(&buffs);
    return uRes;
}

char* Parser_popSubStmt(Args* outbuffs, char** sStmt_p, char* sDelimiter) {
    Arg* aSubstmt = arg_newStr("");
    Arg* aNewStmt = arg_newStr("");
    char* sStmt = *sStmt_p;
    pika_bool bIsGetSubstmt = pika_false;
    Args buffs = {0};
    Cursor_forEach(cs, sStmt) {
        Cursor_iterStart(&cs);
        if (bIsGetSubstmt) {
            /* get new stmt */
            aNewStmt = arg_strAppend(aNewStmt, cs.token1.pyload);
            Cursor_iterEnd(&cs);
            continue;
        }
        if (cs.bracket_deepth > 0) {
            /* ignore */
            aSubstmt = arg_strAppend(aSubstmt, cs.token1.pyload);
            Cursor_iterEnd(&cs);
            continue;
        }
        if (strEqu(cs.token1.pyload, sDelimiter)) {
            /* found delimiter */
            bIsGetSubstmt = pika_true;
            Cursor_iterEnd(&cs);
            continue;
        }
        /* collect substmt */
        aSubstmt = arg_strAppend(aSubstmt, cs.token1.pyload);
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);

    strsDeinit(&buffs);

    char* sSubstmt = strsCacheArg(outbuffs, aSubstmt);
    char* sNewstmt = strsCacheArg(outbuffs, aNewStmt);
    *sStmt_p = sNewstmt;
    return sSubstmt;
}

int Parser_getSubStmtNum(char* sSubStmts, char* sDelimiter) {
    if (strEqu(sSubStmts, ",")) {
        return 0;
    }
    return _Cursor_count(sSubStmts, TOKEN_DEVIDER, sDelimiter, pika_true);
}

char* _Parser_popLastSubStmt(Args* outbuffs, char** sStmt_p, char* sDelimiter, pika_bool bSkipBracket) {
    uint8_t uLastStmtI = 0;
    char* stmt = *sStmt_p;
    Cursor_forEach(cs, stmt) {
        Cursor_iterStart(&cs);
        if (strIsStartWith(cs.token1.pyload, sDelimiter)) {
            /* found delimiter */

            if (bSkipBracket && cs.bracket_deepth > 0) {
                /* skip bracket */
                Cursor_iterEnd(&cs);
                continue;
            }

            /* for "[" */
            if (cs.bracket_deepth > 1) {
                /* ignore */
                Cursor_iterEnd(&cs);
                continue;
            }

            uLastStmtI = cs.iter_index;
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);

    Arg* aMainStmt = arg_newStr("");
    Arg* aLastStmt = arg_newStr("");
    {
        Cursor_forEach(cs, stmt) {
            Cursor_iterStart(&cs);
            if (cs.iter_index < uLastStmtI) {
                aMainStmt = arg_strAppend(aMainStmt, cs.token1.pyload);
            }
            if (cs.iter_index >= uLastStmtI) {
                aLastStmt = arg_strAppend(aLastStmt, cs.token1.pyload);
            }
            Cursor_iterEnd(&cs);
        }
        Cursor_deinit(&cs);
    }

    *sStmt_p = strsCacheArg(outbuffs, aMainStmt);
    return strsCacheArg(outbuffs, aLastStmt);
}

char* Parser_popLastSubStmt(Args* outbuffs, char** sStmt_p, char* sDelimiter) {
    return _Parser_popLastSubStmt(outbuffs, sStmt_p, sDelimiter, pika_true);
}