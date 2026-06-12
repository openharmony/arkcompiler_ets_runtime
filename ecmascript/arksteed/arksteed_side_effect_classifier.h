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

#ifndef ECMASCRIPT_ARKSTEED_SIDE_EFFECT_CLASSIFIER_H
#define ECMASCRIPT_ARKSTEED_SIDE_EFFECT_CLASSIFIER_H

#include "ecmascript/arksteed/arksteed_compile_info_facts.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript::arksteed {

class ArkSteedSideEffectClassifier {
public:
    static SideEffectDescriptor Classify(StoreTaggedFieldVertex *vertex)
    {
        uint32_t propertyId = vertex->GetPropertyId();
        PropertyKey key = propertyId != StoreTaggedFieldVertex::UNKNOWN_PROPERTY_ID ? PropertyKey::Named(propertyId)
                                                                                    : PropertyKey::Unknown();
        SideEffectDescriptor descriptor;
        descriptor.kind = SideEffectKind::FIELD_WRITE;
        descriptor.receiver = vertex->GetInput(StoreTaggedFieldVertex::OBJECT_INDEX);
        descriptor.propertyKey = key;
        return descriptor;
    }

    static SideEffectDescriptor Classify(StoreEnvSlotVertex *vertex)
    {
        SideEffectDescriptor descriptor;
        descriptor.kind = SideEffectKind::ENV_SLOT_WRITE;
        descriptor.env = vertex->GetInput(StoreEnvSlotVertex::ENV_INDEX);
        descriptor.envSlot = vertex->GetOffset();
        descriptor.envSlotValue = vertex->GetInput(StoreEnvSlotVertex::VALUE_INDEX);
        return descriptor;
    }

    static SideEffectDescriptor Classify(CallRuntimeVertex *vertex)
    {
        return SideEffectDescriptor {vertex->GetSideEffectKind()};
    }

    static SideEffectDescriptor Classify(CallCommonStubVertex *vertex)
    {
        return SideEffectDescriptor {vertex->GetSideEffectKind()};
    }

    template <typename VertexT>
    static SideEffectDescriptor Classify(VertexT *vertex)
    {
        static_assert(VertexT::PROPERTIES.CanWrite());
        LOG_COMPILER(WARN) << "ArkSteed side-effect classifier fallback for " << OpcodeToString(vertex->GetOpcode());
        return SideEffectDescriptor {SideEffectKind::UNKNOWN_CALL};
    }
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_SIDE_EFFECT_CLASSIFIER_H
