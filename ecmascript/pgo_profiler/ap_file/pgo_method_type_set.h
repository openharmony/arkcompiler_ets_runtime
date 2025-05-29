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

#ifndef ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_METHOD_TYPE_SET_H
#define ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_METHOD_TYPE_SET_H

#include <set>

#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"

namespace panda::ecmascript::pgo {
class PGOMethodTypeSet {
public:
    static constexpr int METHOD_TYPE_INFO_INDEX = 4;
    static constexpr int METHOD_TYPE_INFO_COUNT = 2;
    static constexpr int METHOD_OFFSET_INDEX = 0;
    static constexpr int METHOD_TYPE_INDEX = 1;
    static constexpr uint32_t MAX_METHOD_TYPE_SIZE = 512 * 1024 * 1024;

    PGOMethodTypeSet() = default;
    void Clear()
    {
        scalarOpTypeInfos_.clear();
        rwScalarOpTypeInfos_.clear();
        objDefOpTypeInfos_.clear();
    }

    void AddType(uint32_t offset, PGOSampleType type)
    {
        auto result = scalarOpTypeInfos_.find(ScalarOpTypeInfo(offset, type));
        if (result != scalarOpTypeInfos_.end()) {
            auto combineType = result->GetType().CombineType(type);
            const_cast<ScalarOpTypeInfo &>(*result).SetType(combineType);
        } else {
            scalarOpTypeInfos_.emplace(offset, type);
        }
    }

    void AddCallTargetType(uint32_t offset, PGOSampleType type)
    {
        auto result = scalarOpTypeInfos_.find(ScalarOpTypeInfo(offset, type));
        if (result != scalarOpTypeInfos_.end()) {
            auto combineType = result->GetType().CombineCallTargetType(type);
            const_cast<ScalarOpTypeInfo &>(*result).SetType(combineType);
        } else {
            scalarOpTypeInfos_.emplace(offset, type);
        }
    }

    void AddObjectInfo(uint32_t offset, const PGOObjectInfo &info)
    {
        auto result = rwScalarOpTypeInfos_.find(RWScalarOpTypeInfo(offset));
        if (result != rwScalarOpTypeInfos_.end()) {
            const_cast<RWScalarOpTypeInfo &>(*result).AddObjectInfo(info);
        } else {
            rwScalarOpTypeInfos_.emplace(offset, info);
        }
    }

    void AddDefine(uint32_t offset, PGODefineOpType type)
    {
        auto result = objDefOpTypeInfos_.find(ObjDefOpTypeInfo(offset, type));
        if (result != objDefOpTypeInfos_.end()) {
            if (type.GetProfileType().IsArrayLiteralType())  {
                const_cast<PGODefineOpTemplate<ProfileType> *>(&((*result).GetTypeRef()))
                    ->SetElementsKind(type.GetElementsKind());
                const_cast<PGODefineOpTemplate<ProfileType> *>(&((*result).GetTypeRef()))
                    ->SetSpaceFlag(type.GetSpaceFlag());
            }
            return;
        }
        objDefOpTypeInfos_.emplace(offset, type);
    }

    template <typename Callback>
    void GetTypeInfo(Callback callback)
    {
        for (const auto &typeInfo : scalarOpTypeInfos_) {
            const auto &type = typeInfo.GetTypeRef();
            callback(typeInfo.GetOffset(), &type);
        }
        for (const auto &typeInfo : rwScalarOpTypeInfos_) {
            const auto &type = typeInfo.GetTypeRef();
            callback(typeInfo.GetOffset(), &type);
        }
        for (const auto &typeInfo : objDefOpTypeInfos_) {
            const auto &type = typeInfo.GetTypeRef();
            callback(typeInfo.GetOffset(), &type);
        }
    }

    void Merge(const PGOMethodTypeSet *info);
    static void SkipFromBinary(void **buffer);

    bool ParseFromBinary(PGOContext& context, void** addr, size_t bufferSize);
    bool ProcessToBinary(PGOContext &context, std::stringstream &stream) const;

    bool ParseFromText(const std::string &typeString);
    void ProcessToText(std::string &text) const;

    void ProcessToJson(ProfileType::VariantVector &typeArray) const;

    NO_COPY_SEMANTIC(PGOMethodTypeSet);
    NO_MOVE_SEMANTIC(PGOMethodTypeSet);

private:
    enum class InfoType : uint8_t {
        NONE,
        OP_TYPE,
        DEFINE_CLASS_TYPE = 3,
        USE_HCLASS_TYPE
    };

    class TypeInfoHeader {
    public:
        TypeInfoHeader(InfoType type, uint32_t offset) : infoType_(type), offset_(offset) {}
        TypeInfoHeader(uint32_t size, InfoType type, uint32_t offset)
            : size_(size), infoType_(type), offset_(offset) {}

        InfoType GetInfoType()
        {
            return infoType_;
        }

        int32_t Size() const
        {
            return size_;
        }

        uint32_t GetOffset() const
        {
            return offset_;
        }

    protected:
        uint32_t size_ {0};
        InfoType infoType_ {InfoType::NONE};
        uint32_t offset_ {0};
    };

    template <typename RWOpType, typename ObjectInfoType>
    class RWScalarOpTemplate : public TypeInfoHeader {
    public:
        explicit RWScalarOpTemplate(uint32_t offset) : TypeInfoHeader(InfoType::USE_HCLASS_TYPE, offset) {};
        RWScalarOpTemplate(uint32_t offset, ObjectInfoType info)
            : TypeInfoHeader(sizeof(RWScalarOpTemplate), InfoType::USE_HCLASS_TYPE, offset)
        {
            type_.AddObjectInfo(info);
        }

        template <typename FromType>
        void ConvertFrom(PGOContext &context, const FromType &from)
        {
            size_ = sizeof(RWScalarOpTemplate);
            type_.ConvertFrom(context, from.GetTypeRef());
        }

        bool operator<(const RWScalarOpTemplate &right) const
        {
            return offset_ < right.offset_;
        }

        int32_t GetCount() const
        {
            return type_.GetCount();
        }

        void Merge(const RWScalarOpTemplate &type)
        {
            type_.Merge(type.type_);
        }

        void AddObjectInfo(const ObjectInfoType &info)
        {
            type_.AddObjectInfo(info);
        }

        const RWOpType &GetTypeRef() const
        {
            return type_;
        }

        void ProcessToText(std::string &text) const
        {
            text += std::to_string(GetOffset());
            text += DumpUtils::BLOCK_START;
            text += DumpUtils::ARRAY_START + DumpUtils::NEW_LINE;
            bool isFirst = true;
            for (uint32_t i = 0; i < type_.GetCount(); i++) {
                if (!isFirst) {
                    text += DumpUtils::SPACE + DumpUtils::BLOCK_SEPARATOR + DumpUtils::NEW_LINE;
                }
                isFirst = false;
                text += type_.GetObjectInfo(i).GetInfoString();
            }
            text += DumpUtils::NEW_LINE + DumpUtils::ALIGN + DumpUtils::ALIGN + DumpUtils::ARRAY_END;
        }

        void ProcessToJson(ProfileType::MapVector &typeArray) const
        {
            for (uint32_t i = 0; i < type_.GetCount(); i++) {
                std::vector<ProfileType::StringMap> sameOffsetTypeArray;
                type_.GetObjectInfo(i).GetInfoJson(sameOffsetTypeArray, std::to_string(GetOffset()));
                typeArray.push_back(sameOffsetTypeArray);
            }
        }

    private:
        RWOpType type_;
    };
    using RWScalarOpTypeInfo = RWScalarOpTemplate<PGORWOpType, PGOObjectInfo>;
    using RWScalarOpTypeInfoRef = RWScalarOpTemplate<PGORWOpTypeRef, PGOObjectInfoRef>;

    template <typename SampleType>
    class ScalarOpTemplate : public TypeInfoHeader {
    public:
        ScalarOpTemplate(uint32_t offset, SampleType type)
            : TypeInfoHeader(sizeof(ScalarOpTemplate), InfoType::OP_TYPE, offset), type_(type) {}

        bool operator<(const ScalarOpTemplate &right) const
        {
            return offset_ < right.offset_;
        }

        void SetType(SampleType type)
        {
            if (type_ != type) {
                type_ = type;
            }
        }

        void Merge(const ScalarOpTemplate &typeInfo)
        {
            SampleType combineType = GetType().CombineType(typeInfo.GetType());
            SetType(combineType);
        }

        SampleType GetType() const
        {
            return type_;
        }

        const SampleType &GetTypeRef() const
        {
            return type_;
        }

    protected:
        ScalarOpTemplate(uint32_t size, InfoType infoType, uint32_t offset, SampleType type)
            : TypeInfoHeader(size, infoType, offset), type_(type) {}

    private:
        SampleType type_;
    };
    using ScalarOpTypeInfo = ScalarOpTemplate<PGOSampleType>;
    using ScalarOpTypeInfoRef = ScalarOpTemplate<PGOSampleTypeRef>;

    template <typename PGODefineOpType>
    class ObjDefOpTemplate : public TypeInfoHeader {
    public:
        ObjDefOpTemplate(uint32_t offset, PGODefineOpType type)
            : TypeInfoHeader(sizeof(ObjDefOpTemplate), InfoType::DEFINE_CLASS_TYPE, offset), type_(type) {}

        bool operator<(const ObjDefOpTemplate &right) const
        {
            if (this->offset_ != right.GetOffset()) {
                return this->offset_ < right.GetOffset();
            }
            return this->GetType() < right.GetType();
        }

        void ProcessToText(std::string &text) const
        {
            text += std::to_string(this->GetOffset());
            text += DumpUtils::BLOCK_START;
            text += DumpUtils::ARRAY_START + DumpUtils::NEW_LINE;
            text += this->GetType().GetTypeString();
            text += DumpUtils::NEW_LINE + DumpUtils::ALIGN + DumpUtils::ALIGN + DumpUtils::ARRAY_END;
        }

        void ProcessToJson(std::vector<ProfileType::StringMap> &sameOffsetTypeArray) const
        {
            this->GetType().GetTypeJson(sameOffsetTypeArray, std::to_string(this->GetOffset()));
        }

        PGODefineOpType GetType() const
        {
            return type_;
        }

        const PGODefineOpType &GetTypeRef() const
        {
            return type_;
        }

    protected:
        ObjDefOpTemplate(
            uint32_t size, InfoType infoType, uint32_t offset, PGODefineOpType type)
            : TypeInfoHeader(size, infoType, offset), type_(type) {}
    private:
        PGODefineOpType type_;
    };

    bool ParseProtoChainsFromBinary(
        PGOContext& context, RWScalarOpTypeInfo& info, void** addr, void* buffer, size_t bufferSize);

    using ObjDefOpTypeInfo = ObjDefOpTemplate<PGODefineOpType>;
    using ObjDefOpTypeInfoRef = ObjDefOpTemplate<PGODefineOpTypeRef>;

    std::set<ScalarOpTypeInfo> scalarOpTypeInfos_;
    std::set<RWScalarOpTypeInfo> rwScalarOpTypeInfos_;
    std::set<ObjDefOpTypeInfo> objDefOpTypeInfos_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_METHOD_TYPE_SET_H
