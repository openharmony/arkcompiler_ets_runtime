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

#include "ecmascript/arksteed/arksteed_assembler.h"

#include "ecmascript/arksteed/arksteed_disassembler.h"

namespace panda::ecmascript::arksteed {

ArkSteedAssembler::ArkSteedAssembler(Chunk *chunk, JSThread *compilerThread, JSThread *entryThread)
    : assembler_(chunk), compilerThread_(compilerThread), entryThread_(entryThread)
{}

ArkSteedAssembler::~ArkSteedAssembler() = default;

// =============================================================================
// Platform-independent implementations
// =============================================================================

void ArkSteedAssembler::LoadTaggedValue(ArkSteedRegister dst, uint64_t taggedValue)
{
    Move(dst, taggedValue);
}

void ArkSteedAssembler::RecordComment(const char *str)
{
    if (!enableComments_) {
        return;
    }
    uint32_t pcOffset = static_cast<uint32_t>(assembler_.GetCurrentPosition());
    comments_.Add(pcOffset, str);
}

void ArkSteedAssembler::Disassemble(std::ostream &os, const uint8_t *commentsData, uint32_t commentsSize)
{
    ArkSteedDisassembler disassembler(GetCodeBuffer(), GetCodeSize(), commentsData, commentsSize);
    disassembler.Disassemble(os);
}

}  // namespace panda::ecmascript::arksteed
