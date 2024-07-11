/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/aot_snapshot/snapshot_constantpool_data.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/pgo_bc_info.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/compiler/compilation_env.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::kungfu {
using PGOProfilerDecoder = pgo::PGOProfilerDecoder;

// each method in the abc file corresponds to one MethodInfo and
// methods with the same instructions share one common MethodPcInfo
struct MethodPcInfo {
    std::vector<const uint8_t*> pcOffsets {};
    uint32_t methodsSize {0};
};

class MethodInfo {
public:
    MethodInfo(uint32_t methodInfoIndex, uint32_t methodPcInfoIndex, std::shared_ptr<CString> recordNamePtr)
        : methodInfoIndex_(methodInfoIndex), methodPcInfoIndex_(methodPcInfoIndex), recordNamePtr_(recordNamePtr) {}

    ~MethodInfo() = default;

    static constexpr uint32_t DEFAULT_OUTMETHOD_OFFSET = 0;
    static constexpr uint32_t DEFAULT_ROOT = std::numeric_limits<uint32_t>::max();

    inline uint32_t GetMethodPcInfoIndex() const
    {
        return methodPcInfoIndex_;
    }

    inline void SetMethodPcInfoIndex(uint32_t methodPcInfoIndex)
    {
        methodPcInfoIndex_ = methodPcInfoIndex;
    }

    inline uint32_t GetMethodInfoIndex() const
    {
        return methodInfoIndex_;
    }

    inline void SetMethodInfoIndex(uint32_t methodInfoIndex)
    {
        methodInfoIndex_ = methodInfoIndex;
    }

    inline void SetRecordNamePtr(const std::shared_ptr<CString> recordNamePtr)
    {
        recordNamePtr_ = recordNamePtr;
    }

    inline const std::shared_ptr<CString> GetRecordNamePtr() const
    {
        return recordNamePtr_;
    }

    inline const CString &GetRecordName() const
    {
        return *recordNamePtr_;
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

    private:
        uint8_t value_ {0};
        friend class MethodInfo;
    };
    // used to record the index of the current MethodInfo to speed up the lookup of lexEnv
    uint32_t methodInfoIndex_ { 0 };
    // used to obtain MethodPcInfo from the vector methodPcInfos of struct BCInfo
    uint32_t methodPcInfoIndex_ { 0 };
    std::shared_ptr<CString> recordNamePtr_ {nullptr};
    CompileStateBit compileState_ { 0 };
};

struct FastCallInfo {
    bool canFastCall_ {false};
    bool isNoGC_ {false};
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

    std::vector<std::shared_ptr<CString>>& GetRecordNamePtrs()
    {
        return recordNamePtrs_;
    }

    const CString &GetRecordNameWithIndex(uint32_t index) const
    {
        return *recordNamePtrs_[index];
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

    const std::set<uint32_t>& GetSkippedMethodSet() const
    {
        return skippedMethods_;
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

    bool FindMethodOffsetToRecordName(uint32_t methodOffset)
    {
        return methodOffsetToRecordName_.find(methodOffset) != methodOffsetToRecordName_.end();
    }

    void AddMethodOffsetToRecordName(uint32_t methodOffset, CString recordName)
    {
        methodOffsetToRecordName_.emplace(methodOffset, recordName);
    }

    size_t GetSkippedMethodSize() const
    {
        return skippedMethods_.size();
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
    std::vector<std::shared_ptr<CString>> recordNamePtrs_ {};
    std::vector<MethodPcInfo> methodPcInfos_ {};
    std::unordered_map<uint32_t, MethodInfo> methodList_ {};
    std::unordered_map<uint32_t, CString> methodOffsetToRecordName_ {};
    std::set<uint32_t> skippedMethods_ {};
    size_t maxMethodSize_;
    std::unordered_map<uint32_t, uint32_t> classTypeLOffsetToDefMethod_ {};
    std::unordered_map<uint32_t, uint32_t> functionTypeIdToMethodOffset_ {};
    std::unordered_map<uint32_t, FastCallInfo> methodOffsetToFastCallInfos_ {};
};

class BytecodeInfoCollector {
public:
    BytecodeInfoCollector(CompilationEnv *env, JSPandaFile *jsPandaFile, PGOProfilerDecoder &pfDecoder,
                          size_t maxAotMethodSize);

    BytecodeInfoCollector(CompilationEnv *env, JSPandaFile *jsPandaFile,
                          PGOProfilerDecoder &pfDecoder);

    ~BytecodeInfoCollector() = default;
    NO_COPY_SEMANTIC(BytecodeInfoCollector);
    NO_MOVE_SEMANTIC(BytecodeInfoCollector);

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

    const PGOBCInfo* GetPGOBCInfo() const
    {
        return &pgoBCInfo_;
    }

    void StoreDataToGlobalData(SnapshotGlobalData &snapshotData)
    {
        snapshotCPData_->StoreDataToGlobalData(snapshotData, GetSkippedMethodSet());
    }

    const std::set<uint32_t>& GetSkippedMethodSet() const
    {
        return bytecodeInfo_.GetSkippedMethodSet();
    }

    bool IsSkippedMethod(uint32_t methodOffset) const
    {
        return bytecodeInfo_.IsSkippedMethod(methodOffset);
    }

    bool FilterMethod(const MethodLiteral *methodLiteral, const MethodPcInfo &methodPCInfo) const
    {
        auto recordName = MethodLiteral::GetRecordName(jsPandaFile_, methodLiteral->GetMethodId());
        bool methodSizeIsIllegal = methodPCInfo.methodsSize > bytecodeInfo_.GetMaxMethodSize();
        bool methodFilteredByPGO = !pfDecoder_.Match(jsPandaFile_, recordName, methodLiteral->GetMethodId());
        if (methodSizeIsIllegal || methodFilteredByPGO) {
            return true;
        }
        return false;
    }

    const JSPandaFile *GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

    CompilationEnv *GetCompilationEnv() const
    {
        return compilationEnv_;
    }

    template <class Callback>
    void IterateAllMethods(const Callback &cb)
    {
        auto &methodList = bytecodeInfo_.GetMethodList();
        for (const auto &method : methodList) {
            uint32_t methodOffset = method.first;
            cb(methodOffset);
        }
    }

    void ProcessMethod(MethodLiteral *methodLiteral);
private:
    inline size_t GetNewMethodInfoID()
    {
        return methodInfoCounts_++;
    }

    void ProcessClasses();
    void ProcessCurrMethod();
    void CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr, MethodLiteral *method,
                                uint32_t methodOffset, const std::shared_ptr<CString> recordNamePtr);
    static bool IsVRegUsed(const BytecodeInstruction &inst, const BytecodeMetaData &metaData, uint32_t idx);
    void SetMethodPcInfoIndex(uint32_t methodOffset, const std::pair<size_t, uint32_t> &processedMethodInfo,
                              const std::shared_ptr<CString> recordNamePtr);
    void CollectMethods(const MethodLiteral *method, const std::shared_ptr<CString> recordNamePtr);
    void CollectMethods(uint32_t methodId, const std::shared_ptr<CString> recordNamePtr);
    void CollectInnerMethodsFromLiteral(uint64_t index, const std::shared_ptr<CString> recordNamePtr);
    void CollectInnerMethodsFromNewLiteral(panda_file::File::EntityId literalId,
                                           const std::shared_ptr<CString> recordNamePtr);
    void CollectMethodInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method, int32_t bcIndex,
                                 const std::shared_ptr<CString> recordNamePtr, bool *canFastCall,
                                 bool *canTypedCall);
    void IterateLiteral(const MethodLiteral *method, std::vector<uint32_t> &classOffsetVector);
    void StoreClassTypeOffset(const uint32_t typeOffset, std::vector<uint32_t> &classOffsetVector);
    void CollectClassLiteralInfo(const MethodLiteral *method, const std::vector<std::string> &classNameVec);

    CompilationEnv *compilationEnv_ {nullptr};
    JSPandaFile *jsPandaFile_ {nullptr};
    BCInfo bytecodeInfo_;
    PGOProfilerDecoder &pfDecoder_;
    PGOBCInfo pgoBCInfo_ {};
    std::unique_ptr<SnapshotConstantPoolData> snapshotCPData_;
    size_t methodInfoCounts_ {0};
    std::set<int32_t> classDefBCIndexes_ {};
    Bytecodes bytecodes_;
    std::set<uint32_t> processedMethod_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H
