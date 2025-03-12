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

#include "ecmascript/compiler/pgo_type/pgo_type_parser.h"

namespace panda::ecmascript::kungfu {
using ProtoTransType = PGOTypeManager::ProtoTransType;
void BaseParser::Parse(const BytecodeInfoCollector &collector, const PGOTypeRecorder &typeRecorder,
    const PGOHClassGenerator &generator, uint32_t methodOffset)
{
    const JSPandaFile *jsPandaFile = collector.GetJSPandaFile();
    const PGOBCInfo *bcInfo = collector.GetPGOBCInfo();

    bcInfo->IterateInfoByType(methodOffset, type_,
        [this, jsPandaFile, &typeRecorder, &generator, methodOffset](
        const uint32_t bcIdx, const uint32_t bcOffset, const uint32_t) {
        auto defType = typeRecorder.GetPGODefOpType(bcOffset);
        if (defType.IsNone()) {
            return;
        }

        PGOTypeLocation loc(jsPandaFile, methodOffset, bcIdx);
        if (!RecordTypeInfo(defType, loc)) {
            return;
        }

        GenerateHClass(generator, loc);
    });
}

bool ClassParser::RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc)
{
    auto rootType = defType.GetProfileType();
    auto ctorPt = defType.GetCtorPt();
    if (ctorPt.IsNone()) {
        return false;
    }
    PGOSampleType ctorSampleType(ctorPt);

    auto protoPt = defType.GetPrototypePt();
    if (protoPt.IsNone()) {
        return false;
    }
    PGOSampleType protoSampleType(protoPt);

    ptManager_->RecordLocationToRootType(loc, rootType);
    auto ctorLoc = loc.ChangeType(PGOTypeLocation::Type::CONSTRUCTOR);
    ptManager_->RecordLocationToRootType(ctorLoc, ctorPt);

    auto protoLoc = loc.ChangeType(PGOTypeLocation::Type::PROTOTYPE);
    ptManager_->RecordLocationToRootType(protoLoc, protoPt);
    return true;
}

void ClassParser::GenerateHClass(const PGOHClassGenerator &generator, const PGOTypeLocation &loc)
{
    auto thread = ptManager_->GetJSThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    auto rootType = ptManager_->GetRootIdByLocation(loc);
    PGOSampleType iSampleType(rootType);

    auto ctorLoc = loc.ChangeType(PGOTypeLocation::Type::CONSTRUCTOR);
    auto ctorPt = ptManager_->GetRootIdByLocation(ctorLoc);
    PGOSampleType ctorSampleType(ctorPt);

    auto protoLoc = loc.ChangeType(PGOTypeLocation::Type::PROTOTYPE);
    auto protoPt = ptManager_->GetRootIdByLocation(protoLoc);
    PGOSampleType protoSampleType(protoPt);

    // testcase: propertyaccessor2.ts. protoSampleType not find desc
    if (generator.FindHClassLayoutDesc(ctorSampleType) && generator.FindHClassLayoutDesc(protoSampleType)) {
        generator.GenerateHClass(ctorSampleType);
        generator.GenerateHClass(protoSampleType);

        auto phValue = ptManager_->QueryHClass(protoPt, protoPt);
        JSHandle<JSHClass> phclass(thread, phValue);
        JSHandle<JSObject> prototype = thread->GetEcmaVM()->GetFactory()->NewJSObjectWithInit(phclass);
        generator.GenerateIHClass(iSampleType, JSHandle<JSTaggedValue>::Cast(prototype));
    }
}

bool ArrayParser::RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc)
{
    auto rootType = defType.GetProfileType();
    if (!rootType.IsArrayLiteralType()) {
        return false;
    }

    ElementsKind kind = defType.GetElementsKind();
    ptManager_->RecordLocationToElementsKind(loc, kind);

    auto traceId = rootType.GetId();
    // For PGO, we do not care whether an array isPrototype or not.
    // This type is used at define point, we can use initial array hclass without IsPrototype bit set.
    auto hclassIdx = ptManager_->GetJSThread()->GetArrayInstanceHClassIndex(kind, false);
    ptManager_->RecordConstantIndex(traceId, static_cast<uint32_t>(hclassIdx));
    return true;
}

bool ObjectLiteralParser::RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc)
{
    auto rootType = defType.GetProfileType();
    ptManager_->RecordLocationToRootType(loc, rootType);
    return true;
}

void ObjectLiteralParser::GenerateHClass(const PGOHClassGenerator &generator, const PGOTypeLocation &loc)
{
    auto thread = ptManager_->GetJSThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    auto rootType = ptManager_->GetRootIdByLocation(loc);
    PGOSampleType rootSampleType(rootType);
    generator.SetStatus(PGOHClassGenerator::Status::ISCACHE);
    generator.GenerateHClass(rootSampleType);
}

bool FunctionParser::RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc)
{
    auto rootType = defType.GetProfileType();
    auto ctorPt = defType.GetCtorPt();
    if (ctorPt.IsNone()) {
        return false;
    }
    PGOSampleType ctorSampleType(ctorPt);

    auto protoPt = defType.GetPrototypePt();
    if (protoPt.IsNone()) {
        return false;
    }
    PGOSampleType protoSampleType(protoPt);

    ptManager_->RecordLocationToRootType(loc, rootType);
    auto ctorLoc = loc.ChangeType(PGOTypeLocation::Type::CONSTRUCTOR);
    ptManager_->RecordLocationToRootType(ctorLoc, ctorPt);

    auto protoLoc = loc.ChangeType(PGOTypeLocation::Type::PROTOTYPE);
    ptManager_->RecordLocationToRootType(protoLoc, protoPt);
    return true;
}

void FunctionParser::GenerateHClass(const PGOHClassGenerator &generator, const PGOTypeLocation &loc)
{
    auto thread = ptManager_->GetJSThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    auto rootType = ptManager_->GetRootIdByLocation(loc);
    PGOSampleType iSampleType(rootType);

    auto ctorLoc = loc.ChangeType(PGOTypeLocation::Type::CONSTRUCTOR);
    auto ctorPt = ptManager_->GetRootIdByLocation(ctorLoc);
    PGOSampleType ctorSampleType(ctorPt);

    auto protoLoc = loc.ChangeType(PGOTypeLocation::Type::PROTOTYPE);
    auto protoPt = ptManager_->GetRootIdByLocation(protoLoc);
    PGOSampleType protoSampleType(protoPt);

    // testcase: propertyaccessor2.ts. protoSampleType not find desc
    if (generator.FindHClassLayoutDesc(ctorSampleType) && generator.FindHClassLayoutDesc(protoSampleType)) {
        generator.GenerateHClass(protoSampleType);

        auto phValue = ptManager_->QueryHClass(protoPt, protoPt);
        JSHandle<JSHClass> phclass(thread, phValue);
        JSHandle<JSObject> prototype = thread->GetEcmaVM()->GetFactory()->NewJSObjectWithInit(phclass);
        generator.GenerateIHClass(iSampleType, JSHandle<JSTaggedValue>::Cast(prototype));
    }
}

PGOTypeParser::PGOTypeParser(const PGOProfilerDecoder &decoder, PGOTypeManager *ptManager)
    : decoder_(decoder), ptManager_(ptManager)
{
    parsers_.emplace_back(std::make_unique<EmptyArrayParser>(ptManager));
    parsers_.emplace_back(std::make_unique<ArrayLiteralParser>(ptManager));
    parsers_.emplace_back(std::make_unique<ObjectLiteralParser>(ptManager));
}

bool PGOTypeParser::SkipGenerateHClass(PGOTypeRecorder typeRecorder, ProfileType rootType,
                                       bool isCache, PGOHClassTreeDesc *desc)
{
    // If the definition point is not includes in the AOT list. This hclass needs to be discarded.
    // otherwise, the using point must to be deopt.
    if ((!rootType.IsNapiType() && !isCache && !typeRecorder.IsValidPt(rootType)) ||
         desc->CheckHasInvalidType()) {
        return true;
    }
    return false;
}

void PGOTypeParser::CreatePGOType(BytecodeInfoCollector &collector)
{
    const JSPandaFile *jsPandaFile = collector.GetJSPandaFile();
    PGOTypeRecorder typeRecorder(decoder_);
    typeRecorder.IterateProtoTransitionPool([this](PGOProtoTransitionType type) {
        ProfileType ihcType = type.GetIhcType();
        std::pair<ProfileType, ProfileType> baseType = type.GetBaseType();
        ProfileType transIhcType = type.GetTransitionType();
        ProfileType transPhcType = type.GetTransitionProtoPt();
        ProtoTransType transType(ihcType, baseType.first, baseType.second, transIhcType, transPhcType);
        ptManager_->RecordProtoTransType(transType);
    });

    typeRecorder.IterateHClassTreeDesc([this, typeRecorder](PGOHClassTreeDesc *desc) {
        auto rootType = desc->GetProfileType();
        auto protoPt = desc->GetProtoPt();
        bool isCache = rootType.IsObjectLiteralType();
        if (SkipGenerateHClass(typeRecorder, rootType, isCache, desc)) {
            return ;
        }
        PGOHClassGenerator::Status status = PGOHClassGenerator::Status::NONE;
        if (isCache) {
            status = PGOHClassGenerator::Status::ISCACHE;
        }
        const PGOHClassGenerator generator(typeRecorder, ptManager_, status);
        if (rootType.IsNapiType()) {
            this->GenerateHClassForNapiType(rootType, generator);
        } else if (rootType.IsGeneralizedClassType()) {
            this->GenerateHClassForClassType(rootType, protoPt, generator);
        } else if (rootType.IsPrototype()) {
            this->GenerateHClassForPrototype(rootType, generator);
        } else {
            generator.GenerateHClass(PGOSampleType(rootType));
        }
    });

    collector.IterateAllMethods([this, jsPandaFile, &collector](uint32_t methodOffset) {
        PGOTypeRecorder typeRecorder(decoder_, jsPandaFile, methodOffset);
        const PGOHClassGenerator generator(typeRecorder, ptManager_);
        for (auto &parser : parsers_) {
            parser->Parse(collector, typeRecorder, generator, methodOffset);
        }
    });
}

void PGOTypeParser::GenerateHClassForNapiType(ProfileType rootType, const PGOHClassGenerator &generator)
{
    auto thread = ptManager_->GetJSThread();
    JSHandle<JSTaggedValue> nullhandle(thread, JSTaggedValue::Null());
    PGOSampleType rootSampleType(rootType);
    generator.GenerateIHClass(rootSampleType, nullhandle);
}

void PGOTypeParser::GenerateHClassForClassType(ProfileType rootType, ProfileType protoPt,
                                               const PGOHClassGenerator &generator)
{
    if (!protoPt.IsGeneralizedPrototype()) {
        return;
    }
    auto phValue = ptManager_->QueryHClass(protoPt, protoPt);
    if (phValue.IsUndefined()) {
        generator.GenerateHClass(PGOSampleType(protoPt));
    }
    phValue = ptManager_->QueryHClass(protoPt, protoPt);
    if (phValue.IsUndefined()) {
        return;
    }
    ASSERT(phValue.IsJSHClass());
    auto thread = ptManager_->GetJSThread();
    JSHandle<JSHClass> phclass(thread, phValue);
    JSHandle<JSObject> prototype = thread->GetEcmaVM()->GetFactory()->NewJSObjectWithInit(phclass);
    PGOSampleType rootSampleType(rootType);
    generator.GenerateIHClass(rootSampleType, JSHandle<JSTaggedValue>::Cast(prototype));
}

void PGOTypeParser::GenerateHClassForPrototype(ProfileType rootType, const PGOHClassGenerator &generator)
{
    PGOSampleType rootSampleType(rootType);
    // When the collected object only has phc, use phc to create a prototype and store it in the IHC field.
    generator.GenerateHClass(rootSampleType);
    auto classType = ProfileType(rootType.GetRaw());
    classType.UpdateKind(ProfileType::Kind::ClassId);
    auto ihc = ptManager_->QueryHClass(classType, classType);
    if (ihc.IsUndefined()) {
        auto phc = ptManager_->QueryHClass(rootType, rootType);
        auto thread = ptManager_->GetJSThread();
        JSHandle<JSHClass> phclass(thread, phc);
        if (!phc.IsUndefined()) {
            JSHandle<JSObject> prototype = thread->GetEcmaVM()->GetFactory()->NewJSObjectWithInit(phclass);
            ptManager_->RecordHClass(classType, classType, prototype.GetTaggedType());
        }
    }
}
}  // namespace panda::ecmascript
