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

#include "ecmascript/pgo_profiler/ap_file/pgo_method_type_set.h"

namespace panda::ecmascript::pgo {
using StringHelper = base::StringHelper;
void PGOMethodTypeSet::Merge(const PGOMethodTypeSet *info)
{
    for (const auto &fromType : info->scalarOpTypeInfos_) {
        auto iter = scalarOpTypeInfos_.find(fromType);
        if (iter != scalarOpTypeInfos_.end()) {
            const_cast<ScalarOpTypeInfo &>(*iter).Merge(fromType);
        } else {
            scalarOpTypeInfos_.emplace(fromType);
        }
    }
    for (const auto &fromType : info->rwScalarOpTypeInfos_) {
        auto iter = rwScalarOpTypeInfos_.find(fromType);
        if (iter != rwScalarOpTypeInfos_.end()) {
            const_cast<RWScalarOpTypeInfo &>(*iter).Merge(fromType);
        } else {
            rwScalarOpTypeInfos_.emplace(fromType);
        }
    }
    for (const auto &fromType : info->objDefOpTypeInfos_) {
        AddDefine(fromType.GetOffset(), fromType.GetType());
    }
}

void PGOMethodTypeSet::SkipFromBinary(void **buffer)
{
    uint32_t size = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        base::ReadBufferInSize<ScalarOpTypeInfo>(buffer);
    }
}

bool PGOMethodTypeSet::ParseFromBinary(PGOContext& context, void** addr, size_t bufferSize)
{
    PGOProfilerHeader *const header = context.GetHeader();
    ASSERT(header != nullptr);
    void* buffer = *addr;
    uint32_t size = base::ReadBuffer<uint32_t>(addr, sizeof(uint32_t));
    if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "PGOMethodTypeSet::size")) {
        return false;
    }
    if (size > MAX_METHOD_TYPE_SIZE) {
        return false;
    }
    for (uint32_t i = 0; i < size; i++) {
        auto typeInfo = base::ReadBufferInSize<TypeInfoHeader>(addr);
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "PGOMethodTypeSet::typeInfo")) {
            return false;
        }
        if (typeInfo->GetInfoType() == InfoType::OP_TYPE) {
            auto *scalerInfo = reinterpret_cast<ScalarOpTypeInfoRef *>(typeInfo);
            scalarOpTypeInfos_.emplace(scalerInfo->GetOffset(),
                                       PGOSampleType::ConvertFrom(context, scalerInfo->GetType()));
        } else if (typeInfo->GetInfoType() == InfoType::DEFINE_CLASS_TYPE) {
            auto *defineInfo = reinterpret_cast<ObjDefOpTypeInfoRef *>(typeInfo);
            PGODefineOpType type;
            type.ConvertFrom(context, defineInfo->GetType());
            ObjDefOpTypeInfo info(defineInfo->GetOffset(), type);
            objDefOpTypeInfos_.emplace(info);
        } else if (header->SupportUseHClassType() && typeInfo->GetInfoType() == InfoType::USE_HCLASS_TYPE) {
            auto *opTypeInfo = reinterpret_cast<RWScalarOpTypeInfoRef *>(typeInfo);
            RWScalarOpTypeInfo info(opTypeInfo->GetOffset());
            info.ConvertFrom(context, *opTypeInfo);
            if (!ParseProtoChainsFromBinary(context, info, addr, buffer, bufferSize)) {
                return false;
            }
            rwScalarOpTypeInfos_.emplace(info);
        }
    }
    return true;
}

bool PGOMethodTypeSet::ParseProtoChainsFromBinary(
    PGOContext& context, RWScalarOpTypeInfo& info, void** addr, void* buffer, size_t bufferSize)
{
    for (int j = 0; j < info.GetCount(); j++) {
        if (info.GetTypeRef().GetObjectInfo(j).GetProtoChainMarker() == ProtoChainMarker::EXSIT) {
            auto protoChainRef = base::ReadBufferInSize<PGOProtoChainRef>(addr);
            if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "PGOMethodTypeSet::protoChainRef")) {
                return false;
            }
            auto protoChain = PGOProtoChain::ConvertFrom(context, protoChainRef);
            const_cast<PGOObjectInfo&>(info.GetTypeRef().GetObjectInfo(j)).SetProtoChain(protoChain);
        }
    }
    return true;
}

bool PGOMethodTypeSet::ProcessToBinary(PGOContext &context, std::stringstream &stream) const
{
    uint32_t number = 0;
    std::stringstream methodStream;
    for (auto &typeInfo : scalarOpTypeInfos_) {
        if (!typeInfo.GetType().IsNone()) {
            PGOSampleTypeRef sampleTypeRef = PGOSampleTypeRef::ConvertFrom(context, typeInfo.GetType());
            ScalarOpTypeInfoRef infoRef(typeInfo.GetOffset(), sampleTypeRef);
            methodStream.write(reinterpret_cast<char *>(&infoRef), infoRef.Size());
            number++;
        }
    }
    for (auto &typeInfo : rwScalarOpTypeInfos_) {
        if (typeInfo.GetCount() != 0) {
            RWScalarOpTypeInfoRef infoRef(typeInfo.GetOffset());
            infoRef.ConvertFrom(context, typeInfo);
            methodStream.write(reinterpret_cast<char *>(&infoRef), infoRef.Size());
            for (int i = 0; i < typeInfo.GetCount(); i++) {
                if (typeInfo.GetTypeRef().GetObjectInfo(i).GetProtoChainMarker() == ProtoChainMarker::EXSIT) {
                    auto protoChain = typeInfo.GetTypeRef().GetObjectInfo(i).GetProtoChain();
                    auto protoChainRef = PGOProtoChainRef::ConvertFrom(context, protoChain);
                    methodStream.write(reinterpret_cast<char *>(protoChainRef), protoChainRef->Size());
                    PGOProtoChainRef::DeleteProtoChain(protoChainRef);
                }
            }
            number++;
        }
    }

    for (const auto &typeInfo : objDefOpTypeInfos_) {
        PGODefineOpTypeRef typeRef;
        typeRef.ConvertFrom(context, typeInfo.GetType());
        ObjDefOpTypeInfoRef infoRef(typeInfo.GetOffset(), typeRef);
        methodStream.write(reinterpret_cast<char *>(&infoRef), infoRef.Size());
        number++;
    }

    stream.write(reinterpret_cast<char *>(&number), sizeof(uint32_t));
    if (number > 0) {
        stream << methodStream.rdbuf();
        return true;
    }
    return false;
}

void PGOMethodTypeSet::ProcessToText(TextFormatter& fmt) const
{
    bool hasOutput = false;
    fmt.SetLabelWidth(TextFormatter::LABEL_WIDTH_SMALL);

    IndentScope indentScope(fmt);
    // Output scalar op types
    for (auto typeInfoIter : scalarOpTypeInfos_) {
        if (typeInfoIter.GetType().IsNone()) {
            continue;
        }
        fmt.NewLine().AutoIndent().Label("Offset").Value(typeInfoIter.GetOffset(), true);
        fmt.Indent().Label("Type").Value(typeInfoIter.GetType().GetTypeString());
        hasOutput = true;
    }

    // Output RW scalar op types
    for (auto rwScalarOpTypeInfoIter : rwScalarOpTypeInfos_) {
        if (rwScalarOpTypeInfoIter.GetCount() == 0) {
            continue;
        }
        fmt.NewLine().AutoIndent().Label("Offset").Value(rwScalarOpTypeInfoIter.GetOffset(), true);
        fmt.Indent().Label("RWType").Value("[");
        bool isFirst = true;
        for (uint32_t i = 0; i < rwScalarOpTypeInfoIter.GetTypeRef().GetCount(); i++) {
            if (!isFirst) {
                fmt.Text(", ");
            }
            isFirst = false;
            rwScalarOpTypeInfoIter.GetTypeRef().GetObjectInfo(i).GetInfoString(fmt);
        }
        fmt.Text("]");
        hasOutput = true;
    }

    // Output object define types
    for (const auto& defTypeInfoIter: objDefOpTypeInfos_) {
        fmt.NewLine().AutoIndent().Label("Offset").Value(defTypeInfoIter.GetOffset(), true);
        fmt.Indent().Label("DefType");
        defTypeInfoIter.GetType().GetTypeString(fmt);
        hasOutput = true;
    }

    if (hasOutput) {
        fmt.NewLine();
    }
}

void PGOMethodTypeSet::ProcessToJson(ProfileType::VariantVector &typeArray) const
{
    for (auto typeInfoIter : scalarOpTypeInfos_) {
        if (typeInfoIter.GetType().IsNone()) {
            continue;
        }
        ProfileType::StringMap type;
        type.insert(std::make_pair(DumpJsonUtils::TYPE_OFFSET, std::to_string(typeInfoIter.GetOffset())));
        typeInfoIter.GetType().GetTypeJson(type);
        typeArray.push_back(type);
    }
    for (auto rwScalarOpTypeInfoIter : rwScalarOpTypeInfos_) {
        if (rwScalarOpTypeInfoIter.GetCount() == 0) {
            continue;
        }
        ProfileType::MapVector sameOffsetTypeArray;
        rwScalarOpTypeInfoIter.ProcessToJson(sameOffsetTypeArray);
        typeArray.push_back(sameOffsetTypeArray);
    }
    for (const auto &defTypeInfoIter : objDefOpTypeInfos_) {
        std::vector<ProfileType::StringMap> sameOffsetTypeArray;
        defTypeInfoIter.ProcessToJson(sameOffsetTypeArray);
        typeArray.push_back(sameOffsetTypeArray);
    }
}
} // namespace panda::ecmascript::pgo
