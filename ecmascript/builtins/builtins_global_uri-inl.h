/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_GLOBAL_URI_INL_H
#define ECMASCRIPT_BUILTINS_GLOBAL_URI_INL_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_global_uri.h"

namespace panda::ecmascript::builtins {

bool BuiltinsGlobal::IsUnescapedURI(uint16_t ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
        return true;
    }
    return IsInMarkURISet(ch);
}

bool BuiltinsGlobal::IsInUnescapedURISet(uint16_t ch)
{
    if (ch == '#') {
        return true;
    }
    return IsUnescapedURI(ch) || IsReservedURI(ch);
}

bool BuiltinsGlobal::IsInReservedURISet(uint16_t ch)
{
    if (ch == '#') {
        return true;
    }
    return IsReservedURI(ch);
}

#if ENABLE_NEXT_OPTIMIZATION
bool BuiltinsGlobal::IsReservedURI(uint16_t ch)
{
    if (ch == '@') {
        return true;
    }

    if (ch > BITMAP_SIZE) {
        return false;
    }

    return ((1ULL << ch) & RESERVED_URI_MASK) != 0;
}

bool BuiltinsGlobal::IsInMarkURISet(uint16_t ch)
{
    if (ch == '_' || ch == '~') {
        return true;
    }

    if (ch > BITMAP_SIZE) {
        return false;
    }

    return ((1ULL << ch) & MARK_URI_MASK) != 0;
}
#endif // ENABLE_NEXT_OPTIMIZATION
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_GLOBAL_URI_INL_H