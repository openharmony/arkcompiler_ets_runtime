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

#ifndef ECMASCRIPT_ARKSTEED_WRITE_BARRIER_H
#define ECMASCRIPT_ARKSTEED_WRITE_BARRIER_H

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_deferred_code.h"
#include "ecmascript/arksteed/arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

class ArkSteedWriteBarrierEmitter {
public:
    ArkSteedWriteBarrierEmitter(ArkSteedAssembler *assembler, Chunk *chunk,
                                ArkSteedDeferredCodeList *deferredCode,
                                const DeferredRegisterSnapshot &registerSnapshot)
        : assembler_(assembler),
          chunk_(chunk),
          deferredCode_(deferredCode),
          registerSnapshot_(registerSnapshot)
    {}

    void StoreTaggedField(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value, int32_t offset,
                          ArkSteedWriteBarrierKind barrierKind, ArkSteedRegister objectRegionScratch,
                          ArkSteedRegister valueRegionScratch,
                          ArkSteedWriteBarrierValueKind valueKind = ArkSteedWriteBarrierValueKind::Unknown);
    void TransitionHClass(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister hclass,
                          ArkSteedRegister objectRegionScratch, ArkSteedRegister hclassRegionScratch);

private:
    void EmitFastWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value,
                              int32_t offset);
    void EmitWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value, int32_t offset,
                          ArkSteedWriteBarrierKind barrierKind, ArkSteedRegister objectRegionScratch,
                          ArkSteedRegister valueRegionScratch);
    void EmitLocalToShareRSet(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value, int32_t offset,
                              ArkSteedRegister objectRegionScratch, ArkSteedRegister bitsetWordAddrScratch,
                              Label *next);
    void EmitPostStoreWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value,
                                   int32_t offset, ArkSteedWriteBarrierKind barrierKind,
                                   ArkSteedRegister objectRegionScratch, ArkSteedRegister valueRegionScratch);
    void CallBarrierRuntime(kungfu::RuntimeStubCSigns::ID runtimeId, ArkSteedRegister glue,
                            ArkSteedRegister object, int32_t offset, ArkSteedRegister value,
                            bool preserveInputs);

    ArkSteedAssembler *assembler_ {nullptr};
    Chunk *chunk_ {nullptr};
    ArkSteedDeferredCodeList *deferredCode_ {nullptr};
    DeferredRegisterSnapshot registerSnapshot_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_WRITE_BARRIER_H
