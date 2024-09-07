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
#include "cursor.h"
#include "lexer.h"
#include "token.h"
#include "dataArgs.h"
#include "dataStrs.h"

void Cursor_iterStart(struct Cursor* cs) {
    cs->iter_index++;
    cs->iter_buffs = New_strBuff();
    /* token1 is the last token */
    cs->token1.tokenStream =
        strsCopy(cs->iter_buffs, arg_getStr(cs->last_token));
    /* token2 is the next token */
    cs->token2.tokenStream = TokenStream_pop(cs->iter_buffs, &cs->tokenStream);
    /* store last token */
    arg_deinit(cs->last_token);
    cs->last_token = arg_newStr(cs->token2.tokenStream);

    LexToken_update(&cs->token1);
    LexToken_update(&cs->token2);
    if (strEqu(cs->token1.pyload, "(")) {
        cs->bracket_deepth++;
    }
    if (strEqu(cs->token1.pyload, ")")) {
        cs->bracket_deepth--;
    }
    if (strEqu(cs->token1.pyload, "[")) {
        cs->bracket_deepth++;
    }
    if (strEqu(cs->token1.pyload, "]")) {
        cs->bracket_deepth--;
    }
    if (strEqu(cs->token1.pyload, "{")) {
        cs->bracket_deepth++;
    }
    if (strEqu(cs->token1.pyload, "}")) {
        cs->bracket_deepth--;
    }
}

void Cursor_iterEnd(struct Cursor* cs) {
    args_deinit(cs->iter_buffs);
}

void Cursor_deinit(struct Cursor* cs) {
    if (NULL != cs->last_token) {
        arg_deinit(cs->last_token);
    }
    args_deinit(cs->buffs_p);
}

char* Cursor_popLastToken(Args* outBuffs, char** pStmt, char* str) {
    char* sStmts = *pStmt;
    uint8_t uDividerIndex = 0;
    Arg* aKeeped = arg_newStr("");
    Arg* aPoped = arg_newStr("");
    Cursor_forEach(cs, sStmts) {
        Cursor_iterStart(&cs);
        if (cs.bracket_deepth == 0) {
            if (strEqu(str, cs.token1.pyload)) {
                uDividerIndex = cs.iter_index;
            }
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    Cursor_forEachExistPs(cs, sStmts) {
        Cursor_iterStart(&cs);
        if (cs.iter_index < uDividerIndex) {
            aPoped = arg_strAppend(aPoped, cs.token1.pyload);
        }
        if (cs.iter_index > uDividerIndex) {
            aKeeped = arg_strAppend(aKeeped, cs.token1.pyload);
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    char* sKeeped = strsCopy(outBuffs, arg_getStr(aKeeped));
    char* sPoped = arg_getStr(aPoped);
    pika_platform_memcpy(sStmts, sPoped, strGetSize(sPoped) + 1);
    arg_deinit(aPoped);
    arg_deinit(aKeeped);
    return sKeeped;
}

char* Cursor_getCleanStmt(Args* outBuffs, char* cmd) {
    pika_assert(cmd != NULL);
    int32_t iSize = strGetSize(cmd);
    /* lexer may generate more chars than input */
    char* sOut = args_getBuff(outBuffs, iSize * 2);
    int32_t iOut = 0;
    Cursor_forEach(cs, cmd) {
        Cursor_iterStart(&cs);
        for (uint16_t k = 0; k < strGetSize(cs.token1.pyload); k++) {
            sOut[iOut] = cs.token1.pyload[k];
            iOut++;
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    /* add \0 */
    sOut[iOut] = 0;
    return sOut;
}

char* Cursor_removeTokensBetween(Args* outBuffs, char* input, char* token_pyload1, char* token_pyload2) {
    Args buffs = {0};
    uint8_t uBlockDeepth = 0;
    char* sOutput = "";
    Cursor_forEach(cs, input) {
        Cursor_iterStart(&cs);
        if (strEqu(token_pyload1, cs.token1.pyload)) {
            if (uBlockDeepth == 0) {
                sOutput = strsAppend(&buffs, sOutput, cs.token1.pyload);
            }
            uBlockDeepth++;
        }
        if (strEqu(token_pyload2, cs.token1.pyload)) {
            uBlockDeepth--;
        }
        if (uBlockDeepth == 0) {
            sOutput = strsAppend(&buffs, sOutput, cs.token1.pyload);
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    sOutput = strsCopy(outBuffs, sOutput);
    strsDeinit(&buffs);
    return sOutput;
}

void _Cursor_init(struct Cursor* cs) {
    cs->tokenStream = NULL;
    cs->length = 0;
    cs->iter_index = 0;
    cs->bracket_deepth = 0;
    cs->last_token = NULL;
    cs->iter_buffs = NULL;
    cs->buffs_p = New_strBuff();
    cs->result = PIKA_RES_OK;
    LexToken_init(&cs->token1);
    LexToken_init(&cs->token2);
}

void _Cursor_parse(struct Cursor* cs, char* stmt) {
    if (NULL == stmt) {
        cs->result = PIKA_RES_ERR_SYNTAX_ERROR;
        return;
    }
    cs->tokenStream = lexer_get_token_stream(cs->buffs_p, stmt);
    if (NULL == cs->tokenStream) {
        cs->result = PIKA_RES_ERR_SYNTAX_ERROR;
        return;
    }
    cs->length = TokenStream_getSize(cs->tokenStream);
}

void _Cursor_beforeIter(struct Cursor* cs) {
    /* clear first token */
    if (cs->result != PIKA_RES_OK) {
        return;
    }
    TokenStream_pop(cs->buffs_p, &cs->tokenStream);
    cs->last_token = arg_newStr(TokenStream_pop(cs->buffs_p, &cs->tokenStream));
}

uint8_t _Cursor_count(char* sStmt,
                      TokenType eType,
                      char* sPyload,
                      pika_bool bSkipbracket) {
    /* fast return */
    if (!strstr(sStmt, sPyload)) {
        return pika_false;
    }
    Args buffs = {0};
    uint8_t uRes = 0;
    Cursor_forEach(cs, sStmt) {
        Cursor_iterStart(&cs);
        if (cs.token1.type == eType && (strEqu(cs.token1.pyload, sPyload))) {
            if (bSkipbracket) {
                uint8_t branket_deepth_check = 0;
                if (Token_isBranketStart(&cs.token1)) {
                    branket_deepth_check = 1;
                }
                if (cs.bracket_deepth > branket_deepth_check) {
                    /* skip bracket */
                    Cursor_iterEnd(&cs);
                    continue;
                }
            }
            uRes++;
        }
        Cursor_iterEnd(&cs);
    };
    Cursor_deinit(&cs);
    strsDeinit(&buffs);
    return uRes;
}

uint8_t Cursor_count(char* sStmt, TokenType eType, char* sPyload) {
    return _Cursor_count(sStmt, eType, sPyload, pika_false);
}

pika_bool Cursor_isContain(char* sStmt, TokenType eType, char* sPyload) {
    if (Cursor_count(sStmt, eType, sPyload) > 0) {
        return pika_true;
    }
    return pika_false;
}

char* Cursor_popToken(Args* buffs, char** sStmt_p, char* sDevide) {
    Arg* aOutitem = arg_newStr("");
    Arg* aTokenStreamAfter = arg_newStr("");
    pika_bool bFindDevide = pika_false;
    Cursor_forEach(cs, *sStmt_p) {
        Cursor_iterStart(&cs);
        if (!bFindDevide) {
            if ((cs.bracket_deepth == 0 && strEqu(cs.token1.pyload, sDevide)) ||
                cs.iter_index == cs.length) {
                bFindDevide = pika_true;
                Cursor_iterEnd(&cs);
                continue;
            }
        }
        if (!bFindDevide) {
            aOutitem = arg_strAppend(aOutitem, cs.token1.pyload);
        } else {
            aTokenStreamAfter =
                arg_strAppend(aTokenStreamAfter, cs.token1.pyload);
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    /* cache out item */
    char* sOutItem = strsCopy(buffs, arg_getStr(aOutitem));
    arg_deinit(aOutitem);
    /* cache tokenStream after */
    char* sTokenAfter = strsCopy(buffs, arg_getStr(aTokenStreamAfter));
    arg_deinit(aTokenStreamAfter);
    /* update tokenStream */
    *sStmt_p = sTokenAfter;
    return sOutItem;
}

char* Cursor_splitCollect(Args* buffs, char* sStmt, char* sDevide, int index) {
    Arg* aOut = arg_newStr("");
    int iExpectBracket = 0;
    if (sDevide[0] == '(' || sDevide[0] == '[' || sDevide[0] == '{') {
        iExpectBracket = 1;
    }
    int i = 0;
    Cursor_forEach(cs, sStmt) {
        Cursor_iterStart(&cs);
        if (cs.bracket_deepth == iExpectBracket &&
            strEqu(cs.token1.pyload, sDevide)) {
            i++;
            Cursor_iterEnd(&cs);
            continue;
        }
        if (i == index) {
            aOut = arg_strAppend(aOut, cs.token1.pyload);
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    /* if not found, return origin string */
    if (i == 0) {
        arg_deinit(aOut);
        aOut = arg_newStr(sStmt);
    }
    return strsCacheArg(buffs, aOut);
}

char* _remove_sub_stmt(Args* outBuffs, char* sStmt) {
    Args buffs = {0};
    sStmt = strsCopy(&buffs, sStmt);
    sStmt = Cursor_removeTokensBetween(&buffs, sStmt, "(", ")");
    sStmt = Cursor_removeTokensBetween(&buffs, sStmt, "[", "]");
    sStmt = Cursor_removeTokensBetween(&buffs, sStmt, "{", "}");
    char* sRes = args_cacheStr(outBuffs, sStmt);
    strsDeinit(&buffs);
    return sRes;
}

static void Slice_getPars(Args* outBuffs, char* sInner, char** sStart_p, char** sEnd_p, char** sStep_p) {
#if PIKA_NANO_ENABLE
    return;
#endif
    Args buffs = {0};
    *sStart_p = "";
    *sEnd_p = "";
    *sStep_p = "";

    /* slice */
    uint8_t uColonIndex = 0;
    Cursor_forEach(cs, sInner) {
        Cursor_iterStart(&cs);
        if (strEqu(cs.token1.pyload, ":") && cs.bracket_deepth == 0) {
            uColonIndex++;
            goto __iter_continue1;
        }
        if (uColonIndex == 0) {
            *sStart_p = strsAppend(&buffs, *sStart_p, cs.token1.pyload);
        }
        if (uColonIndex == 1) {
            *sEnd_p = strsAppend(&buffs, *sEnd_p, cs.token1.pyload);
        }
        if (uColonIndex == 2) {
            *sStep_p = strsAppend(&buffs, *sStep_p, cs.token1.pyload);
        }
    __iter_continue1:
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    if (uColonIndex == 1) {
        *sStep_p = "1";
        if (strEqu(*sStart_p, "")) {
            *sStart_p = "0";
        }
        if (strEqu(*sEnd_p, "")) {
            *sEnd_p = "-1";
        }
    }
    if (uColonIndex == 0) {
        *sEnd_p = strsAppend(&buffs, *sStart_p, " + 1");
        *sStep_p = "1";
    }

    /* slice with step */

    /* output */
    *sStart_p = strsCopy(outBuffs, *sStart_p);
    *sEnd_p = strsCopy(outBuffs, *sEnd_p);
    *sStep_p = strsCopy(outBuffs, *sStep_p);
    /* clean */
    strsDeinit(&buffs);
}

char* Suger_leftSlice(Args* outBuffs, char* sRight, char** sLeft_p) {
#if !PIKA_SYNTAX_SLICE_ENABLE
    return sRight;
#endif
    /* init objects */
    Args buffs = {0};
    Arg* aRight = arg_newStr("");
    char* sLeft = *sLeft_p;
    pika_bool bInBrancket = 0;
    args_setStr(&buffs, "inner", "");
    pika_bool bMatched = 0;
    char* sRightRes = NULL;
    /* exit when NULL */
    if (NULL == sLeft) {
        arg_deinit(aRight);
        aRight = arg_newStr(sRight);
        goto __exit;
    }
    /* exit when not match
         (symble|iteral)'['
    */
    Cursor_forEach(cs, sLeft) {
        Cursor_iterStart(&cs);
        if (strEqu(cs.token2.pyload, "[")) {
            if (TOKEN_SYMBOL == cs.token1.type ||
                TOKEN_LITERAL == cs.token1.type) {
                bMatched = 1;
                Cursor_iterEnd(&cs);
                break;
            }
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    if (!bMatched) {
        /* not contain '[', return origin */
        arg_deinit(aRight);
        aRight = arg_newStr(sRight);
        goto __exit;
    }

    /* matched [] */
    Cursor_forEachExistPs(cs, sLeft) {
        Cursor_iterStart(&cs);
        /* found '[' */
        if ((TOKEN_DEVIDER == cs.token2.type) &&
            (strEqu(cs.token2.pyload, "["))) {
            /* get 'obj' from obj[] */
            args_setStr(&buffs, "obj", cs.token1.pyload);
            bInBrancket = 1;
            /* fond ']' */
        } else if ((TOKEN_DEVIDER == cs.token2.type) &&
                   (strEqu(cs.token2.pyload, "]"))) {
            bInBrancket = 0;
            char* sInner = args_getStr(&buffs, "inner");
            Arg* aInner = arg_newStr(sInner);
            aInner = arg_strAppend(aInner, cs.token1.pyload);
            args_setStr(&buffs, "inner", arg_getStr(aInner));
            arg_deinit(aInner);
            /* update inner pointer */
            sInner = args_getStr(&buffs, "inner");
            char* sStart = NULL;
            char* sEnd = NULL;
            char* sStep = NULL;
            Slice_getPars(&buffs, sInner, &sStart, &sEnd, &sStep);
            /* obj = __setitem__(obj, key, val) */
            aRight = arg_strAppend(aRight, "__setitem__(");
            aRight = arg_strAppend(aRight, args_getStr(&buffs, "obj"));
            aRight = arg_strAppend(aRight, ",");
            aRight = arg_strAppend(aRight, sStart);
            aRight = arg_strAppend(aRight, ",");
            aRight = arg_strAppend(aRight, sRight);
            aRight = arg_strAppend(aRight, ")");
            /* clean the inner */
            args_setStr(&buffs, "inner", "");
            /* in brancket and found '[' */
        } else if (bInBrancket && (!strEqu(cs.token1.pyload, "["))) {
            char* sInner = args_getStr(&buffs, "inner");
            Arg* aIndex = arg_newStr(sInner);
            aIndex = arg_strAppend(aIndex, cs.token1.pyload);
            args_setStr(&buffs, "inner", arg_getStr(aIndex));
            arg_deinit(aIndex);
            /* out of brancket and not found ']' */
        } else if (!bInBrancket && (!strEqu(cs.token1.pyload, "]"))) {
            if (TOKEN_STREND != cs.token1.type) {
                aRight = arg_strAppend(aRight, cs.token1.pyload);
            }
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    /* clean the left */
    for (size_t i = 0; i < strGetSize(sLeft); i++) {
        if (sLeft[i] == '[') {
            sLeft[i] = '\0';
            break;
        }
    }
__exit:
    /* clean and return */
    sRightRes = strsCopy(outBuffs, arg_getStr(aRight));
    arg_deinit(aRight);
    strsDeinit(&buffs);
    return sRightRes;
}

char* Suger_format(Args* outBuffs, char* sRight) {
#if !PIKA_SYNTAX_FORMAT_ENABLE
    return sRight;
#endif
    /* quick skip */
    if (!strIsContain(sRight, '%')) {
        return sRight;
    }

    pika_bool bFormat = pika_false;
    Cursor_forEach(ps1, sRight) {
        Cursor_iterStart(&ps1);
        if (ps1.bracket_deepth == 0 && strEqu(ps1.token1.pyload, "%")) {
            bFormat = pika_true;
        }
        Cursor_iterEnd(&ps1);
    }
    Cursor_deinit(&ps1);
    if (pika_false == bFormat) {
        return sRight;
    }

    char* sRes = sRight;
    Arg* aStrBuf = arg_newStr("");
    Arg* aVarBuf = arg_newStr("");
    pika_bool bInFormat = pika_false;
    pika_bool bTuple = pika_false;
    pika_bool bOutVars = pika_false;
    Args buffs = {0};
    char* sFmt = NULL;
    Cursor_forEach(cs, sRight) {
        char* sItem = "";
        Cursor_iterStart(&cs);
        if (pika_false == bInFormat) {
            if (cs.token1.type != TOKEN_LITERAL) {
                sItem = cs.token1.pyload;
                goto __iter_continue;
            }
            if (cs.token1.pyload[0] != '\'' && cs.token1.pyload[0] != '"') {
                sItem = cs.token1.pyload;
                goto __iter_continue;
            }
            if (!strEqu(cs.token2.pyload, "%")) {
                sItem = cs.token1.pyload;
                goto __iter_continue;
            }
            /* found the format stmt */
            bInFormat = pika_true;
            sFmt = strsCopy(&buffs, cs.token1.pyload);
            goto __iter_continue;
        }
        if (pika_true == bInFormat) {
            /* check the format vars */
            if (strEqu(cs.token1.pyload, "%")) {
                /* is a tuple */
                if (strEqu(cs.token2.pyload, "(")) {
                    bTuple = pika_true;
                } else {
                    aVarBuf = arg_strAppend(aVarBuf, cs.token2.pyload);
                }
                goto __iter_continue;
            }
            /* found the end of tuple */
            if (cs.iter_index == cs.length) {
                bOutVars = pika_true;
                bInFormat = pika_false;
            } else {
                /* push the vars inner the tuple */
                aVarBuf = arg_strAppend(aVarBuf, cs.token2.pyload);
            }
            if (bOutVars) {
                if (bTuple) {
                    aStrBuf = arg_strAppend(aStrBuf, "cformat(");
                    aStrBuf = arg_strAppend(aStrBuf, sFmt);
                    aStrBuf = arg_strAppend(aStrBuf, ",");
                    aStrBuf = arg_strAppend(aStrBuf, arg_getStr(aVarBuf));
                } else {
                    aStrBuf = arg_strAppend(aStrBuf, "cformat(");
                    aStrBuf = arg_strAppend(aStrBuf, sFmt);
                    aStrBuf = arg_strAppend(aStrBuf, ",");
                    aStrBuf = arg_strAppend(aStrBuf, arg_getStr(aVarBuf));
                    aStrBuf = arg_strAppend(aStrBuf, ")");
                }
            }
        }
    __iter_continue:
        if (!bInFormat) {
            aStrBuf = arg_strAppend(aStrBuf, sItem);
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);

    sRes = strsCopy(outBuffs, arg_getStr(aStrBuf));
    arg_deinit(aStrBuf);
    arg_deinit(aVarBuf);
    strsDeinit(&buffs);
    return sRes;
}

char* _Suger_process(Args* out_buffs,
                     char* sLine,
                     char* sToken1,
                     char* sToken2,
                     char* sFormat) {
    char* sRet = sLine;
    char* sStmt1 = "";
    char* sStmt2 = "";
    pika_bool bGotTokens = pika_false;
    pika_bool bSkip = pika_false;
    Args buffs = {0};

    if (1 != Cursor_count(sLine, TOKEN_OPERATOR, sToken1)) {
        sRet = sLine;
        goto __exit;
    }
    if (1 != Cursor_count(sLine, TOKEN_OPERATOR, sToken2)) {
        sRet = sLine;
        goto __exit;
    }

    Cursor_forEach(cs, sLine) {
        Cursor_iterStart(&cs);
        if (!bGotTokens) {
            if (strEqu(cs.token1.pyload, sToken1) &&
                strEqu(cs.token2.pyload, sToken2)) {
                bGotTokens = pika_true;
                Cursor_iterEnd(&cs);
                continue;
            }
            sStmt1 = strsAppend(&buffs, sStmt1, cs.token1.pyload);
        } else {
            if (!bSkip) {
                bSkip = pika_true;
                Cursor_iterEnd(&cs);
                continue;
            }
            sStmt2 = strsAppend(&buffs, sStmt2, cs.token1.pyload);
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);

    if (!bGotTokens) {
        sRet = sLine;
        goto __exit;
    }

    sRet = strsFormat(out_buffs,
                      strGetSize(sLine) + strlen(sToken1) + strlen(sToken2),
                      sFormat, sStmt1, sStmt2);

__exit:
    strsDeinit(&buffs);
    return sRet;
}

char* Suger_not_in(Args* out_buffs, char* sLine) {
    return _Suger_process(out_buffs, sLine, " not ", " in ", " not %s in %s");
}

char* Suger_is_not(Args* out_buffs, char* sLine) {
    return _Suger_process(out_buffs, sLine, " is ", " not ", " not %s is %s");
}

char* Suger_multiReturn(Args* out_buffs, char* sLine) {
#if PIKA_NANO_ENABLE
    return sLine;
#endif
    Cursor_forEach(cs, sLine) {
        Cursor_iterStart(&cs);
        if (cs.bracket_deepth == 0 && strEqu(cs.token1.pyload, ",")) {
            sLine = strsFormat(out_buffs, strGetSize(sLine) + 3, "(%s)", sLine);
            Cursor_iterEnd(&cs);
            break;
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    return sLine;
}

#define SELF_OPERATORES_LEN 4
static const char selfOperators[][SELF_OPERATORES_LEN] = {
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "**=", "//="};

uint8_t Suger_selfOperator(Args* outbuffs, char* sStmt, char** sRight_p, char** sLeft_p) {
    char* sLeftNew = NULL;
    char* sRightNew = NULL;
    Arg* aLeft = arg_newStr("");
    Arg* aRight = arg_newStr("");
    Arg* aRightNew = arg_newStr("");
    pika_bool bLeftExist = 0;

    Args buffs = {0};
    char _sOperator[3] = {0};
    char* sOperator = (char*)_sOperator;
    pika_bool bRight = 0;
    for (uint8_t i = 0; i < sizeof(selfOperators) / SELF_OPERATORES_LEN; i++) {
        if (Cursor_isContain(sStmt, TOKEN_OPERATOR, (char*)selfOperators[i])) {
            pika_platform_memcpy(sOperator, selfOperators[i],
                                 strGetSize((char*)selfOperators[i]) - 1);
            break;
        }
    }
    /* not found self operator */
    if (sOperator[0] == 0) {
        goto __exit;
    }
    /* found self operator */
    bLeftExist = 1;
    Cursor_forEach(cs, sStmt) {
        Cursor_iterStart(&cs);
        for (uint8_t i = 0; i < sizeof(selfOperators) / SELF_OPERATORES_LEN;
             i++) {
            if (strEqu(cs.token1.pyload, (char*)selfOperators[i])) {
                bRight = 1;
                goto __iter_continue;
            }
        }
        if (!bRight) {
            aLeft = arg_strAppend(aLeft, cs.token1.pyload);
        } else {
            aRight = arg_strAppend(aRight, cs.token1.pyload);
        }
    __iter_continue:
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    /* connect right */
    aRightNew = arg_strAppend(aRightNew, arg_getStr(aLeft));
    aRightNew = arg_strAppend(aRightNew, sOperator);
    aRightNew = arg_strAppend(aRightNew, "(");
    aRightNew = arg_strAppend(aRightNew, arg_getStr(aRight));
    aRightNew = arg_strAppend(aRightNew, ")");

    /* collect left_new and right_new */
    sLeftNew = arg_getStr(aLeft);
    sRightNew = arg_getStr(aRightNew);

__exit:
    strsDeinit(&buffs);
    if (NULL != sRightNew) {
        *(sRight_p) = strsCopy(outbuffs, sRightNew);
        ;
    }
    if (NULL != sLeftNew) {
        *(sLeft_p) = strsCopy(outbuffs, sLeftNew);
    }
    arg_deinit(aRight);
    arg_deinit(aLeft);
    arg_deinit(aRightNew);
    return bLeftExist;
}

pika_bool _check_is_multi_assign(char* sArgList) {
#if PIKA_NANO_ENABLE
    return pika_false;
#endif
    pika_bool bRes = pika_false;
    Cursor_forEach(cs, sArgList) {
        Cursor_iterStart(&cs);
        if ((cs.bracket_deepth == 0 && strEqu(cs.token1.pyload, ","))) {
            bRes = pika_true;
        }
        Cursor_iterEnd(&cs);
    }
    Cursor_deinit(&cs);
    return bRes;
}