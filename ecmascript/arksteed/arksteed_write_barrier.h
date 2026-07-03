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
#include "ecmascript/arksteed/arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

class ArkSteedWriteBarrierEmitter {
public:
    explicit ArkSteedWriteBarrierEmitter(ArkSteedAssembler *assembler)
        : assembler_(assembler)
    {}

    void StoreTaggedField(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value, int32_t offset,
                          ArkSteedWriteBarrierKind barrierKind, ArkSteedRegister primaryScratch,
                          ArkSteedRegister offsetScratch, ArkSteedRegister secondaryScratch,
                          ArkSteedWriteBarrierValueKind valueKind = ArkSteedWriteBarrierValueKind::Unknown);

private:
    void EmitFastWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value,
                              int32_t offset, ArkSteedRegister offsetScratch);
    void EmitWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value, int32_t offset,
                          ArkSteedWriteBarrierKind barrierKind, ArkSteedRegister primaryScratch,
                          ArkSteedRegister offsetScratch, ArkSteedRegister secondaryScratch);
    void EmitLocalToShareRSet(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value, int32_t offset,
                              ArkSteedRegister objectRegionScratch, ArkSteedRegister bitsetWordAddrScratch,
                              ArkSteedRegister bitScratch, Label *next);
    void EmitPostStoreWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object, ArkSteedRegister value,
                                   int32_t offset, ArkSteedWriteBarrierKind barrierKind,
                                   ArkSteedRegister primaryScratch, ArkSteedRegister offsetScratch,
                                   ArkSteedRegister secondaryScratch);
    void CallBarrierRuntime(kungfu::RuntimeStubCSigns::ID runtimeId, ArkSteedRegister glue,
                            ArkSteedRegister object, int32_t offset, ArkSteedRegister value,
                            ArkSteedRegister offsetScratch, bool preserveInputs);

    ArkSteedAssembler *assembler_ {nullptr};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_WRITE_BARRIER_H
