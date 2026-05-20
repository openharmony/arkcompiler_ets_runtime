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
#ifndef ECMASCRIPT_SERIALIZER_FILE_SERIALIZER_H
#define ECMASCRIPT_SERIALIZER_FILE_SERIALIZER_H

#include "value_serializer.h"

namespace panda::ecmascript {

class FileSerializer : public ValueSerializer {
public:
    enum class Policy : uint8_t {
        INHERIT,
        ALLOW,
        DENY,
    };
    using ValueFilter = std::function<Policy(TaggedObject*)>;
    explicit FileSerializer(JSThread* thread, ValueFilter filter = nullptr)
        : ValueSerializer(thread), valueFilter_(filter)
    {
    }
    ~FileSerializer() override = default;
    static Policy SourceTextModuleFilter(TaggedObject* object);

private:
    bool CheckObjectCanSerialize(TaggedObject* object, bool& findSharedObject) override;
    bool SerializeSharedObj([[maybe_unused]] TaggedObject* object) override;

    size_t GetMaxJSSerializerSize(EcmaVM* vm) override
    {
        constexpr const int SERIALIZE_SIZE_MULTIPLE = 2;
        return vm->GetEcmaParamConfiguration().GetMaxJSSerializerSize() * SERIALIZE_SIZE_MULTIPLE;
    }

    ValueFilter valueFilter_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_SERIALIZER_FILE_SERIALIZER_H
