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

#include "ecmascript/compiler/hash_stub_builder.h"

namespace panda::ecmascript::kungfu {

GateRef HashStubBuilder::GetHash(GateRef key)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    Label exit(env);
    env->SubCfgEntry(&entryLabel);
    DEFVARIABLE(res, VariableType::INT32(), Int32(0));
    Label intKey(env);
    Label symbolCheck(env);
    BRANCH(TaggedIsInt(key), &intKey, &symbolCheck);
    Bind(&intKey);
    res = TaggedGetInt(key);
    Jump(&exit);
    Bind(&symbolCheck);
    Label slowGetHash(env);
    Label symbolKey(env);
    Label stringCheck(env);
    BRANCH(TaggedIsSymbol(glue_, key), &symbolKey, &stringCheck);

    Bind(&symbolKey);
    res = LoadPrimitive(VariableType::INT32(), key, IntPtr(JSSymbol::HASHFIELD_OFFSET));
    Jump(&exit);
    Bind(&stringCheck);
    Label stringKey(env);
    Label objectCheck(env);
    BRANCH(TaggedIsString(glue_, key), &stringKey, &objectCheck);
    Bind(&stringKey);
    res = GetHashcodeFromString(glue_, key);
    Jump(&exit);

    Bind(&objectCheck);
    Label heapObjectKey(env);
    Label numberCheck(env);
    BRANCH(TaggedIsHeapObject(key), &heapObjectKey, &numberCheck);

    Bind(&heapObjectKey);
    Label ecmaObjectKey(env);
    BRANCH(TaggedObjectIsEcmaObject(glue_, key), &ecmaObjectKey, &slowGetHash);
    Bind(&ecmaObjectKey);
    CalcHashcodeForObject(glue_, key, &res, &exit);

    Bind(&numberCheck);
    Label numberKey(env);
    BRANCH(TaggedIsNumber(key), &numberKey, &slowGetHash);

    Bind(&numberKey);
    CalcHashcodeForNumber(key, &res, &exit);

    Bind(&slowGetHash);
    res = GetInt32OfTInt(CallRuntime(glue_, RTSTUB_ID(GetLinkedHash), { key }));
    Jump(&exit);

    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

void HashStubBuilder::CalcHashcodeForNumber(GateRef key, Variable *res, Label *exit)
{
    auto env = GetEnvironment();
    Label doubleKey(env);
    Label intKey(env);
    BRANCH(TaggedIsDouble(key), &doubleKey, &intKey);
    Bind(&doubleKey);
    {
        CalcHashcodeForDouble(key, res, exit);
    }
    Bind(&intKey);
    {
        *res = CalcHashcodeForInt(key);
        Jump(exit);
    }
}
}  // namespace panda::ecmascript::kungfu
