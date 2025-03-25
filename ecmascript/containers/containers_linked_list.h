/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_LINKED_LIST_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_LINKED_LIST_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in LinkedList.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersLinkedList::func refers to the native implementation of LinkedList.prototype[name].
#define CONTAINER_LINKEDLIST_PROTOTYPE_FUNCTIONS(V)                                         \
    V("add",                    Add,                    1,          INVALID)                \
    V("insert",                 Insert,                 2,          INVALID)                \
    V("clear",                  Clear,                  0,          INVALID)                \
    V("clone",                  Clone,                  0,          INVALID)                \
    V("removeFirst",            RemoveFirst,            0,          INVALID)                \
    V("removeLast",             RemoveLast,             0,          INVALID)                \
    V("removeFirstFound",       RemoveFirstFound,       1,          INVALID)                \
    V("removeByIndex",          RemoveByIndex,          1,          INVALID)                \
    V("remove",                 Remove,                 1,          INVALID)                \
    V("removeLastFound",        RemoveLastFound,        1,          INVALID)                \
    V("has",                    Has,                    1,          INVALID)                \
    V("get",                    Get,                    1,          INVALID)                \
    V("addFirst",               AddFirst,               1,          INVALID)                \
    V("getFirst",               GetFirst,               0,          INVALID)                \
    V("getLast",                GetLast,                0,          INVALID)                \
    V("getIndexOf",             GetIndexOf,             1,          INVALID)                \
    V("getLastIndexOf",         GetLastIndexOf,         1,          INVALID)                \
    V("convertToArray",         ConvertToArray,         0,          INVALID)                \
    V("set",                    Set,                    2,          INVALID)                \
    V("forEach",                ForEach,                2,          LinkedListForEach)

namespace panda::ecmascript::containers {
class ContainersLinkedList : public base::BuiltinsBase {
public:
    static JSTaggedValue LinkedListConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLast(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Insert(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue AddFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clone(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLastIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveByIndex(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveFirstFound(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveLast(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveLastFound(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Length(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ConvertToArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetLinkedListPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(LINKEDLIST_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_LINKEDLIST_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersLinkedList::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array LINKEDLIST_PROTOTYPE_FUNCTIONS = {
        CONTAINER_LINKEDLIST_PROTOTYPE_FUNCTIONS(CONTAINER_LINKEDLIST_FUNCTION_ENTRY)
    };
#undef CONTAINER_LINKEDLIST_FUNCTION_ENTRY

};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_LINKED_LIST_H
