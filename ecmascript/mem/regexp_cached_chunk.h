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

#ifndef ECMASCRIPT_MEM_REGEXP_CACHED_CHUNK_H
#define ECMASCRIPT_MEM_REGEXP_CACHED_CHUNK_H

#include "ecmascript/common.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/ecma_list.h"
#include "ecmascript/mem/area.h"
#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript {
class NativeAreaAllocator;
class RegExpCachedChunk : public Chunk {
public:

    explicit RegExpCachedChunk(JSThread *JSThread);

    NO_COPY_SEMANTIC(RegExpCachedChunk);
    NO_MOVE_SEMANTIC(RegExpCachedChunk);

private:
    JSThread *jsThread_ ;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_REGEXP_CACHED_CHUNK_H