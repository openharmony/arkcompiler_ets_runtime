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

#ifndef ECMASCRIPT_JS_THREAD_HCLASS_ENTRIES_H
#define ECMASCRIPT_JS_THREAD_HCLASS_ENTRIES_H

#include <array>
#include <string_view>

#include "ecmascript/common.h"
#include "ecmascript/ts_types/builtin_type_id.h"

namespace panda::ecmascript {
class GlobalEnv;
class JSHClass;

// Records initial HClass of builtin objects.
// Let X be the builtin object (e.g. X = Array, object, etc.),
// HClass address mismatch happens if properties in X or X.prototype are modified (with HClass change),
// thus the record entries are useful to detect builtin object modification.
struct BuiltinHClassEntries {
#define TS_BUILTIN_TYPE_ITEM(TYPE) BuiltinTypeId::TYPE,
    static constexpr std::array BUILTIN_TYPES = {
        TS_BUILTIN_TYPE_LIST(TS_BUILTIN_TYPE_ITEM)
    };
    static constexpr size_t N_ENTRIES = static_cast<size_t>(BuiltinTypeId::NUM_OF_BUILTIN_TYPES);
#undef TS_BUILTIN_TYPE_ITEM

    struct Entry {
        // Let X be the builtin object (e.g. X = Array, Object, etc.)
        // builtinHClass = HClass of X
        JSHClass *builtinHClass = nullptr;
        // Let x be the builtin instance object (e.g. x = new Array(), new Object(), etc.)
        // instanceHClass = HClass of x
        JSHClass *instanceHClass = nullptr;
        // prototypeHClass = HClass of X.prototype
        JSHClass *prototypeHClass = nullptr;
        // prototypeOfPrototypeHClass = HClass of X.prototype.prototype
        JSHClass *prototypeOfPrototypeHClass = nullptr;
        //extraHClass . In typedArray means instanceHClassOnHeap
        JSHClass *extraHClass = nullptr;
    };
    Entry entries[N_ENTRIES];

    static size_t GetEntryIndex(BuiltinTypeId type)
    {
        size_t res = static_cast<size_t>(type) - INDEX_OFFSET;
        ASSERT(res < N_ENTRIES);
        return res;
    }

    size_t EntryIsValid(BuiltinTypeId type) const
    {
        size_t index = GetEntryIndex(type);
        return entries[index].builtinHClass != nullptr && entries[index].prototypeHClass != nullptr;
    }

    static size_t GetBuiltinHClassOffset(BuiltinTypeId type)
    {
        size_t index = GetEntryIndex(type);
        return sizeof(Entry) * index + MEMBER_OFFSET(Entry, builtinHClass);
    }

    static size_t GetInstanceHClassOffset(BuiltinTypeId type)
    {
        size_t index = GetEntryIndex(type);
        return sizeof(Entry) * index + MEMBER_OFFSET(Entry, instanceHClass);
    }

    static size_t GetExtraHClassOffset(BuiltinTypeId type)
    {
        size_t index = GetEntryIndex(type);
        return sizeof(Entry) * index + MEMBER_OFFSET(Entry, extraHClass);
    }

    static size_t GetPrototypeHClassOffset(BuiltinTypeId type)
    {
        size_t index = GetEntryIndex(type);
        return sizeof(Entry) * index + MEMBER_OFFSET(Entry, prototypeHClass);
    }

    static size_t GetPrototypeOfPrototypeHClassOffset(BuiltinTypeId type)
    {
        size_t index = GetEntryIndex(type);
        return sizeof(Entry) * index + MEMBER_OFFSET(Entry, prototypeOfPrototypeHClass);
    }

    static constexpr size_t SizeArch32 = sizeof(entries);
    static constexpr size_t SizeArch64 = sizeof(entries);

private:
    static constexpr size_t INDEX_OFFSET = static_cast<size_t>(BuiltinTypeId::BUILTIN_OFFSET) + 1;
};
STATIC_ASSERT_EQ_ARCH(sizeof(BuiltinHClassEntries),
    BuiltinHClassEntries::SizeArch32,
    BuiltinHClassEntries::SizeArch64);
} // namespace panda::ecmascript

#endif // ECMASCRIPT_JS_THREAD_HCLASS_ENTRIES_H
