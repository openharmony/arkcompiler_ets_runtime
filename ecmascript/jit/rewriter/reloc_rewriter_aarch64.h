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

#ifndef ECMASCRIPT_COECMASCRIPT_COMPILER_ASSEMBLER_RELOC_REWRITER_AARCH64_RELOC_REWRITER_H
#define ECMASCRIPT_COECMASCRIPT_COMPILER_ASSEMBLER_RELOC_REWRITER_AARCH64_RELOC_REWRITER_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/jit/rewriter/reloc_rewriter.h"

namespace panda::ecmascript::kungfu {

class PUBLIC_API RelocWriterAArch64 : public RelocWriter {
    static const int INSTRUCT_SIZE = 4;
    static const int MAX_JUMP_SIZE = 128 * 1024 * 1024;
    public:
        bool PUBLIC_API RewriteRelocInfo(uint8_t *codeAddr, uint8_t *jitAllocAddr, RelocMap &relocInfo) override;
        virtual ~RelocWriterAArch64() = default;
};

}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COECMASCRIPT_COMPILER_ASSEMBLER_RELOC_REWRITER_AARCH64_RELOC_REWRITER_H