/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_DFX_VMSTAT_OPT_CODE_PROFILER_H
#define ECMASCRIPT_DFX_VMSTAT_OPT_CODE_PROFILER_H

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/compiler/share_opcodes.h"
#include <set>

// bcIndex   bytecode   count    fast    slow    typerate
// ====================(print all)=======================
// ........
// ======================================================
// total

// tip: not print if count below 10000
// ====================(top1 method: name, methodId)=====
// .......
// (bcIndex   bytecode   count    fast    slow    typerate)
// .......
// ======================================================

// ====================(top2 method: name)===============
// .......
// ======================================================

// .......

// ====================(top10 method: name)==============
// .......
// ======================================================

// methodId, abcId, bcIndex, Value1(opcode, fastcount, slowcount)，Value2(methodName, totalcount)
// key: methodId, abcId
// std::map<key, <bcIndex, Value1>>
// std::map<key, Value2>

namespace panda::ecmascript {
class OptCodeProfiler {
public:
    using EcmaOpcode = kungfu::EcmaOpcode;

    enum Mode {
        TYPED_PATH,
        SLOW_PATH,
    };

    struct Value {
        Value() : typedPathValue(0), slowPathValue(0) {}

        uint64_t TypedPathCount() const
        {
            return typedPathValue;
        }

        uint64_t SlowPathCount() const
        {
            return slowPathValue;
        }

        uint64_t Count() const
        {
            return typedPathValue + slowPathValue;
        }

        void ResetStat()
        {
            typedPathValue = 0;
            slowPathValue = 0;
        }

        uint64_t typedPathValue;
        uint64_t slowPathValue;
    };

    struct Key {
        Key(uint32_t abcId, uint32_t methodId)
        {
            abcAndMethodId_ = (((uint64_t)abcId) << 32) + methodId; // 32： 32bit
        }

        Key(uint64_t key) : abcAndMethodId_(key) {};

        uint32_t GetAbcId() const
        {
            return (uint32_t) (abcAndMethodId_ >> 32); // 32: 32bit
        }

        uint32_t GetMethodId() const
        {
            return (uint32_t) abcAndMethodId_;
        }

        uint64_t Value() const
        {
            return abcAndMethodId_;
        }

        uint64_t abcAndMethodId_;
    };

    struct Name {
        Name(std::string name) : methodName_(name), totalCount_(1) {}

        uint64_t Count() const
        {
            return totalCount_;
        }

        void Inc()
        {
            totalCount_ = totalCount_ + 1;
        }

        std::string GetName() const
        {
            return methodName_;
        }

        std::string methodName_;
        uint64_t totalCount_;
    };

    struct Record {
        Record(EcmaOpcode opcode) : opcode_(opcode), fast_(0), slow_(0) {}

        uint64_t Count() const
        {
            return fast_ + slow_;
        }

        uint64_t GetFast() const
        {
            return fast_;
        }

        uint64_t GetSlow() const
        {
            return slow_;
        }

        EcmaOpcode GetOpCode() const
        {
            return opcode_;
        }

        void IncFast()
        {
            fast_ = fast_ + 1;
        }

        void IncSlow()
        {
            slow_ = slow_ + 1;
        }

        EcmaOpcode opcode_;
        uint64_t fast_;
        uint64_t slow_;
    };

    OptCodeProfiler()
    {
#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
        profMap_ = {
#define BYTECODE_PROF_MAP(name) { kungfu::EcmaOpcode::name, OptCodeProfiler::Value() },
    ECMA_OPCODE_LIST(BYTECODE_PROF_MAP)
#undef BYTECODE_PROF_MAP
        };
#endif
    }

    ~OptCodeProfiler();

    void Update(JSHandle<JSTaggedValue> &func, int bcIndex, EcmaOpcode opcode, Mode mode);

    void PrintAndReset();

    using BcRecord = std::map<int, Record>;
private:
    void FilterMethodToPrint();
    void PrintMethodRecord(Key key, std::string methodName);
    void ResetMethodInfo()
    {
        methodIdToRecord_.clear();
        methodIdToName_.clear();
    }
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER == 0
    int printMehodCount_ {10};
#endif
    std::map<EcmaOpcode, Value> profMap_;
    std::map<uint64_t, BcRecord> methodIdToRecord_;
    std::map<uint64_t, Name> methodIdToName_;
    std::vector<CString> abcNames_;
};

class TypedOpProfiler {
public:
    using OpCode = kungfu::OpCode;

    TypedOpProfiler()
    {
        strOpMap_ = {
#define DECLARE_GATE_OPCODE(NAME, OP, R, S, D, V) \
        { #OP, OpCode::OP },

    MCR_IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_PC_OFFSET(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_FOR_CALL(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_VALUE(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_BOOL(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_GATE_TYPE(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_VALUE_IN(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
        };

        profMap_ = {
#define DECLARE_GATE_OPCODE(NAME, OP, R, S, D, V) \
        { OpCode::OP, 0 },

    MCR_IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_PC_OFFSET(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_FOR_CALL(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_VALUE(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_BOOL(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_GATE_TYPE(DECLARE_GATE_OPCODE)
    MCR_GATE_META_DATA_LIST_WITH_VALUE_IN(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
        };
    }

    void Update(OpCode opcode)
    {
        auto it = profMap_.find(opcode);
        if (it != profMap_.end()) {
            it->second++;
            recordOp_.insert(opcode);
        }
    }

    void Print(std::string opStr)
    {
        auto it = strOpMap_.find(opStr);
        if (it != strOpMap_.end()) {
            LOG_TRACE(INFO) << "Opcode: " << it->first << " Count:"
                            << profMap_.at(it->second);
        }
    }

    void Clear()
    {
        for (OpCode op : recordOp_) {
            profMap_.at(op) = 0;
        }
        recordOp_.clear();
    }

private:
    std::map<std::string, OpCode> strOpMap_;
    std::map<OpCode, uint64_t> profMap_;
    std::set<OpCode> recordOp_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_VMSTAT_OPT_CODE_PROFILER_H
