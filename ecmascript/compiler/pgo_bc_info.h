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

#ifndef ECMASCRIPT_COMPILER_PGO_BC_INFO_RECORDER_H
#define ECMASCRIPT_COMPILER_PGO_BC_INFO_RECORDER_H

#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"
#include "libpandafile/bytecode_instruction.h"

namespace panda::ecmascript::kungfu {
class PGOBCInfo {
public:
    enum Type {
        OBJ_LITERAL = 0,
        ARRAY_LITERAL,
        EMPTY_ARRAY,
        CALL_TARGET,
        CLASS,
        FUNCTION,

        TYPE_NUM,
        TYPE_FIRST = OBJ_LITERAL,
        TYPE_LAST = FUNCTION,
    };

    struct InfoDetail {
        const CString &recordName;
        uint32_t methodOffset;
        uint32_t bcIndex;
        uint32_t bcOffset;
        uint32_t cpIndex;
    };

    class Info {
    public:
        void Record(const InfoDetail &detail);

        uint32_t GetPGOExtendGTCount(const CString &recordName) const;

        template <class Callback>
        void IterateValByMethodOffset(uint32_t methodOffset, Type type, const Callback &cb) const
        {
            if (methodOffsetToValVec_.find(methodOffset) != methodOffsetToValVec_.end()) {
                const ValVec &valVec = methodOffsetToValVec_.at(methodOffset);
                for (auto val : valVec) {
                    cb(type, val.bcIndex, val.bcOffset, val.cpIndex);
                }
            }
        }

        template <class Callback>
        void IterateValByMethodOffsetAndType(uint32_t methodOffset, const Callback &cb) const
        {
            if (methodOffsetToValVec_.find(methodOffset) != methodOffsetToValVec_.end()) {
                const ValVec &valVec = methodOffsetToValVec_.at(methodOffset);
                for (auto val : valVec) {
                    cb(val.bcIndex, val.bcOffset, val.cpIndex);
                }
            }
        }

    private:
        struct Val {
            uint32_t bcIndex;
            uint32_t bcOffset;
            uint32_t cpIndex;
        };
        using ValVec = CVector<Val>;
        CMap<CString, uint32_t> recordNameToValCount_;
        CUnorderedMap<uint32_t, ValVec> methodOffsetToValVec_;
    };

    PGOBCInfo() : infos_(Type::TYPE_NUM, Info{}) {}
    ~PGOBCInfo() = default;

    const Info& GetInfo(Type type) const;

    uint32_t GetPGOExtendGTCount(const CString &recordName) const;

    void PUBLIC_API Record(const BytecodeInstruction &bcIns, int32_t bcIndex,
                           const CString &recordName, const MethodLiteral *method);

    template <class Callback>
    void IterateInfoAndType(uint32_t methodOffset, const Callback &cb) const
    {
        for (size_t idx = 0; idx < Type::TYPE_NUM; ++idx) {
            Type type = static_cast<Type>(idx);
            infos_[idx].IterateValByMethodOffset(methodOffset, type, cb);
        }
    }

    template <class Callback>
    void IterateInfoByType(uint32_t methodOffset, Type type, const Callback &cb) const
    {
        infos_[type].IterateValByMethodOffsetAndType(methodOffset, cb);
    }
private:
    void Record(const InfoDetail &detail, Type type);

    CVector<Info> infos_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_PGO_BC_INFO_RECORDER_H
