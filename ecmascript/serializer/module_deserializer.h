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

#ifndef ECMASCRIPT_SERIALIZER_MODULE_DESERIALIZER_H
#define ECMASCRIPT_SERIALIZER_MODULE_DESERIALIZER_H

#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/serializer/serialize_data.h"

namespace panda::ecmascript {

class ModuleDeserializer : public BaseDeserializer {
public:
    explicit ModuleDeserializer(JSThread *thread, SerializeData *data)
        : BaseDeserializer(thread, data), thread_(thread) {}

    ~ModuleDeserializer() override = default;

private:
    uintptr_t DeserializeTaggedObject(SerializedObjectSpace space) override;
    JSThread *thread_;
};
}
#endif // ECMASCRIPT_SERIALIZER_MODULE_DESERIALIZER_H