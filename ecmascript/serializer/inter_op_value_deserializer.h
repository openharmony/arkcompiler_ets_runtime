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

#ifndef ECMASCRIPT_SERIALIZER_INTER_OP_VALUE_DESERIALIZER_H
#define ECMASCRIPT_SERIALIZER_INTER_OP_VALUE_DESERIALIZER_H

#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/serializer/serialize_data.h"

namespace panda::ecmascript {
class Heap;
class JSThread;
struct XRefBindingAttachInfo {
    AttachXRefFunc af_ {nullptr};
    void *attachXRefData_ = {nullptr};
    JSHandle<JSTaggedValue> obj_;
    size_t offset_ {0U};

    XRefBindingAttachInfo(AttachXRefFunc af, void *attachXRefData, JSHandle<JSTaggedValue> obj, size_t offset)
        : af_(af), attachXRefData_(attachXRefData), obj_(obj), offset_(offset) {}

    uintptr_t GetObjAddr() const
    {
        return static_cast<uintptr_t>(obj_->GetRawData());
    }

    size_t GetFieldOffset() const
    {
        return offset_;
    }

    ObjectSlot GetSlot() const
    {
        return ObjectSlot(GetObjAddr() + offset_);
    }
};

class InterOpValueDeserializer : public BaseDeserializer {
public:
    explicit InterOpValueDeserializer(JSThread *thread, SerializeData *data, void *hint = nullptr)
        : BaseDeserializer(thread, data, hint) {}

    ~InterOpValueDeserializer() = default;

    NO_COPY_SEMANTIC(InterOpValueDeserializer);
    NO_MOVE_SEMANTIC(InterOpValueDeserializer);

private:
    size_t DerivedExtraReadSingleEncodeData(uint8_t encodeFlag, uintptr_t objAddr, size_t fieldOffset) override;
    void DeserializeSpecialRecordedObjects() override;
    void DeserializeXRefBindingObject(XRefBindingAttachInfo *info);
    std::vector<XRefBindingAttachInfo> xRefBindingAttachInfos_;
};
}

#endif  // ECMASCRIPT_SERIALIZER_INTER_OP_VALUE_DESERIALIZER_H