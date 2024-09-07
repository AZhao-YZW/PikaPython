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
#include "token.h"
#include "dataStrs.h"

uint8_t Token_isBranketStart(LexToken* token) {
    if (token->type != TOKEN_DEVIDER) {
        return pika_false;
    }
    if (strEqu(token->pyload, "(") || strEqu(token->pyload, "[") ||
        strEqu(token->pyload, "{")) {
        return pika_true;
    }
    return pika_false;
}

uint8_t Token_isBranketEnd(LexToken* token) {
    if (token->type != TOKEN_DEVIDER) {
        return pika_false;
    }
    if (strEqu(token->pyload, ")") || strEqu(token->pyload, "]") ||
        strEqu(token->pyload, "}")) {
        return pika_true;
    }
    return pika_false;
}

uint8_t Token_isBranket(LexToken* token) {
    return Token_isBranketStart(token) || Token_isBranketEnd(token);
}

char* TokenStream_pop(Args* buffs, char** sTokenStream_pp) {
    return strsPopToken(buffs, sTokenStream_pp, 0x1F);
}

enum TokenType Token_getType(char* sToken) {
    return (enum TokenType)sToken[0];
}

char* Token_getPyload(char* sToken) {
    return (char*)((uintptr_t)sToken + 1);
}

uint16_t TokenStream_getSize(char* tokenStream) {
    if (strEqu("", tokenStream)) {
        return 0;
    }
    return strCountSign(tokenStream, 0x1F) + 1;
}

uint8_t TokenStream_count(char* sTokenStream,
                          enum TokenType eTokenType,
                          char* sPyload) {
    Args buffs = {0};
    char* sTokenStreamBuff = strsCopy(&buffs, sTokenStream);
    uint8_t uRes = 0;
    uint16_t uTokenSize = TokenStream_getSize(sTokenStream);
    for (int i = 0; i < uTokenSize; i++) {
        char* sToken = TokenStream_pop(&buffs, &sTokenStreamBuff);
        if (eTokenType == Token_getType(sToken)) {
            if (strEqu(Token_getPyload(sToken), sPyload)) {
                uRes++;
            }
        }
    }
    strsDeinit(&buffs);
    return uRes;
}

uint8_t TokenStream_isContain(char* sTokenStream,
                              enum TokenType eTokenType,
                              char* sPyload) {
    if (TokenStream_count(sTokenStream, eTokenType, sPyload) > 0) {
        return 1;
    }
    return 0;
}

const char void_str[] = "";

void LexToken_update(struct LexToken* lex_token) {
    lex_token->type = Token_getType(lex_token->tokenStream);
    if (lex_token->type == TOKEN_STREND) {
        lex_token->pyload = (char*)void_str;
    } else {
        lex_token->pyload = Token_getPyload(lex_token->tokenStream);
    }
}

void LexToken_init(struct LexToken* lt) {
    lt->pyload = NULL;
    lt->tokenStream = NULL;
    lt->type = TOKEN_STREND;
}