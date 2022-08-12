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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_STUBS_INL_H
#define ECMASCRIPT_COMPILER_BUILTINS_STUBS_INL_H

#include "ecmascript/compiler/builtins/builtins_stubs.h"

namespace panda::ecmascript::kungfu {
GateRef BuiltinsStubBuilder::StringAt(GateRef obj, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    Label exit(env);
    Label isUtf16(env);
    Label isUtf8(env);
    Label doIntOp(env);
    Label leftIsNumber(env);
    Label rightIsNumber(env);
    GateRef dataUtf16 = PtrAdd(obj, IntPtr(EcmaString::DATA_OFFSET));
    Branch(IsUtf16String(obj), &isUtf16, &isUtf8);
    Bind(&isUtf16);
    {
        result = ZExtInt16ToInt32(Load(VariableType::INT16(), PtrAdd(dataUtf16,
            PtrMul(ChangeInt32ToIntPtr(index), IntPtr(sizeof(uint16_t))))));
        Jump(&exit);
    }
    Bind(&isUtf8);
    {
        result = ZExtInt8ToInt32(Load(VariableType::INT8(), PtrAdd(dataUtf16,
            PtrMul(ChangeInt32ToIntPtr(index), IntPtr(sizeof(uint8_t))))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_BUILTINS_STUBS_INL_H