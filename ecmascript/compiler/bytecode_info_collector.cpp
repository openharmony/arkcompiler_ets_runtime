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

#include "ecmascript/compiler/bytecode_info_collector.h"

#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "libpandafile/code_data_accessor.h"
#include "libpandafile/class_data_accessor-inl.h"
#include "libpandafile/index_accessor.h"

namespace panda::ecmascript::kungfu {
template<class T, class... Args>
static T *InitializeMemory(T *mem, Args... args)
{
    return new (mem) T(std::forward<Args>(args)...);
}

BytecodeInfoCollector::BytecodeInfoCollector(CompilationEnv *env, JSPandaFile *jsPandaFile,
                                             PGOProfilerDecoder &pfDecoder,
                                             size_t maxAotMethodSize)
    : compilationEnv_(env),
      jsPandaFile_(jsPandaFile),
      bytecodeInfo_(maxAotMethodSize),
      pfDecoder_(pfDecoder),
      snapshotCPData_(new SnapshotConstantPoolData(env->GetEcmaVM(), jsPandaFile, &pfDecoder))
{
    ASSERT(env->IsAotCompiler());
    ProcessClasses();
}

BytecodeInfoCollector::BytecodeInfoCollector(CompilationEnv *env, JSPandaFile *jsPandaFile,
                                             PGOProfilerDecoder &pfDecoder)
    : compilationEnv_(env),
      jsPandaFile_(jsPandaFile),
      // refactor: jit max method size
      bytecodeInfo_(env->GetJSOptions().GetMaxAotMethodSize()),
      pfDecoder_(pfDecoder),
      snapshotCPData_(nullptr) // jit no need
{
    ASSERT(env->IsJitCompiler());
    ProcessCurrMethod();
}

void BytecodeInfoCollector::ProcessClasses()
{
    ASSERT(jsPandaFile_ != nullptr && jsPandaFile_->GetMethodLiterals() != nullptr);
    MethodLiteral *methods = jsPandaFile_->GetMethodLiterals();
    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    size_t methodIdx = 0;
    std::map<uint32_t, std::pair<size_t, uint32_t>> processedMethod;
    Span<const uint32_t> classIndexes = jsPandaFile_->GetClasses();

    auto &recordNamePtrs = bytecodeInfo_.GetRecordNamePtrs();
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (jsPandaFile_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        const std::shared_ptr<CString> recordNamePtr = std::make_shared<CString>(JSPandaFile::ParseEntryPoint(desc));
        cda.EnumerateMethods([this, methods, &methodIdx, pf, &processedMethod,
            &recordNamePtrs, &methodPcInfos, &recordNamePtr] (panda_file::MethodDataAccessor &mda) {
            auto methodId = mda.GetMethodId();

            // Generate all constpool
            compilationEnv_->FindOrCreateConstPool(jsPandaFile_, methodId);

            auto methodOffset = methodId.GetOffset();
            CString name = reinterpret_cast<const char *>(jsPandaFile_->GetStringData(mda.GetNameId()).data);
            if (JSPandaFile::IsEntryOrPatch(name)) {
                jsPandaFile_->UpdateMainMethodIndex(methodOffset, *recordNamePtr);
                recordNamePtrs.emplace_back(recordNamePtr);
            }

            MethodLiteral *methodLiteral = methods + (methodIdx++);
            InitializeMemory(methodLiteral, methodId);
            methodLiteral->Initialize(jsPandaFile_);

            ASSERT(jsPandaFile_->IsNewVersion());
            panda_file::IndexAccessor indexAccessor(*pf, methodId);
            panda_file::FunctionKind funcKind = indexAccessor.GetFunctionKind();
            bool isShared = JSPandaFile::IsSendableFunctionKind(funcKind);
            methodLiteral->SetIsShared(isShared);
            FunctionKind kind = JSPandaFile::GetFunctionKind(funcKind);
            methodLiteral->SetFunctionKind(kind);

            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());
            panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();
            const uint8_t *insns = codeDataAccessor.GetInstructions();
            auto it = processedMethod.find(methodOffset);
            if (it == processedMethod.end()) {
                CollectMethodPcsFromBC(codeSize, insns, methodLiteral,
                    methodOffset, recordNamePtr);
                ASSERT(methodPcInfos.size() > 0);
                processedMethod[methodOffset] = std::make_pair(methodPcInfos.size() - 1, methodOffset);
            }

            SetMethodPcInfoIndex(methodOffset, processedMethod[methodOffset], recordNamePtr);
            jsPandaFile_->SetMethodLiteralToMap(methodLiteral);
            pfDecoder_.MatchAndMarkMethod(jsPandaFile_, *recordNamePtr, name.c_str(), methodId);
        });
    }
    LOG_COMPILER(INFO) << "total number of methods in file " << jsPandaFile_->GetJSPandaFileDesc() << " is "
                       << methodIdx;
}

void BytecodeInfoCollector::ProcessCurrMethod()
{
    ProcessMethod(compilationEnv_->GetMethodLiteral());
}

void BytecodeInfoCollector::ProcessMethod(MethodLiteral *methodLiteral)
{
    if (UNLIKELY(methodLiteral == nullptr)) {
        return;
    }
    panda_file::File::EntityId methodIdx = methodLiteral->GetMethodId();
    auto methodOffset = methodIdx.GetOffset();
    if (processedMethod_.find(methodOffset) != processedMethod_.end()) {
        return;
    }

    auto &recordNamePtrs = bytecodeInfo_.GetRecordNamePtrs();
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    const std::shared_ptr<CString> recordNamePtr =
        std::make_shared<CString>(jsPandaFile_->GetRecordNameWithBundlePack(methodIdx));
    recordNamePtrs.emplace_back(recordNamePtr);
    ASSERT(jsPandaFile_->IsNewVersion());

    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, methodIdx);
    auto codeId = mda.GetCodeId();
    ASSERT(codeId.has_value());
    panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
    uint32_t codeSize = codeDataAccessor.GetCodeSize();
    const uint8_t *insns = codeDataAccessor.GetInstructions();

    CollectMethodPcsFromBC(codeSize, insns, methodLiteral, methodOffset, recordNamePtr);
    ASSERT(methodPcInfos.size() > 0);
    SetMethodPcInfoIndex(methodOffset, {methodPcInfos.size() - 1, methodOffset}, recordNamePtr);
    processedMethod_.emplace(methodOffset);
}

void BytecodeInfoCollector::CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr,
                                                   MethodLiteral *method, uint32_t methodOffset,
                                                   const std::shared_ptr<CString> recordNamePtr)
{
    auto bcIns = BytecodeInst(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);
    int32_t bcIndex = 0;
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    methodPcInfos.emplace_back(MethodPcInfo { {}, insSz });
    auto &pcOffsets = methodPcInfos.back().pcOffsets;
    const uint8_t *curPc = bcIns.GetAddress();
    bool canFastCall = true;
    bool noGC = true;
    bool debuggerStmt = false;
    uint32_t newtargetIndex = method->GetNewTargetVregIndex();
    bool canTypedCall = true;

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        curPc = bcIns.GetAddress();
        auto metaData = bytecodes_.GetBytecodeMetaData(curPc);
        bool opcodeSupprotFastCall = true;
        bool opcodeSupportTypeByteCall = true;
        CollectMethodInfoFromBC(bcIns, method, bcIndex, recordNamePtr,
                                &opcodeSupprotFastCall, &opcodeSupportTypeByteCall);
        bool vregSupportFastCall = !IsVRegUsed(bcIns, metaData, newtargetIndex);
        if (!opcodeSupprotFastCall || !vregSupportFastCall) {
            canFastCall = false;
        }
        if (!opcodeSupportTypeByteCall) {
            canTypedCall = false;
        }
        if (snapshotCPData_ != nullptr) {
            snapshotCPData_->Record(bcIns, bcIndex, *recordNamePtr, method);
        }
        pgoBCInfo_.Record(bcIns, bcIndex, *recordNamePtr, method);
        if (noGC && !metaData.IsNoGC()) {
            noGC = false;
        }
        if (!debuggerStmt && metaData.HasDebuggerStmt()) {
            debuggerStmt = true;
        }
        auto nextInst = bcIns.GetNext();
        bcIns = nextInst;
        pcOffsets.emplace_back(curPc);
        bcIndex++;
    }
    bytecodeInfo_.SetMethodOffsetToFastCallInfo(methodOffset, canFastCall, noGC);
    method->SetIsFastCall(canFastCall);
    method->SetNoGCBit(noGC);
    method->SetHasDebuggerStmtBit(debuggerStmt);
    method->SetCanTypedCall(canTypedCall);
}

// static
bool BytecodeInfoCollector::IsVRegUsed(const BytecodeInstruction &inst, const BytecodeMetaData &metaData, uint32_t idx)
{
    if (idx == 0) {
        return false;
    }
    uint32_t vregCount = metaData.GetVRegCount();
    for (uint32_t i = 0; i < vregCount; i++) {
        ASSERT(inst.HasVReg(inst.GetFormat(), i));
        uint16_t vregIdx = inst.GetVReg(i);
        if (vregIdx == idx) {
            return true;
        }
    }
    return false;
}

void BytecodeInfoCollector::SetMethodPcInfoIndex(uint32_t methodOffset,
                                                 const std::pair<size_t, uint32_t> &processedMethodInfo,
                                                 const std::shared_ptr<CString> recordNamePtr)
{
    auto processedMethodPcInfoIndex = processedMethodInfo.first;
    auto &methodList = bytecodeInfo_.GetMethodList();

    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        MethodInfo &methodInfo = iter->second;
        methodInfo.SetMethodPcInfoIndex(processedMethodPcInfoIndex);
        return;
    }
    MethodInfo info(GetNewMethodInfoID(), processedMethodPcInfoIndex, recordNamePtr);
    methodList.emplace(methodOffset, info);
}

void BytecodeInfoCollector::CollectMethods(const MethodLiteral *method, const std::shared_ptr<CString> recordNamePtr)
{
    auto methodId = method->GetMethodId().GetOffset();
    CollectMethods(methodId, recordNamePtr);
}

void BytecodeInfoCollector::CollectMethods(uint32_t methodId, const std::shared_ptr<CString> recordNamePtr)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    if (methodList.find(methodId) == methodList.end()) {
        methodList.emplace(methodId, MethodInfo(GetNewMethodInfoID(), 0, recordNamePtr));
    }
}

void BytecodeInfoCollector::CollectInnerMethodsFromLiteral(uint64_t index, const std::shared_ptr<CString> recordNamePtr)
{
    std::vector<uint32_t> methodOffsets;
    LiteralDataExtractor::GetMethodOffsets(jsPandaFile_, index, methodOffsets);
    for (auto methodOffset : methodOffsets) {
        CollectMethods(methodOffset, recordNamePtr);
    }
}

void BytecodeInfoCollector::CollectInnerMethodsFromNewLiteral(panda_file::File::EntityId literalId,
                                                              const std::shared_ptr<CString> recordNamePtr)
{
    std::vector<uint32_t> methodOffsets;
    LiteralDataExtractor::GetMethodOffsets(jsPandaFile_, literalId, methodOffsets);
    for (auto methodOffset : methodOffsets) {
        CollectMethods(methodOffset, recordNamePtr);
    }
}

void BytecodeInfoCollector::CollectMethodInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method,
                                                    int32_t bcIndex, const std::shared_ptr<CString> recordNamePtr,
                                                    bool *canFastCall, bool *canTypedCall)
{
    if (!(bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) &&
        BytecodeInstruction::HasId(BytecodeInstruction::GetFormat(bcIns.GetOpcode()), 0))) {
        CollectMethods(method, recordNamePtr);
        BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
        switch (opcode) {
            uint32_t innerMethodId;
            case BytecodeInstruction::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
            case BytecodeInstruction::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
            case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
            case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
                innerMethodId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    static_cast<uint16_t>(bcIns.GetId().AsRawValue())).GetOffset();
                CollectMethods(innerMethodId, recordNamePtr);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:{
                auto entityId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                classDefBCIndexes_.insert(bcIndex);
                innerMethodId = entityId.GetOffset();
                CollectMethods(innerMethodId, recordNamePtr);
                auto literalId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                CollectInnerMethodsFromNewLiteral(literalId, recordNamePtr);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
                auto entityId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                classDefBCIndexes_.insert(bcIndex);
                innerMethodId = entityId.GetOffset();
                CollectMethods(innerMethodId, recordNamePtr);
                auto literalId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                CollectInnerMethodsFromNewLiteral(literalId, recordNamePtr);
                break;
            }
            case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
            case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
            case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
                auto literalId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    static_cast<uint16_t>(bcIns.GetId().AsRawValue()));
                CollectInnerMethodsFromNewLiteral(literalId, recordNamePtr);
                break;
            }
            case BytecodeInstruction::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16:
            case BytecodeInstruction::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                CollectInnerMethodsFromLiteral(imm, recordNamePtr);
                break;
            }
            case EcmaOpcode::RESUMEGENERATOR:
            case EcmaOpcode::SUSPENDGENERATOR_V8:
            case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8: {
                *canFastCall = false;
                break;
            }
            case EcmaOpcode::CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8:
            case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
            case EcmaOpcode::GETUNMAPPEDARGS:
            case EcmaOpcode::COPYRESTARGS_IMM8:
            case EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16: {
                *canFastCall = false;
                *canTypedCall = false;
                break;
            }
            default:
                break;
        }
    }
}
}  // namespace panda::ecmascript::kungfu
