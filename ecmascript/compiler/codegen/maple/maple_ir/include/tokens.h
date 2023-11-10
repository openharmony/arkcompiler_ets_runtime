/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MAPLE_IR_INCLUDE_TOKENS_H
#define MAPLE_IR_INCLUDE_TOKENS_H

namespace maple {
enum TokenKind {
    TK_invalid,
// keywords from this file
#define KEYWORD(STR) TK_##STR,
#include "keywords.def"
#undef KEYWORD
    // non-keywords starting here
    // constants
    TK_intconst,
    TK_floatconst,
    TK_doubleconst,
    // local name
    TK_lname,
    // global name
    TK_gname,
    // function name
    TK_fname,
    // pseudo register
    TK_preg,
    // special register
    TK_specialreg,
    // parent field
    TK_prntfield,
    // type parameter name
    TK_typeparam,
    // misc.
    TK_newline,
    TK_lparen,     // (
    TK_rparen,     // )
    TK_lbrace,     // {
    TK_rbrace,     // }
    TK_lbrack,     // [
    TK_rbrack,     // ]
    TK_langle,     // <
    TK_rangle,     // >
    TK_eqsign,     // =
    TK_coma,       // ,
    TK_dotdotdot,  // ...
    TK_colon,      // :
    TK_asterisk,   // *
    TK_string,     // a literal string enclosed between "
    TK_eof
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_TOKENS_H
