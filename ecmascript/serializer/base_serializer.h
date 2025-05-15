/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_H
#define ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_H

#include "ecmascript/mem/object_xray.h"
#include "ecmascript/serializer/serialize_chunk.h"
#include "ecmascript/serializer/serialize_data.h"

namespace panda::ecmascript {

class Ecmavm;
class JSThread;
class BaseSerializer {
public:
    explicit BaseSerializer(JSThread *thread) : thread_(thread), vm_(thread->GetEcmaVM())
    {
        data_.reset(new SerializeData(thread));
        sharedObjChunk_.reset(new SerializationChunk());
    }
    virtual ~BaseSerializer()
    {
        referenceMap_.clear();
    }
    NO_COPY_SEMANTIC(BaseSerializer);
    NO_MOVE_SEMANTIC(BaseSerializer);

    void SerializeJSTaggedValue(JSTaggedValue value);
    std::unique_ptr<SerializeData> Release()
    {
        return std::move(data_);
    }

protected:
    template <SerializeType serializeType>
    class SerializeObjectFieldVisitor final : public BaseObjectVisitor<SerializeObjectFieldVisitor<serializeType>> {
    public:
        explicit SerializeObjectFieldVisitor(BaseSerializer *serializer);
        ~SerializeObjectFieldVisitor() override = default;

        void VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr, uintptr_t endAddr,
                                  VisitObjectArea area) override;
        void VisitObjectHClassImpl(BaseObject *hclass) override;
    private:
        BaseSerializer *serializer_ {nullptr};
    };
    // Different serialize mode can implement this interface to custom processing
    virtual void SerializeObjectImpl(TaggedObject *object, bool isWeak = false) = 0;
    void WriteMultiRawData(uintptr_t beginAddr, size_t fieldSize);
    template<SerializeType serializeType>
    void SerializeTaggedObject(TaggedObject *object);
    bool SerializeReference(TaggedObject *object);
    bool SerializeRootObject(TaggedObject *object);
    void SerializeSharedObject(TaggedObject *object);
    template<SerializeType serializeType>
    void SerializeObjectField(TaggedObject *object);
    bool SerializeSpecialObjIndividually(JSType objectType, TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeHClassFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeSFunctionFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeSFunctionModule(JSFunction *func);
    void SerializeLexicalEnvFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeSFunctionEnvFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeSendableEnvFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeAsyncFunctionFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeObjectProto(JSHClass *kclass, JSTaggedValue proto);
    void SerializeTaggedObjField(SerializeType serializeType, TaggedObject *root, ObjectSlot start, ObjectSlot end);
    void SerializeInObjField(TaggedObject *object, ObjectSlot start, ObjectSlot end);
    SerializedObjectSpace GetSerializedObjectSpace(TaggedObject *object) const;

protected:
    JSThread *thread_;
    EcmaVM *vm_;
    std::unique_ptr<SerializeData> data_;
    std::unique_ptr<SerializationChunk> sharedObjChunk_;
    CUnorderedMap<TaggedObject *, uint32_t> referenceMap_;
    size_t objectIndex_ {0};
    static constexpr size_t LEXICALENV_GLOBAL_ENV_SLOT =
        LexicalEnv::DATA_OFFSET + LexicalEnv::GLOBAL_ENV_INDEX * sizeof(JSTaggedValue);
    static constexpr size_t LEXICALENV_PARENT_ENV_SLOT =
        LexicalEnv::DATA_OFFSET + LexicalEnv::PARENT_ENV_INDEX * sizeof(JSTaggedValue);
    static constexpr size_t LEXICALENV_SCOPE_INFO_SLOT =
        LexicalEnv::DATA_OFFSET + LexicalEnv::SCOPE_INFO_INDEX * sizeof(JSTaggedValue);
    static constexpr size_t SFUNCTIONENV_GLOBAL_ENV_SLOT =
        SFunctionEnv::DATA_OFFSET + SFunctionEnv::GLOBAL_ENV_INDEX * sizeof(JSTaggedValue);
    static constexpr size_t SFUNCTIONENV_CONSTRUCTOR_SLOT =
        SFunctionEnv::DATA_OFFSET + SFunctionEnv::CONSTRUCTOR_INDEX * sizeof(JSTaggedValue);
    static constexpr size_t SENDABLEENV_PARENT_ENV_SLOT =
        SendableEnv::DATA_OFFSET + SendableEnv::SENDABLE_PARENT_ENV_INDEX * sizeof(JSTaggedValue);
    static constexpr size_t SENDABLEENV_SCOPE_INFO_SLOT =
        SendableEnv::DATA_OFFSET + SendableEnv::SENDABLE_SCOPE_INFO_INDEX * sizeof(JSTaggedValue);
    int32_t serializeSharedEvent_ = 0;
};
}

#endif  // ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_H
