/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_BIT_VECTOR_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_BIT_VECTOR_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::containers {
class ContainersBitVector : public base::BuiltinsBase {
public:
    static JSTaggedValue BitVectorConstructor(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Push(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Pop(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue SetBitsByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetBitsByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Resize(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetBitCountByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetIndexOf(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetLastIndexOf(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue FlipBitByIndex(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue FlipBitsByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue SetAllBits(EcmaRuntimeCallInfo* argv);
private:
    static void FreeBitsetVectorPointer([[maybe_unused]] void *env, void *pointer, void *data);
};
} // namespace panda::ecmascript::containers
#endif // ECMASCRIPT_CONTAINERS_CONTAINERS_BIT_VECTOR_H
