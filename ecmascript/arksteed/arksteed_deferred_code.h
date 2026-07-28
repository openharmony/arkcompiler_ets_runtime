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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_DEFERRED_CODE_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_DEFERRED_CODE_H

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {

class ArkSteedDeferredCode {
public:
    virtual void Generate(ArkSteedAssembler *assembler) = 0;

    Label *GetEntryLabel()
    {
        return &entryLabel_;
    }

protected:
    ArkSteedDeferredCode() = default;
    ~ArkSteedDeferredCode() = default;

private:
    Label entryLabel_;
};

using ArkSteedDeferredCodeList = ChunkVector<ArkSteedDeferredCode *>;

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_DEFERRED_CODE_H
