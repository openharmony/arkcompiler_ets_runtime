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

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/pgo_profiler/pgo_profiler_loader.h"
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
        outerMethodId_ = outMethodId;
        lexVarTypes_.resize(numOfLexVars, GateType::AnyType());
        status_ = status;
    }

    inline uint32_t GetOutMethodId() const
    {
        return outerMethodId_;
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
    uint32_t outerMethodId_ { DEFAULT_ROOT };
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
    explicit MethodInfo(uint32_t methodInfoIndex, uint32_t methodPcInfoIndex, uint32_t outMethodIdx,
                        uint32_t outMethodOffset = MethodInfo::DEFAULT_OUTMETHOD_OFFSET, uint32_t num = 0,
                        LexicalEnvStatus lexEnvStatus = LexicalEnvStatus::VIRTUAL_LEXENV)
        : methodInfoIndex_(methodInfoIndex), methodPcInfoIndex_(methodPcInfoIndex), outerMethodId_(outMethodIdx),
          outerMethodOffset_(outMethodOffset), numOfLexVars_(num), status_(lexEnvStatus)
    {
    }

    ~MethodInfo() = default;

    static constexpr uint32_t DEFAULT_OUTMETHOD_OFFSET = 0;

    inline uint32_t GetOutMethodId() const
    {
        return outerMethodId_;
    }

    inline uint32_t SetOutMethodId(uint32_t outMethodId)
    {
        return outerMethodId_ = outMethodId;
    }

    inline uint32_t GetOutMethodOffset() const
    {
        return outerMethodOffset_;
    }

    inline uint32_t SetOutMethodOffset(uint32_t outMethodOffset)
    {
        return outerMethodOffset_ = outMethodOffset;
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

    bool IsPGO() const
    {
        return isPgoMarked_;
    }

    void SetIsPGO(bool pgoMark)
    {
        isPgoMarked_ = pgoMark;
    }

    bool IsCompiled() const
    {
        return isCompiled_;
    }

    void SetIsCompiled(bool isCompiled)
    {
        isCompiled_ = isCompiled;
    }

private:
    // used to record the index of the current MethodInfo to speed up the lookup of lexEnv
    uint32_t methodInfoIndex_ { 0 };
    // used to obtain MethodPcInfo from the vector methodPcInfos of struct BCInfo
    uint32_t methodPcInfoIndex_ { 0 };
    std::vector<uint32_t> innerMethods_ {};
    uint32_t outerMethodId_ { LexEnv::DEFAULT_ROOT };
    uint32_t outerMethodOffset_ { MethodInfo::DEFAULT_OUTMETHOD_OFFSET };
    uint32_t numOfLexVars_ { 0 };
    LexicalEnvStatus status_ { LexicalEnvStatus::VIRTUAL_LEXENV };
    bool isPgoMarked_ {false};
    bool isCompiled_ {false};
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
    explicit BCInfo(size_t maxAotMethodSize)
        : maxMethodSize_(maxAotMethodSize)
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

    size_t GetMaxMethodSize() const
    {
        return maxMethodSize_;
    }

    bool IsSkippedMethod(uint32_t methodOffset) const
    {
        if (skippedMethods_.find(methodOffset) == skippedMethods_.end()) {
            return false;
        }
        return true;
    }

    void AddSkippedMethod(uint32_t methodOffset)
    {
        skippedMethods_.insert(methodOffset);
    }

    void EraseSkippedMethod(uint32_t methodOffset)
    {
        if (skippedMethods_.find(methodOffset) != skippedMethods_.end()) {
            skippedMethods_.erase(methodOffset);
        }
    }

    void AddRecordName(const CString &recordName)
    {
        recordNames_.emplace_back(recordName);
    }

    CString GetRecordName(uint32_t index) const
    {
        return recordNames_[index];
    }

    void AddMethodOffsetToRecordName(uint32_t methodOffset, CString recordName)
    {
        methodOffsetToRecordName_.emplace(methodOffset, recordName);
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

    uint32_t GetDefineMethod(const uint32_t classLiteralOffset) const
    {
        return classTypeLOffsetToDefMethod_.at(classLiteralOffset);
    }

    bool HasClassDefMethod(const uint32_t classLiteralOffset) const
    {
        return classTypeLOffsetToDefMethod_.find(classLiteralOffset) != classTypeLOffsetToDefMethod_.end();
    }

    void SetClassTypeOffsetAndDefMethod(uint32_t classLiteralOffset, uint32_t methodOffset)
    {
        if (classTypeLOffsetToDefMethod_.find(classLiteralOffset) == classTypeLOffsetToDefMethod_.end()) {
            classTypeLOffsetToDefMethod_.emplace(classLiteralOffset, methodOffset);
        }
    }
private:
    std::vector<uint32_t> mainMethodIndexes_ {};
    std::vector<CString> recordNames_ {};
    std::vector<MethodPcInfo> methodPcInfos_ {};
    std::unordered_map<uint32_t, MethodInfo> methodList_ {};
    std::unordered_map<uint32_t, CString> methodOffsetToRecordName_ {};
    std::set<uint32_t> skippedMethods_ {};
    ConstantPoolInfo cpInfo_;
    size_t maxMethodSize_;
    std::unordered_map<uint32_t, uint32_t> classTypeLOffsetToDefMethod_ {};
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
    explicit BytecodeInfoCollector(EcmaVM *vm, JSPandaFile *jsPandaFile,
                                   size_t maxAotMethodSize, bool enableCollectLiteralInfo)
        : vm_(vm), jsPandaFile_(jsPandaFile), bytecodeInfo_(maxAotMethodSize),
          enableCollectLiteralInfo_(enableCollectLiteralInfo)
    {
        ProcessClasses();
    }
    ~BytecodeInfoCollector() = default;
    NO_COPY_SEMANTIC(BytecodeInfoCollector);
    NO_MOVE_SEMANTIC(BytecodeInfoCollector);

    bool EnableCollectLiteralInfo() const
    {
        return enableCollectLiteralInfo_;
    }

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

    inline std::string GetClassName(const EntityId entityId)
    {
        std::string className(MethodLiteral::GetMethodName(jsPandaFile_, entityId));
        if (LIKELY(className.find('#') != std::string::npos)) {
            size_t poiIndex = className.find_last_of('#');
            className = className.substr(poiIndex + 1);
        }
        return className;
    }

    const CString GetEntryFunName(const std::string_view &entryPoint) const;
    void ProcessClasses();
    void CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr,
        const MethodLiteral *method, std::vector<std::string> &classNameVec);
    void SetMethodPcInfoIndex(uint32_t methodOffset, const std::pair<size_t, uint32_t> &processedMethodInfo);
    void CollectInnerMethods(const MethodLiteral *method, uint32_t innerMethodOffset);
    void CollectInnerMethods(uint32_t methodId, uint32_t innerMethodOffset);
    void CollectInnerMethodsFromLiteral(const MethodLiteral *method, uint64_t index);
    void NewLexEnvWithSize(const MethodLiteral *method, uint64_t numOfLexVars);
    void CollectInnerMethodsFromNewLiteral(const MethodLiteral *method, panda_file::File::EntityId literalId);
    void CollectMethodInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method,
                                 std::vector<std::string> &classNameVec, int32_t bcIndex);
    void CollectConstantPoolIndexInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method);
    void IterateLiteral(const MethodLiteral *method, std::vector<uint32_t> &classOffsetVector);
    void CollectClassLiteralInfo(const MethodLiteral *method, const std::vector<std::string> &classNameVec);

    EcmaVM *vm_;
    JSPandaFile *jsPandaFile_ {nullptr};
    BCInfo bytecodeInfo_;
    size_t methodInfoIndex_ {0};
    bool enableCollectLiteralInfo_ {false};
    std::set<int32_t> classDefBCIndexes_ {};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H
