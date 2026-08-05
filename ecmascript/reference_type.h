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

#ifndef ECMASCRIPT_REFERENCE_TYPE_H
#define ECMASCRIPT_REFERENCE_TYPE_H

namespace panda::ecmascript {
#ifdef USE_COMPRESSED_POINTER
enum class ReferenceType : size_t {
    NORMAL,
    COMPRESSED,
    TOTAL_CNT,
};
static_assert(static_cast<size_t>(ReferenceType::TOTAL_CNT) == 2);
#else
enum class ReferenceType : size_t {
    NORMAL,
    COMPRESSED = NORMAL,
    TOTAL_CNT,
};
static_assert(static_cast<size_t>(ReferenceType::TOTAL_CNT) == 1);
#endif
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_REFERENCE_TYPE_H
