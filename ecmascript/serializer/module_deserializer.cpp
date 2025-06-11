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

#include "ecmascript/serializer/module_deserializer.h"

namespace panda::ecmascript {

uintptr_t ModuleDeserializer::DeserializeTaggedObject(SerializedObjectSpace space)
{
    uintptr_t res = BaseDeserializer::DeserializeTaggedObject(space);
    JSType type = reinterpret_cast<TaggedObject *>(res)->GetClass()->GetObjectType();
    // String need remove duplicates if string table can find
    if (type == JSType::LINE_STRING) {
        size_t resIndex = objectVector_.size() - 1;
        EcmaStringTable *stringTable = thread_->GetEcmaVM()->GetEcmaStringTable();
        EcmaString *str = reinterpret_cast<EcmaString *>(res);
        EcmaStringAccessor(str).ClearInternString();
        EcmaString *strInTable = stringTable->GetOrInternFlattenStringNoGC(thread_->GetEcmaVM(), str);
        res = ToUintPtr(strInTable);
        objectVector_[resIndex] = res;
    }
    return res;
}
}