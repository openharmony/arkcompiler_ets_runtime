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

bool PGOMethodTypeSet::ParseFromBinary(PGOContext &context, void **buffer)
{
    PGOProfilerHeader *const header = context.GetHeader();
    ASSERT(header != nullptr);
    uint32_t size = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
    if (size > MAX_METHOD_TYPE_SIZE) {
        return false;
    }
    for (uint32_t i = 0; i < size; i++) {
        auto typeInfo = base::ReadBufferInSize<TypeInfoHeader>(buffer);
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
            for (int j = 0; j < info.GetCount(); j++) {
                if (info.GetTypeRef().GetObjectInfo(j).GetProtoChainMarker() == ProtoChainMarker::EXSIT) {
                    auto protoChainRef = base::ReadBufferInSize<PGOProtoChainRef>(buffer);
                    auto protoChain = PGOProtoChain::ConvertFrom(context, protoChainRef);
                    const_cast<PGOObjectInfo &>(info.GetTypeRef().GetObjectInfo(j)).SetProtoChain(protoChain);
                }
            }
            rwScalarOpTypeInfos_.emplace(info);
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

bool PGOMethodTypeSet::ParseFromText(const std::string &typeString)
{
    std::vector<std::string> typeInfoVector = StringHelper::SplitString(typeString, DumpUtils::TYPE_SEPARATOR);
    if (typeInfoVector.size() > 0) {
        for (const auto &iter : typeInfoVector) {
            std::vector<std::string> typeStrings = StringHelper::SplitString(iter, DumpUtils::BLOCK_START);
            if (typeStrings.size() < METHOD_TYPE_INFO_COUNT) {
                return false;
            }

            uint32_t offset = 0;
            if (!StringHelper::StrToUInt32(typeStrings[METHOD_OFFSET_INDEX].c_str(), &offset)) {
                return false;
            }
            uint32_t type = 0;
            if (!StringHelper::StrToUInt32(typeStrings[METHOD_TYPE_INDEX].c_str(), &type)) {
                return false;
            }
            scalarOpTypeInfos_.emplace(offset, PGOSampleType(type));
        }
    }
    return true;
}

void PGOMethodTypeSet::ProcessToText(std::string &text) const
{
    bool isFirst = true;
    for (auto typeInfoIter : scalarOpTypeInfos_) {
        if (typeInfoIter.GetType().IsNone()) {
            continue;
        }
        if (isFirst) {
            text += DumpUtils::ARRAY_START + DumpUtils::SPACE;
            isFirst = false;
        } else {
            text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
        }
        text += std::to_string(typeInfoIter.GetOffset());
        text += DumpUtils::BLOCK_START;
        text += typeInfoIter.GetType().GetTypeString();
    }
    for (auto rwScalarOpTypeInfoIter : rwScalarOpTypeInfos_) {
        if (rwScalarOpTypeInfoIter.GetCount() == 0) {
            continue;
        }
        if (isFirst) {
            text += DumpUtils::ARRAY_START + DumpUtils::SPACE;
            isFirst = false;
        } else {
            text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
        }
        rwScalarOpTypeInfoIter.ProcessToText(text);
    }
    for (const auto &defTypeInfoIter : objDefOpTypeInfos_) {
        if (isFirst) {
            text += DumpUtils::ARRAY_START + DumpUtils::SPACE;
            isFirst = false;
        } else {
            text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
        }
        defTypeInfoIter.ProcessToText(text);
    }
    if (!isFirst) {
        text += (DumpUtils::SPACE + DumpUtils::ARRAY_END);
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
