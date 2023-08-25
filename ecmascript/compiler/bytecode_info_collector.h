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
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/compiler/pgo_bc_info.h"
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

// importRecord : ldExternalModuleVar {importIndex} --> {exportRecordName} : stmoduleVar {exportIndex}
class ImportRecordInfo {
public:
    ImportRecordInfo() = default;
    ~ImportRecordInfo() = default;

    void AddImportIdAndRecord(uint32_t importId, uint32_t bindingId, const CString &importRecord)
    {
        records_.insert(importRecord);
        if (idToRecord_.find(importId) == idToRecord_.end()) {
            idToRecord_.emplace(importId, std::make_pair(importRecord, bindingId));
        }
    }

    const std::set<CString> &GetImportRecords() const
    {
        return records_;
    }

    uint32_t GetImportRecordSize() const
    {
        return records_.size();
    }

    const std::unordered_map<uint32_t, std::pair<CString, uint32_t>> &GetImportIdToExportRecord() const
    {
        return idToRecord_;
    }

private:
    std::set<CString> records_ {};
    std::unordered_map<uint32_t, std::pair<CString, uint32_t>> idToRecord_ {};
};

// exportIndex_ {exportIndex1...}: collect bytecode export index whose type has not been recorded.
// starExportRecord_ {recordName A...}: collect recordName when there is Forwarding syntax in this record,
// like "export * from record A, export * from record B"
class ExportRecordInfo {
public:
    ExportRecordInfo() = default;
    explicit ExportRecordInfo(uint32_t index) : exportIndex_({index}) {}
    explicit ExportRecordInfo(const CString &starRecord) : starExportRecord_({starRecord}) {}

    ~ExportRecordInfo() = default;

    bool HasExportIndex(uint32_t index) const
    {
        return exportIndex_.find(index) != exportIndex_.end();
    }

    bool HasStarExport() const
    {
        return !starExportRecord_.empty();
    }

    void AddExportIndex(uint32_t index)
    {
        exportIndex_.insert(index);
    }

    void AddStarExport(const CString &starExportRecord)
    {
        starExportRecord_.insert(starExportRecord);
    }

    const std::unordered_set<CString> &GetstarExportRecord() const
    {
        return starExportRecord_;
    }

private:
    std::unordered_set<uint32_t> exportIndex_ {};
    std::unordered_set<CString> starExportRecord_ {};
};

class MethodInfo {
public:
    MethodInfo(uint32_t methodInfoIndex, uint32_t methodPcInfoIndex, uint32_t outMethodIdx,
               uint32_t outMethodOffset = MethodInfo::DEFAULT_OUTMETHOD_OFFSET, uint32_t num = 0,
               LexicalEnvStatus lexEnvStatus = LexicalEnvStatus::VIRTUAL_LEXENV, bool isNamespace = false)
        : methodInfoIndex_(methodInfoIndex), methodPcInfoIndex_(methodPcInfoIndex), outerMethodId_(outMethodIdx),
          outerMethodOffset_(outMethodOffset), numOfLexVars_(num), status_(lexEnvStatus), isNamespace_(isNamespace)
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

    inline void AddInnerMethod(uint32_t offset, bool isConstructor)
    {
        if (isConstructor) {
            constructorMethods_.emplace_back(offset);
        } else {
            innerMethods_.emplace_back(offset);
        }
    }

    inline void RearrangeInnerMethods()
    {
        innerMethods_.insert(innerMethods_.begin(), constructorMethods_.begin(), constructorMethods_.end());
    }

    inline void AddBcToTypeId(int32_t bcIndex, uint32_t innerFuncTypeId)
    {
        bcToFuncTypeId_.emplace(bcIndex, innerFuncTypeId);
    }

    inline const std::unordered_map<int32_t, uint32_t> &GetBCAndTypes() const
    {
        return bcToFuncTypeId_;
    }

    inline void MarkMethodNamespace()
    {
        isNamespace_ = true;
    }

    inline bool IsNamespace() const
    {
        return isNamespace_;
    }

    inline const std::vector<uint32_t> &GetInnerMethods() const
    {
        return innerMethods_;
    }

    inline void AddImportIndex(uint32_t index)
    {
        importIndex_.insert(index);
    }

    inline const std::set<uint32_t> &GetImportIndexes() const
    {
        return importIndex_;
    }

    inline void CopyImportIndex(const std::set<uint32_t> &indexSet)
    {
        importIndex_ = indexSet;
    }

    bool IsPGO() const
    {
        return CompileStateBit::PGOBit::Decode(compileState_.value_);
    }

    void SetIsPGO(bool pgoMark)
    {
        CompileStateBit::PGOBit::Set<uint8_t>(pgoMark, &compileState_.value_);
    }

    bool IsCompiled() const
    {
        return CompileStateBit::CompiledBit::Decode(compileState_.value_);
    }

    void SetIsCompiled(bool isCompiled)
    {
        CompileStateBit::CompiledBit::Set<uint8_t>(isCompiled, &compileState_.value_);
    }

    bool IsTypeInferAbort() const
    {
        return CompileStateBit::TypeInferAbortBit::Decode(compileState_.value_);
    }

    void SetTypeInferAbort(bool halfCompiled)
    {
        CompileStateBit::TypeInferAbortBit::Set<uint8_t>(halfCompiled, &compileState_.value_);
    }

    bool IsResolvedMethod() const
    {
        return CompileStateBit::ResolvedMethodBit::Decode(compileState_.value_);
    }

    void SetResolvedMethod(bool isDeoptResolveNeed)
    {
        CompileStateBit::ResolvedMethodBit::Set<uint8_t>(isDeoptResolveNeed, &compileState_.value_);
    }

private:
    class CompileStateBit {
    public:
        explicit CompileStateBit(uint8_t value) : value_(value) {}
        CompileStateBit() = default;
        ~CompileStateBit() = default;
        DEFAULT_COPY_SEMANTIC(CompileStateBit);
        DEFAULT_MOVE_SEMANTIC(CompileStateBit);

        static constexpr size_t BOOL_FLAG_BIT_LENGTH = 1;
        using PGOBit = panda::BitField<bool, 0, BOOL_FLAG_BIT_LENGTH>;
        using CompiledBit = PGOBit::NextField<bool, BOOL_FLAG_BIT_LENGTH>;
        using TypeInferAbortBit = CompiledBit::NextField<bool, BOOL_FLAG_BIT_LENGTH>;
        using ResolvedMethodBit = TypeInferAbortBit::NextField<bool, BOOL_FLAG_BIT_LENGTH>;

    private:
        uint8_t value_ {0};
        friend class MethodInfo;
    };
    // used to record the index of the current MethodInfo to speed up the lookup of lexEnv
    uint32_t methodInfoIndex_ { 0 };
    // used to obtain MethodPcInfo from the vector methodPcInfos of struct BCInfo
    uint32_t methodPcInfoIndex_ { 0 };
    std::vector<uint32_t> innerMethods_ {};
    std::vector<uint32_t> constructorMethods_ {};
    std::unordered_map<int32_t, uint32_t> bcToFuncTypeId_ {};
    uint32_t outerMethodId_ { LexEnv::DEFAULT_ROOT };
    uint32_t outerMethodOffset_ { MethodInfo::DEFAULT_OUTMETHOD_OFFSET };
    uint32_t numOfLexVars_ { 0 };
    LexicalEnvStatus status_ { LexicalEnvStatus::VIRTUAL_LEXENV };
    std::set<uint32_t> importIndex_ {};
    CompileStateBit compileState_ { 0 };
    bool isNamespace_ {false};
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
        uint32_t bcIndex {0};
    };

    // key:constantpool index, value:ItemData
    using Item = std::unordered_map<uint64_t, ItemData>;

    ConstantPoolInfo(JSPandaFile* jsPandaFile) :
        items_(ItemType::ITEM_TYPE_NUM, Item{}), jsPandaFile_(jsPandaFile) {}

    Item& GetCPItem(ItemType type)
    {
        ASSERT(ItemType::ITEM_TYPE_FIRST <= type && type <= ItemType::ITEM_TYPE_LAST);
        return items_[type];
    }

    void AddIndexToCPItem(ItemType type, uint32_t index, uint32_t methodOffset, uint32_t bcIndex);
private:
    static constexpr uint32_t CONSTPOOL_MASK = 32;
    uint64_t GetItemKey(uint32_t index, uint32_t methodOffset);

    std::vector<Item> items_;
    JSPandaFile* jsPandaFile_ {nullptr};
};

struct FastCallInfo {
    bool canFastCall_ {false};
    bool isNoGC_ {false};
};

class BCInfo {
public:
    explicit BCInfo(size_t maxAotMethodSize, JSPandaFile* jsPandaFile)
        : cpInfo_(jsPandaFile), maxMethodSize_(maxAotMethodSize)
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

    // for deopt resolve, when we add new resolve method to compile queue, the recordName vector also need to update
    // for seek, its recordName also need to be set correspondingly
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

    void AddIndexToCPInfo(ConstantPoolInfo::ItemType type, uint32_t index, uint32_t methodOffset, uint32_t bcIndex)
    {
        cpInfo_.AddIndexToCPItem(type, index, methodOffset, bcIndex);
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

    uint32_t IterateFunctionTypeIDAndMethodOffset(uint32_t functionTypeId)
    {
        auto iter = functionTypeIdToMethodOffset_.find(functionTypeId);
        if (iter != functionTypeIdToMethodOffset_.end()) {
            return iter->second;
        }
        return 0;
    }

    void SetFunctionTypeIDAndMethodOffset(uint32_t functionTypeId, uint32_t methodOffset)
    {
        if (functionTypeIdToMethodOffset_.find(functionTypeId) == functionTypeIdToMethodOffset_.end()) {
            functionTypeIdToMethodOffset_.emplace(functionTypeId, methodOffset);
        }
    }
    bool HasExportIndexToRecord(const CString &recordName, uint32_t index) const
    {
        auto iter = recordNameToExportInfo_.find(recordName);
        if (iter != recordNameToExportInfo_.end()) {
            return iter->second.HasExportIndex(index);
        }
        return false;
    }

    bool HasStarExportToRecord(const CString &recordName) const
    {
        auto iter = recordNameToExportInfo_.find(recordName);
        if (iter != recordNameToExportInfo_.end()) {
            return iter->second.HasStarExport();
        }
        return false;
    }

    void AddExportIndexToRecord(const CString &recordName, uint32_t index)
    {
        auto iter = recordNameToExportInfo_.find(recordName);
        if (iter != recordNameToExportInfo_.end()) {
            iter->second.AddExportIndex(index);
        } else {
            ExportRecordInfo info(index);
            recordNameToExportInfo_.emplace(recordName, std::move(info));
        }
    }

    void AddStarExportToRecord(const CString &recordName, const CString &starRecord)
    {
        auto iter = recordNameToExportInfo_.find(recordName);
        if (iter != recordNameToExportInfo_.end()) {
            iter->second.AddStarExport(starRecord);
        } else {
            ExportRecordInfo info(starRecord);
            recordNameToExportInfo_.emplace(recordName, std::move(info));
        }
    }

    const std::unordered_set<CString> &GetstarExportToRecord(const CString &recordName) const
    {
        return recordNameToExportInfo_.at(recordName).GetstarExportRecord();
    }

    void AddImportRecordInfoToRecord(const CString &recordName, const CString &importRecord,
                                     uint32_t importIndex, uint32_t bindingIndex)
    {
        auto iter = recordToImportRecordsInfo_.find(recordName);
        if (iter == recordToImportRecordsInfo_.end()) {
            ImportRecordInfo info;
            info.AddImportIdAndRecord(importIndex, bindingIndex, importRecord);
            recordToImportRecordsInfo_.emplace(recordName, std::move(info));
        } else {
            iter->second.AddImportIdAndRecord(importIndex, bindingIndex, importRecord);
        }
    }

    const std::unordered_map<CString, ImportRecordInfo> &GetImportRecordsInfos() const
    {
        return recordToImportRecordsInfo_;
    }

    FastCallInfo IterateMethodOffsetToFastCallInfo(uint32_t methodOffset, bool *isValid)
    {
        auto iter = methodOffsetToFastCallInfos_.find(methodOffset);
        if (iter != methodOffsetToFastCallInfos_.end()) {
            *isValid = true;
            return iter->second;
        }
        *isValid = false;
        return FastCallInfo();
    }

    void SetMethodOffsetToFastCallInfo(uint32_t methodOffset, bool canFastCall, bool noGC)
    {
        if (methodOffsetToFastCallInfos_.find(methodOffset) == methodOffsetToFastCallInfos_.end()) {
            methodOffsetToFastCallInfos_.emplace(methodOffset, FastCallInfo { canFastCall, noGC });
        }
    }

    void ModifyMethodOffsetToCanFastCall(uint32_t methodOffset, bool canFastCall)
    {
        auto iter = methodOffsetToFastCallInfos_.find(methodOffset);
        bool isNoGC = false;
        if (iter != methodOffsetToFastCallInfos_.end()) {
            isNoGC = iter->second.isNoGC_;
        }
        methodOffsetToFastCallInfos_.erase(methodOffset);
        if (methodOffsetToFastCallInfos_.find(methodOffset) == methodOffsetToFastCallInfos_.end()) {
            methodOffsetToFastCallInfos_.emplace(methodOffset, FastCallInfo { canFastCall, isNoGC });
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
    std::unordered_map<uint32_t, uint32_t> functionTypeIdToMethodOffset_ {};
    std::unordered_map<CString, ExportRecordInfo> recordNameToExportInfo_ {};
    std::unordered_map<CString, ImportRecordInfo> recordToImportRecordsInfo_ {};
    std::unordered_map<uint32_t, FastCallInfo> methodOffsetToFastCallInfos_ {};
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
    BytecodeInfoCollector(EcmaVM *vm, JSPandaFile *jsPandaFile, PGOProfilerDecoder &pfDecoder,
                          size_t maxAotMethodSize, bool enableCollectLiteralInfo);
    ~BytecodeInfoCollector();
    NO_COPY_SEMANTIC(BytecodeInfoCollector);
    NO_MOVE_SEMANTIC(BytecodeInfoCollector);

    bool EnableCollectLiteralInfo() const
    {
        return enableCollectLiteralInfo_;
    }

    Bytecodes* GetByteCodes()
    {
        return &bytecodes_;
    }

    BCInfo& GetBytecodeInfo()
    {
        return bytecodeInfo_;
    }

    BCInfo* GetBytecodeInfoPtr()
    {
        return &bytecodeInfo_;
    }

    PGOBCInfo* GetPGOBCInfo()
    {
        return &pgoBCInfo_;
    }

    bool IsSkippedMethod(uint32_t methodOffset) const
    {
        return bytecodeInfo_.IsSkippedMethod(methodOffset);
    }

    const JSPandaFile *GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

    EcmaVM *GetVM() const
    {
        return vm_;
    }

    LexEnvManager* GetEnvManager() const
    {
        return envManager_;
    }

    template <class Callback>
    void IterateConstantPoolInfo(ConstantPoolInfo::ItemType type, const Callback &cb)
    {
        bytecodeInfo_.IterateConstantPoolInfo(type, cb);
    }

private:
    void ProcessEnvs();

    inline size_t GetMethodInfoID()
    {
        return methodInfoIndex_++;
    }

    void AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType type,
                                      uint32_t index, uint32_t methodOffset, uint32_t bcIndex)
    {
        bytecodeInfo_.AddIndexToCPInfo(type, index, methodOffset, bcIndex);
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
    void RearrangeInnerMethods();
    void CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr,
        MethodLiteral *method, std::vector<std::string> &classNameVec, const CString &recordName,
        uint32_t methodOffset, std::vector<panda_file::File::EntityId> &classConstructIndexes);
    void SetMethodPcInfoIndex(uint32_t methodOffset, const std::pair<size_t, uint32_t> &processedMethodInfo);
    void CollectInnerMethods(const MethodLiteral *method, uint32_t innerMethodOffset, bool isConstructor = false);
    void CollectInnerMethods(uint32_t methodId, uint32_t innerMethodOffset, bool isConstructor = false);
    void CollectInnerMethodsFromLiteral(const MethodLiteral *method, uint64_t index);
    void CollectInnerFuncType(const MethodLiteral *method, uint32_t innerMethodOffset, int32_t bcIndex);
    void NewLexEnvWithSize(const MethodLiteral *method, uint64_t numOfLexVars);
    void CollectInnerMethodsFromNewLiteral(const MethodLiteral *method, panda_file::File::EntityId literalId);
    void CollectMethodInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method,
                                 std::vector<std::string> &classNameVec, int32_t bcIndex,
                                 std::vector<panda_file::File::EntityId> &classConstructIndexes,
                                 bool *canFastCall);
    void CollectModuleInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method,
                                 const CString &recordName);
    void CollectConstantPoolIndexInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method,
                                            uint32_t bcIndex);
    void IterateLiteral(const MethodLiteral *method, std::vector<uint32_t> &classOffsetVector);
    void StoreClassTypeOffset(const uint32_t typeOffset, std::vector<uint32_t> &classOffsetVector);
    void CollectClassLiteralInfo(const MethodLiteral *method, const std::vector<std::string> &classNameVec);
    void CollectFunctionTypeId(panda_file::File::EntityId fieldId);
    void CollectImportIndexs(uint32_t methodOffset, uint32_t index);
    void CollectExportIndexs(const CString &recordName, uint32_t index);
    bool CheckExportNameAndClassType(const CString &recordName, const JSHandle<EcmaString> &exportStr);
    void CollectRecordReferenceREL();
    void CollectRecordImportInfo(const CString &recordName);
    void CollectRecordExportInfo(const CString &recordName);
    void MarkMethodNamespace(const uint32_t methodOffset);

    EcmaVM *vm_;
    JSPandaFile *jsPandaFile_ {nullptr};
    BCInfo bytecodeInfo_;
    PGOProfilerDecoder &pfDecoder_;
    PGOBCInfo pgoBCInfo_ {};
    size_t methodInfoIndex_ {0};
    bool enableCollectLiteralInfo_ {false};
    std::set<int32_t> classDefBCIndexes_ {};
    LexEnvManager* envManager_ {nullptr};
    Bytecodes bytecodes_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H
