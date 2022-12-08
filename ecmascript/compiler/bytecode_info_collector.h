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

#ifndef ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H
#define ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H

#include "ecmascript/dfx/pgo_profiler/pgo_profiler_loader.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::kungfu {
/*    ts source code
 *    let a:number = 1;
 *    function f() {
 *        let b:number = 1;
 *        function g() {
 *            return a + b;
 *        }
 *        return g();
 *    }
 *
 *                                     The structure of Lexical Environment
 *
 *                                               Lexical Environment             Lexical Environment
 *               Global Environment                 of function f                   of function g
 *              +-------------------+ <----+    +-------------------+ <----+    +-------------------+
 *    null <----|  Outer Reference  |      +----|  Outer Reference  |      +----|  Outer Reference  |
 *              +-------------------+           +-------------------+           +-------------------+
 *              |Environment Recoder|           |Environment Recoder|           |Environment Recoder|
 *              +-------------------+           +-------------------+           +-------------------+
 *
 *    We only record the type of the variable in Environment Recoder.
 *    In the design of the Ark bytecode, if a method does not have any
 *    lex-env variable in its Lexical Environment, then there will be
 *    no EcmaOpcode::NEWLEXENV in it which leads to ARK runtime will
 *    not create a Lexical Environment when the method is executed.
 *    In order to simulate the state of the runtime as much as possible,
 *    a field named 'status' will be added into the class LexEnv to
 *    measure this state. Take the above code as an example, although in
 *    static analysis, we will create LexEnv for each method, only Lexenvs
 *    of global and function f will be created when methods are executed.
 */

enum class LexicalEnvStatus : uint8_t {
    VIRTUAL_LEXENV,
    REALITY_LEXENV
};

class LexEnv {
public:
    LexEnv() = default;
    ~LexEnv() = default;

    static constexpr uint32_t DEFAULT_ROOT = std::numeric_limits<uint32_t>::max();

    inline void Inilialize(uint32_t outMethodId, uint32_t numOfLexVars, LexicalEnvStatus status)
    {
        outMethodId_ = outMethodId;
        lexVarTypes_.resize(numOfLexVars, GateType::AnyType());
        status_ = status;
    }

    inline uint32_t GetOutMethodId() const
    {
        return outMethodId_;
    }

    inline LexicalEnvStatus GetLexEnvStatus() const
    {
        return status_;
    }

    inline GateType GetLexVarType(uint32_t slot) const
    {
        if (slot < lexVarTypes_.size()) {
            return lexVarTypes_[slot];
        }
        return GateType::AnyType();
    }

    inline void SetLexVarType(uint32_t slot, const GateType &type)
    {
        if (slot < lexVarTypes_.size()) {
            lexVarTypes_[slot] = type;
        }
    }

private:
    uint32_t outMethodId_ { DEFAULT_ROOT };
    std::vector<GateType> lexVarTypes_ {};
    LexicalEnvStatus status_ { LexicalEnvStatus::VIRTUAL_LEXENV };
};

// each method in the abc file corresponds to one MethodInfo and
// methods with the same instructions share one common MethodPcInfo
struct MethodPcInfo {
    std::vector<const uint8_t*> pcOffsets {};
    uint32_t methodsSize {0};
};

class MethodInfo {
public:
    explicit MethodInfo(uint32_t methodInfoIndex, uint32_t methodPcInfoIndex, uint32_t outMethodIdx, uint32_t num = 0,
                        LexicalEnvStatus lexEnvStatus = LexicalEnvStatus::VIRTUAL_LEXENV)
        : methodInfoIndex_(methodInfoIndex), methodPcInfoIndex_(methodPcInfoIndex), outMethodId_(outMethodIdx),
          numOfLexVars_(num), status_(lexEnvStatus)
    {
    }

    ~MethodInfo() = default;

    inline uint32_t GetOutMethodId() const
    {
        return outMethodId_;
    }

    inline uint32_t SetOutMethodId(uint32_t outMethodId)
    {
        return outMethodId_ = outMethodId;
    }

    inline uint32_t GetNumOfLexVars() const
    {
        return numOfLexVars_;
    }

    inline uint32_t SetNumOfLexVars(uint32_t numOfLexVars)
    {
        return numOfLexVars_ = numOfLexVars;
    }

    inline LexicalEnvStatus GetLexEnvStatus() const
    {
        return status_;
    }

    inline LexicalEnvStatus SetLexEnvStatus(LexicalEnvStatus status)
    {
        return status_ = status;
    }

    inline uint32_t GetMethodPcInfoIndex() const
    {
        return methodPcInfoIndex_;
    }

    inline uint32_t SetMethodPcInfoIndex(uint32_t methodPcInfoIndex)
    {
        return methodPcInfoIndex_ = methodPcInfoIndex;
    }

    inline uint32_t GetMethodInfoIndex() const
    {
        return methodInfoIndex_;
    }

    inline uint32_t SetMethodInfoIndex(uint32_t methodInfoIndex)
    {
        return methodInfoIndex_ = methodInfoIndex;
    }

    inline void AddInnerMethod(uint32_t offset)
    {
        innerMethods_.emplace_back(offset);
    }

    inline const std::vector<uint32_t> &GetInnerMethods() const
    {
        return innerMethods_;
    }

private:
    // used to record the index of the current MethodInfo to speed up the lookup of lexEnv
    uint32_t methodInfoIndex_ { 0 };
    // used to obtain MethodPcInfo from the vector methodPcInfos of struct BCInfo
    uint32_t methodPcInfoIndex_ { 0 };
    std::vector<uint32_t> innerMethods_ {};
    uint32_t outMethodId_ { LexEnv::DEFAULT_ROOT };
    uint32_t numOfLexVars_ { 0 };
    LexicalEnvStatus status_ { LexicalEnvStatus::VIRTUAL_LEXENV };
};


class ConstantPoolInfo {
public:
    enum ItemType {
        STRING = 0,
        METHOD,
        CLASS_LITERAL,
        OBJECT_LITERAL,
        ARRAY_LITERAL,

        ITEM_TYPE_NUM,
        ITEM_TYPE_FIRST = STRING,
        ITEM_TYPE_LAST = ARRAY_LITERAL,
    };

    struct ItemData {
        uint32_t index {0};
        uint32_t outerMethodOffset {0};
        CString *recordName {nullptr};
    };

    // key:constantpool index, value:ItemData
    using Item = std::unordered_map<uint32_t, ItemData>;

    ConstantPoolInfo() : items_(ItemType::ITEM_TYPE_NUM, Item{}) {}

    Item& GetCPItem(ItemType type)
    {
        ASSERT(ItemType::ITEM_TYPE_FIRST <= type && type <= ItemType::ITEM_TYPE_LAST);
        return items_[type];
    }

    void AddIndexToCPItem(ItemType type, uint32_t index, uint32_t methodOffset);
private:
    std::vector<Item> items_;
};

class BCInfo {
public:
    explicit BCInfo(PGOProfilerLoader &profilerLoader, size_t maxAotMethodSize)
        : pfLoader_(profilerLoader), maxMethodSize_(maxAotMethodSize)
    {
    }

    std::vector<uint32_t>& GetMainMethodIndexes()
    {
        return mainMethodIndexes_;
    }

    std::vector<CString>& GetRecordNames()
    {
        return recordNames_;
    }

    std::vector<MethodPcInfo>& GetMethodPcInfos()
    {
        return methodPcInfos_;
    }

    std::unordered_map<uint32_t, MethodInfo>& GetMethodList()
    {
        return methodList_;
    }

    bool IsSkippedMethod(uint32_t methodOffset) const
    {
        if (skippedMethods_.find(methodOffset) == skippedMethods_.end()) {
            return false;
        }
        return true;
    }

    size_t GetSkippedMethodSize() const
    {
        return skippedMethods_.size();
    }

    void AddIndexToCPInfo(ConstantPoolInfo::ItemType type, uint32_t index, uint32_t methodOffset)
    {
        cpInfo_.AddIndexToCPItem(type, index, methodOffset);
    }

    template <class Callback>
    void IterateConstantPoolInfo(ConstantPoolInfo::ItemType type, const Callback &cb)
    {
        auto &item = cpInfo_.GetCPItem(type);
        for (auto &iter : item) {
            ConstantPoolInfo::ItemData &data = iter.second;
            data.recordName = &methodOffsetToRecordName_[data.outerMethodOffset];
            cb(data);
        }
    }

    template <class Callback>
    void EnumerateBCInfo(JSPandaFile *jsPandaFile, const Callback &cb)
    {
        for (uint32_t i = 0; i < mainMethodIndexes_.size(); i++) {
            std::queue<uint32_t> methodCompiledOrder;
            methodCompiledOrder.push(mainMethodIndexes_[i]);
            while (!methodCompiledOrder.empty()) {
                auto compilingMethod = methodCompiledOrder.front();
                methodCompiledOrder.pop();
                methodOffsetToRecordName_.emplace(compilingMethod, recordNames_[i]);
                auto &methodInfo = methodList_.at(compilingMethod);
                auto &methodPcInfo = methodPcInfos_[methodInfo.GetMethodPcInfoIndex()];
                auto methodLiteral = jsPandaFile->FindMethodLiteral(compilingMethod);
                const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile, methodLiteral->GetMethodId()));
                if (FilterMethod(recordNames_[i], methodLiteral, methodPcInfo)) {
                    skippedMethods_.insert(compilingMethod);
                    LOG_COMPILER(INFO) << " method " << methodName << " has been skipped";
                } else {
                    cb(recordNames_[i], methodName, methodLiteral, compilingMethod,
                       methodPcInfo, methodInfo.GetMethodInfoIndex());
                }
                auto &innerMethods = methodInfo.GetInnerMethods();
                for (auto it : innerMethods) {
                    methodCompiledOrder.push(it);
                }
            }
        }
    }
private:
    bool FilterMethod(const CString &recordName, const MethodLiteral *methodLiteral,
                      const MethodPcInfo &methodPCInfo) const
    {
        if (methodPCInfo.methodsSize > maxMethodSize_ ||
            !pfLoader_.Match(recordName, methodLiteral->GetMethodId())) {
            return true;
        }
        return false;
    }

    std::vector<uint32_t> mainMethodIndexes_ {};
    std::vector<CString> recordNames_ {};
    std::vector<MethodPcInfo> methodPcInfos_ {};
    std::unordered_map<uint32_t, MethodInfo> methodList_ {};
    std::unordered_map<uint32_t, CString> methodOffsetToRecordName_ {};
    std::set<uint32_t> skippedMethods_ {};
    ConstantPoolInfo cpInfo_;
    PGOProfilerLoader &pfLoader_;
    size_t maxMethodSize_;
};

class LexEnvManager {
public:
    explicit LexEnvManager(BCInfo &bcInfo);
    ~LexEnvManager() = default;
    NO_COPY_SEMANTIC(LexEnvManager);
    NO_MOVE_SEMANTIC(LexEnvManager);

    void SetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot, const GateType &type);
    GateType GetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot) const;

private:
    uint32_t GetTargetLexEnv(uint32_t methodId, uint32_t level) const;

    inline uint32_t GetOutMethodId(uint32_t methodId) const
    {
        return lexEnvs_[methodId].GetOutMethodId();
    }

    inline LexicalEnvStatus GetLexEnvStatus(uint32_t methodId) const
    {
        return lexEnvs_[methodId].GetLexEnvStatus();
    }

    inline bool HasDefaultRoot(uint32_t methodId) const
    {
        return GetOutMethodId(methodId) == LexEnv::DEFAULT_ROOT;
    }

    std::vector<LexEnv> lexEnvs_ {};
};

class BytecodeInfoCollector {
public:
    explicit BytecodeInfoCollector(EcmaVM *vm, JSPandaFile *jsPandaFile, PGOProfilerLoader &profilerLoader,
                                   size_t maxAotMethodSize)
        : vm_(vm), jsPandaFile_(jsPandaFile), bytecodeInfo_(profilerLoader, maxAotMethodSize)
    {
        ProcessClasses();
    }
    ~BytecodeInfoCollector() = default;
    NO_COPY_SEMANTIC(BytecodeInfoCollector);
    NO_MOVE_SEMANTIC(BytecodeInfoCollector);

    BCInfo& GetBytecodeInfo()
    {
        return bytecodeInfo_;
    }

    bool IsSkippedMethod(uint32_t methodOffset) const
    {
        return bytecodeInfo_.IsSkippedMethod(methodOffset);
    }

    const JSPandaFile* GetJSPandaFile()
    {
        return jsPandaFile_;
    }

    template <class Callback>
    void IterateConstantPoolInfo(ConstantPoolInfo::ItemType type, const Callback &cb)
    {
        bytecodeInfo_.IterateConstantPoolInfo(type, cb);
    }

private:
    inline size_t GetMethodInfoID()
    {
        return methodInfoIndex_++;
    }

    void AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType type,
                                      uint32_t index, uint32_t methodOffset)
    {
        bytecodeInfo_.AddIndexToCPInfo(type, index, methodOffset);
    }

    const CString GetEntryFunName(const std::string_view &entryPoint) const;
    void ProcessClasses();
    void CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr, const MethodLiteral *method);
    void SetMethodPcInfoIndex(uint32_t methodOffset, const std::pair<size_t, uint32_t> &processedMethodInfo);
    void CollectInnerMethods(const MethodLiteral *method, uint32_t innerMethodOffset);
    void CollectInnerMethods(uint32_t methodId, uint32_t innerMethodOffset);
    void CollectInnerMethodsFromLiteral(const MethodLiteral *method, uint64_t index);
    void NewLexEnvWithSize(const MethodLiteral *method, uint64_t numOfLexVars);
    void CollectInnerMethodsFromNewLiteral(const MethodLiteral *method, panda_file::File::EntityId literalId);
    void CollectMethodInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method);
    void CollectConstantPoolIndexInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method);

    EcmaVM *vm_;
    JSPandaFile *jsPandaFile_ {nullptr};
    BCInfo bytecodeInfo_;
    size_t methodInfoIndex_ {0};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H
