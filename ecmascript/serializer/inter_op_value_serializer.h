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

#ifndef ECMASCRIPT_SERIALIZER_INTER_OP_VALUE_SERIALIZER_H
#define ECMASCRIPT_SERIALIZER_INTER_OP_VALUE_SERIALIZER_H

#include "ecmascript/serializer/value_serializer.h"

namespace panda::ecmascript {

class InterOpValueSerializer : public ValueSerializer {
public:
    explicit InterOpValueSerializer(JSThread *thread, bool defaultTransfer = false, bool defaultCloneShared = false)
        : ValueSerializer(thread, defaultTransfer, defaultCloneShared) {}
    ~InterOpValueSerializer() = default;
    NO_COPY_SEMANTIC(InterOpValueSerializer);
    NO_MOVE_SEMANTIC(InterOpValueSerializer);

private:
    void SerializeObjectImpl(TaggedObject *object, bool isWeak = false) override;
    bool TrySerializeInterOpObject(TaggedObject *object, bool isWeak);
    void SerializeInterOpObjectImpl(TaggedObject *object, JSObject *xRefObject, bool isWeak);
};
}

#endif  // ECMASCRIPT_SERIALIZER_INTER_OP_VALUE_SERIALIZER_H