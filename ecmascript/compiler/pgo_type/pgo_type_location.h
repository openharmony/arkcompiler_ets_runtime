/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_ID_H
#define ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_ID_H

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"

namespace panda::ecmascript::kungfu {
using ProfileType = pgo::ProfileType;
using ProfileTyper = std::pair<ProfileType, ProfileType>;
using ProfileTypeTuple = std::tuple<ProfileType, ProfileType, uint64_t>;

class PGOTypeLocation {
public:
    enum class Type : uint8_t {
        LOCAL,
        PROTOTYPE,
        CONSTRUCTOR,
    };

    explicit PGOTypeLocation(const JSPandaFile *jsPandaFile, uint32_t methodOffset,
                             int32_t bcIdx)
        : abcName_(jsPandaFile->GetNormalizedFileDesc()), methodOffset_(methodOffset), bcIdx_(bcIdx) {}
    explicit PGOTypeLocation(const CString &abcName, uint32_t methodOffset,
                             int32_t bcIdx)
        : abcName_(abcName), methodOffset_(methodOffset), bcIdx_(bcIdx) {}

    bool operator==(const PGOTypeLocation &loc) const
    {
        return abcName_ == loc.abcName_ &&
               methodOffset_ == loc.methodOffset_ &&
               bcIdx_ == loc.bcIdx_ &&
               type_ == loc.type_;
    }

    PGOTypeLocation ChangeType(Type type) const
    {
        auto loc = *this;
        loc.type_ = type;
        return loc;
    }

    const CString& GetAbcName() const
    {
        return abcName_;
    }

    uint32_t GetMethodOffset() const
    {
        return methodOffset_;
    }

    int32_t GetBcIdx() const
    {
        return bcIdx_;
    }

private:
    CString abcName_;
    uint32_t methodOffset_;
    int32_t bcIdx_;
    Type type_ { Type::LOCAL };
};

struct HashPGOTypeLocation {
    size_t operator()(const PGOTypeLocation &loc) const
    {
        return std::hash<CString>()(loc.GetAbcName()) ^
               std::hash<uint32_t>()(loc.GetMethodOffset()) ^
               std::hash<int32_t>()(loc.GetBcIdx());
    }
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_ID_H
