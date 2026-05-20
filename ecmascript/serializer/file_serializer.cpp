/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "file_serializer.h"

namespace panda::ecmascript {
bool FileSerializer::CheckObjectCanSerialize(TaggedObject* object, bool& findSharedObject)
{
    if (valueFilter_) {
        switch (valueFilter_(object)) {
            case Policy::ALLOW:
                return true;
            case Policy::DENY:
                return false;
            case Policy::INHERIT:
            default:
                break;
        }
    }
    return ValueSerializer::CheckObjectCanSerialize(object, findSharedObject);
}

bool FileSerializer::SerializeSharedObj([[maybe_unused]] TaggedObject* object)
{
    // FileSerializer not support serialize SharedObject, this data will load by different process.
    return false;
}

FileSerializer::Policy FileSerializer::SourceTextModuleFilter(TaggedObject* object)
{
    JSType type = object->GetClass()->GetObjectType();
    if (type >= JSType::JS_RECORD_FIRST && type <= JSType::JS_RECORD_LAST) {
        return Policy::ALLOW;
    }
    return Policy::INHERIT;
}
} // namespace panda::ecmascript
