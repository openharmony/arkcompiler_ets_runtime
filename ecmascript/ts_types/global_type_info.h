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

#ifndef ECMASCRIPT_TS_TYPES_GLOBAL_TYPE_INFO_H
#define ECMASCRIPT_TS_TYPES_GLOBAL_TYPE_INFO_H

#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
using PGOSampleType = pgo::PGOSampleType;
using PGORWOpType = pgo::PGORWOpType;
// a unique ID to determine whether the corresponding GT has been generated
class GlobalTypeID {
public:
    explicit GlobalTypeID(const JSPandaFile *jsPandaFile, PGOSampleType pgoTypeId)
        : jsPandaFile_(jsPandaFile), typeId_(0), pgoTypeId_(pgoTypeId) {}

    explicit GlobalTypeID(const JSPandaFile *jsPandaFile, uint32_t typeId)
        : jsPandaFile_(jsPandaFile), typeId_(typeId), pgoTypeId_() {}

    bool operator==(const GlobalTypeID &id) const
    {
        return jsPandaFile_ == id.jsPandaFile_ &&
               typeId_ == id.typeId_ &&
               pgoTypeId_ == id.pgoTypeId_;
    }

    const JSPandaFile *GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

    uint32_t GetTypeId() const
    {
        return typeId_;
    }

    PGOSampleType GetPGOTypeId() const
    {
        return pgoTypeId_;
    }

    bool IsPGOType() const
    {
        return !pgoTypeId_.IsNone();
    }

private:
    const JSPandaFile *jsPandaFile_;
    uint32_t typeId_;         // the information is recording in the typeliteral
    PGOSampleType pgoTypeId_; // the information is recording in the '.ap' file
};

struct HashGlobalTypeID {
    size_t operator()(const GlobalTypeID &id) const
    {
        if (id.IsPGOType()) {
            return std::hash<const JSPandaFile*>()(id.GetJSPandaFile()) ^
                   std::hash<uint64_t>()(id.GetPGOTypeId().GetProfileType().GetRaw());
        }
        return std::hash<const JSPandaFile*>()(id.GetJSPandaFile()) ^
               std::hash<uint64_t>()(id.GetTypeId());
    }
};

// this class records information about definition points related to class and object types.
class TypeLocation {
public:
    explicit TypeLocation(const JSPandaFile *jsPandaFile, uint32_t methodOffset,
                          int32_t bcIdx)
        : jsPandaFile_(jsPandaFile), methodOffset_(methodOffset), bcIdx_(bcIdx) {}

    bool operator==(const TypeLocation &loc) const
    {
        return jsPandaFile_ == loc.jsPandaFile_ &&
               methodOffset_ == loc.methodOffset_ &&
               bcIdx_ == loc.bcIdx_;
    }

    int32_t GetBcIdx() const
    {
        return bcIdx_;
    }

    void SetBcIdx(int32_t bcIdx)
    {
        bcIdx_ = bcIdx;
    }

    const JSPandaFile *GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

    uint32_t GetMethodOffset() const
    {
        return methodOffset_;
    }

private:
    const JSPandaFile *jsPandaFile_;
    uint32_t methodOffset_;
    int32_t bcIdx_;
};

struct HashTypeLocation {
    size_t operator()(const TypeLocation &loc) const
    {
        return std::hash<const JSPandaFile*>()(loc.GetJSPandaFile()) ^
               std::hash<uint32_t>()(loc.GetMethodOffset()) ^
               std::hash<int32_t>()(loc.GetBcIdx());
    }
};
}  // panda::ecmascript
#endif  // ECMASCRIPT_TS_TYPES_GLOBAL_TYPE_INFO_H
