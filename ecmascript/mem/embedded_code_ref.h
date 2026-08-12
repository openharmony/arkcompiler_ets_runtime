/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_MEM_EMBEDDED_CODE_REF_H
#define ECMASCRIPT_MEM_EMBEDDED_CODE_REF_H

#include <cstdint>

namespace panda::ecmascript {

enum class EmbeddedCodeRefRelocKind : uint8_t {
    X64_MOVABS_IMM64,
    ARM64_LITERAL64,
};

struct EmbeddedCodeRefReloc {
    uint32_t codeOffset {0};
    uint32_t handleIndex {0};
    EmbeddedCodeRefRelocKind kind {EmbeddedCodeRefRelocKind::X64_MOVABS_IMM64};
    uint8_t width {0};
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_EMBEDDED_CODE_REF_H
