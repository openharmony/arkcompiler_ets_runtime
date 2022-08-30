/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/panda_file_translator.h"

#include "ecmascript/file_loader.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/ts_types/ts_type_table.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"
#include "libpandabase/utils/utf.h"
#include "libpandafile/bytecode_instruction-inl.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript {
template<class T, class... Args>
static T *InitializeMemory(T *mem, Args... args)
{
    return new (mem) T(std::forward<Args>(args)...);
}

void PandaFileTranslator::TranslateClasses(JSPandaFile *jsPandaFile, const CString &methodName)
{
    ASSERT(jsPandaFile != nullptr && jsPandaFile->GetMethodLiterals() != nullptr);
    MethodLiteral *methodLiterals = jsPandaFile->GetMethodLiterals();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    size_t methodIdx = 0;
    std::set<const uint8_t *> translatedCode;
    Span<const uint32_t> classIndexes = jsPandaFile->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        cda.EnumerateMethods([jsPandaFile, &translatedCode, methodLiterals, &methodIdx, pf, methodName, &desc]
            (panda_file::MethodDataAccessor &mda) {
            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());

            MethodLiteral *methodLiteral = methodLiterals + (methodIdx++);
            panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();

            CString name = reinterpret_cast<const char *>(pf->GetStringData(mda.GetNameId()).data);
            if (jsPandaFile->IsBundle()) {
                if (name == methodName) {
                    jsPandaFile->UpdateMainMethodIndex(mda.GetMethodId().GetOffset());
                }
            } else {
                if (name == JSPandaFile::ENTRY_FUNCTION_NAME) {
                    jsPandaFile->UpdateMainMethodIndex(
                        mda.GetMethodId().GetOffset(), desc.substr(1, desc.size() - 2)); // 2 : skip symbol "L" and ";"
                }
            }

            InitializeMemory(methodLiteral, jsPandaFile, mda.GetMethodId());
            methodLiteral->SetHotnessCounter(EcmaInterpreter::GetHotnessCounter(codeSize));
            methodLiteral->InitializeCallField(
                jsPandaFile, codeDataAccessor.GetNumVregs(), codeDataAccessor.GetNumArgs());

            const uint8_t *insns = codeDataAccessor.GetInstructions();
            if (translatedCode.find(insns) == translatedCode.end()) {
                translatedCode.insert(insns);
                TranslateBytecode(jsPandaFile, codeSize, insns, methodLiteral);
            }
            jsPandaFile->SetMethodLiteralToMap(methodLiteral);
        });
    }
}

JSHandle<Program> PandaFileTranslator::GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                       std::string_view entryPoint)
{
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<Program> program = factory->NewProgram();

    {
        JSThread *thread = vm->GetJSThread();
        EcmaHandleScope handleScope(thread);

        // Parse constpool.
        JSTaggedValue constpool = vm->FindConstpool(jsPandaFile);
        if (constpool.IsHole()) {
            CString entry = "";
            if (!jsPandaFile->IsBundle()) {
                entry = entryPoint.data();
            }
            constpool = ParseConstPool(vm, jsPandaFile, entry);
            vm->SetConstpool(jsPandaFile, constpool);
        }

        // Generate Program.
        uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex(entryPoint.data());
        auto methodLiteral = jsPandaFile->FindMethodLiteral(mainMethodIndex);
        if (methodLiteral == nullptr) {
            program->SetMainFunction(thread, JSTaggedValue::Undefined());
        } else {
            JSHandle<Method> method = factory->NewMethod(methodLiteral);
            JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(vm->GetGlobalEnv()->GetFunctionClassWithProto());
            JSHandle<JSFunction> mainFunc =
                factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::BASE_CONSTRUCTOR);

            program->SetMainFunction(thread, mainFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool);
        }
    }
    return program;
}

JSTaggedValue PandaFileTranslator::ParseConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                  const CString &entryPoint)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    ObjectFactory *factory = vm->GetFactory();
    uint32_t constpoolIndex = jsPandaFile->GetConstpoolIndex();
    JSHandle<ConstantPool> constpool = factory->NewConstantPool(constpoolIndex);

    // Put JSPandaFile at the first index of constpool.
    JSHandle<JSNativePointer> jsPandaFilePointer = factory->NewJSNativePointer(
        const_cast<JSPandaFile *>(jsPandaFile), JSPandaFileManager::RemoveJSPandaFile,
        JSPandaFileManager::GetInstance(), true);
    constpool->Set(thread, 0, jsPandaFilePointer.GetTaggedValue());

    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSHClass> normalDynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSHClass> asyncDynclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    JSHandle<JSHClass> generatorDynclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    JSHandle<JSHClass> asyncGeneratorDynclass = JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClass());

    const CUnorderedMap<uint32_t, uint64_t> &constpoolMap = jsPandaFile->GetConstpoolMap();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    const bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
    auto fileLoader = vm->GetFileLoader();

#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
    if (isLoadedAOT) {
        JSTaggedValue constPoolInfo = vm->GetTSManager()->GetConstantPoolInfo();
        ConstantPoolProcessor::RestoreConstantPoolInfo(vm->GetJSThread(), constPoolInfo, jsPandaFile, constpool);
    }
#endif

    for (const auto &it : constpoolMap) {
        ConstPoolValue value(it.second);
        if (value.GetConstpoolType() == ConstPoolType::STRING) {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
            if (isLoadedAOT) {
                continue;
            }
#endif
            panda_file::File::EntityId id(it.first);
            auto foundStr = pf->GetStringData(id);
            auto string = factory->GetRawStringFromStringTable(foundStr.data, foundStr.utf16_length, foundStr.is_ascii,
                                                               MemSpaceType::OLD_SPACE);
            constpool->Set(thread, value.GetConstpoolIndex(), JSTaggedValue(string));
        } else if (value.GetConstpoolType() == ConstPoolType::BASE_FUNCTION) {
            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(it.first);
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(
                method, dynclass, FunctionKind::BASE_CONSTRUCTOR, MemSpaceType::OLD_SPACE);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntry(jsPandaFile, jsFunc);
            }
            constpool->Set(thread, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::NC_FUNCTION) {
            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(it.first);
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(
                method, normalDynclass, FunctionKind::NORMAL_FUNCTION, MemSpaceType::OLD_SPACE);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntry(jsPandaFile, jsFunc);
            }
            constpool->Set(thread, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::GENERATOR_FUNCTION) {
            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(it.first);
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            JSHandle<JSFunction> jsFunc =
                factory->NewJSFunctionByDynClass(method, generatorDynclass, FunctionKind::GENERATOR_FUNCTION);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntry(jsPandaFile, jsFunc);
            }
            // 26.3.4.3 prototype
            // Whenever a GeneratorFunction instance is created another ordinary object is also created and
            // is the initial value of the generator function's "prototype" property.
            JSHandle<JSFunction> objFun(env->GetObjectFunction());
            JSHandle<JSObject> initialGeneratorFuncPrototype = factory->NewJSObjectByConstructor(objFun);
            JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
            jsFunc->SetProtoOrDynClass(thread, initialGeneratorFuncPrototype);

            constpool->Set(thread, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::ASYNC_GENERATOR_FUNCTION) {
            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(it.first);
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            JSHandle<JSFunction> jsFunc =
                factory->NewJSFunctionByDynClass(method, asyncGeneratorDynclass,
                                                 FunctionKind::ASYNC_GENERATOR_FUNCTION);
            constpool->Set(thread, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::ASYNC_FUNCTION) {
            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(it.first);
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            JSHandle<JSFunction> jsFunc =
                factory->NewJSFunctionByDynClass(method, asyncDynclass, FunctionKind::ASYNC_FUNCTION);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntry(jsPandaFile, jsFunc);
            }
            constpool->Set(thread, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::CLASS_FUNCTION) {
            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(it.first);
            ASSERT(methodLiteral != nullptr);

            JSHandle<Method> method = factory->NewMethod(methodLiteral);
            method->SetConstantPool(thread, constpool.GetTaggedValue());
            constpool->Set(thread, value.GetConstpoolIndex(), method.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::METHOD) {
            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(it.first);
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(
                method, normalDynclass, FunctionKind::NORMAL_FUNCTION, MemSpaceType::OLD_SPACE);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntry(jsPandaFile, jsFunc);
            }
            constpool->Set(thread, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::OBJECT_LITERAL) {
            size_t index = it.first;
            JSMutableHandle<TaggedArray> elements(thread, JSTaggedValue::Undefined());
            JSMutableHandle<TaggedArray> properties(thread, JSTaggedValue::Undefined());
            LiteralDataExtractor::ExtractObjectDatas(
                thread, jsPandaFile, index, elements, properties, JSHandle<JSTaggedValue>(constpool));
            JSHandle<JSObject> obj = JSObject::CreateObjectFromProperties(thread, properties);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntryForLiteral(jsPandaFile, properties);
            }
            JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
            JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
            size_t elementsLen = elements->GetLength();
            for (size_t i = 0; i < elementsLen; i += 2) {  // 2: Each literal buffer contains a pair of key-value.
                key.Update(elements->Get(i));
                if (key->IsHole()) {
                    break;
                }
                valueHandle.Update(elements->Get(i + 1));
                JSObject::DefinePropertyByLiteral(thread, obj, key, valueHandle);
            }
            constpool->Set(thread, value.GetConstpoolIndex(), obj.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::ARRAY_LITERAL) {
            size_t index = it.first;
            JSHandle<TaggedArray> literal =LiteralDataExtractor::GetDatasIgnoreType(
                thread, jsPandaFile, static_cast<size_t>(index), JSHandle<JSTaggedValue>(constpool), entryPoint);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntryForLiteral(jsPandaFile, literal);
            }
            uint32_t length = literal->GetLength();

            JSHandle<JSArray> arr(JSArray::ArrayCreate(thread, JSTaggedNumber(length)));
            arr->SetElements(thread, literal);
            constpool->Set(thread, value.GetConstpoolIndex(), arr.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::CLASS_LITERAL) {
            size_t index = it.first;
            JSHandle<TaggedArray> literal = LiteralDataExtractor::GetDatasIgnoreType(
                thread, jsPandaFile, static_cast<size_t>(index), JSHandle<JSTaggedValue>(constpool), entryPoint);
            if (isLoadedAOT) {
                fileLoader->SetAOTFuncEntryForLiteral(jsPandaFile, literal);
            }
            constpool->Set(thread, value.GetConstpoolIndex(), literal.GetTaggedValue());
        }
    }

    return constpool.GetTaggedValue();
}

void PandaFileTranslator::FixOpcode(uint8_t *pc)
{
    auto opcode = static_cast<BytecodeInstruction::Opcode>(*pc);

    switch (opcode) {
        case BytecodeInstruction::Opcode::MOV_V4_V4:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_V4_V4);
            break;
        case BytecodeInstruction::Opcode::MOV_DYN_V8_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_DYN_V8_V8);
            break;
        case BytecodeInstruction::Opcode::MOV_DYN_V16_V16:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_DYN_V16_V16);
            break;
        case BytecodeInstruction::Opcode::LDA_STR_ID32:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDA_STR_ID32);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM8);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM16);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM32:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM32);
            break;
        case BytecodeInstruction::Opcode::JEQZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM8);
            break;
        case BytecodeInstruction::Opcode::JEQZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM16);
            break;
        case BytecodeInstruction::Opcode::JNEZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM8);
            break;
        case BytecodeInstruction::Opcode::JNEZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM16);
            break;
        case BytecodeInstruction::Opcode::LDA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDA_DYN_V8);
            break;
        case BytecodeInstruction::Opcode::STA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::STA_DYN_V8);
            break;
        case BytecodeInstruction::Opcode::LDAI_DYN_IMM32:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDAI_DYN_IMM32);
            break;
        case BytecodeInstruction::Opcode::FLDAI_DYN_IMM64:
            *pc = static_cast<uint8_t>(EcmaOpcode::FLDAI_DYN_IMM64);
            break;
        case BytecodeInstruction::Opcode::RETURN_DYN:
            *pc = static_cast<uint8_t>(EcmaOpcode::RETURN_DYN);
            break;
        default:
            if (*pc != static_cast<uint8_t>(BytecodeInstruction::Opcode::ECMA_LDNAN_PREF_NONE)) {
                LOG_FULL(FATAL) << "Is not an Ecma Opcode opcode: " << static_cast<uint16_t>(opcode);
                UNREACHABLE();
            }
            *pc = *(pc + 1);
            *(pc + 1) = 0xFF;
            break;
    }
}

// reuse prefix 8bits to store slotid
void PandaFileTranslator::UpdateICOffset(MethodLiteral *methodLiteral, uint8_t *pc)
{
    uint8_t offset = MethodLiteral::MAX_SLOT_SIZE;
    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::TRYLDGLOBALBYNAME_PREF_ID32:
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32:
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32:
        case EcmaOpcode::STGLOBALVAR_PREF_ID32:
        case EcmaOpcode::ADD2DYN_PREF_V8:
        case EcmaOpcode::SUB2DYN_PREF_V8:
        case EcmaOpcode::MUL2DYN_PREF_V8:
        case EcmaOpcode::DIV2DYN_PREF_V8:
        case EcmaOpcode::MOD2DYN_PREF_V8:
        case EcmaOpcode::SHL2DYN_PREF_V8:
        case EcmaOpcode::SHR2DYN_PREF_V8:
        case EcmaOpcode::ASHR2DYN_PREF_V8:
        case EcmaOpcode::AND2DYN_PREF_V8:
        case EcmaOpcode::OR2DYN_PREF_V8:
        case EcmaOpcode::XOR2DYN_PREF_V8:
        case EcmaOpcode::EQDYN_PREF_V8:
        case EcmaOpcode::NOTEQDYN_PREF_V8:
        case EcmaOpcode::LESSDYN_PREF_V8:
        case EcmaOpcode::LESSEQDYN_PREF_V8:
        case EcmaOpcode::GREATERDYN_PREF_V8:
        case EcmaOpcode::GREATEREQDYN_PREF_V8:
            offset = methodLiteral->UpdateSlotSize(1);
            break;
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STOWNBYVALUE_PREF_V8_V8:
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STOBJBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STOWNBYNAME_PREF_ID32_V8:
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::STOBJBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::STOWNBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::LDSUPERBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STSUPERBYVALUE_PREF_V8_V8:
        case EcmaOpcode::LDSUPERBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STSUPERBYNAME_PREF_ID32_V8:
        case EcmaOpcode::LDMODULEVAR_PREF_ID32_IMM8:
        case EcmaOpcode::STMODULEVAR_PREF_ID32:
            offset = methodLiteral->UpdateSlotSize(2); // 2: occupy two ic slot
            break;
        default:
            return;
    }

    *(pc + 1) = offset;
}

void PandaFileTranslator::FixInstructionId32(const BytecodeInstruction &inst, uint32_t index, uint32_t fixOrder)
{
    // NOLINTNEXTLINE(hicpp-use-auto)
    auto pc = const_cast<uint8_t *>(inst.GetAddress());
    switch (inst.GetFormat()) {
        case BytecodeInstruction::Format::ID32: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_ONE, size, &index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID16_IMM16_V8: {
            uint16_t u16Index = index;
            uint8_t size = sizeof(uint16_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &u16Index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID32:
        case BytecodeInstruction::Format::PREF_ID32_V8:
        case BytecodeInstruction::Format::PREF_ID32_IMM8: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_IMM16: {
            ASSERT(static_cast<uint16_t>(index) == index);
            uint16_t u16Index = index;
            uint8_t size = sizeof(uint16_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &u16Index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8: {
            // Usually, we fix one part of instruction one time. But as for instruction DefineClassWithBuffer,
            // which use both method id and literal buffer id.Using fixOrder indicates fix Location.
            if (fixOrder == 0) {
                uint8_t size = sizeof(uint16_t);
                ASSERT(static_cast<uint16_t>(index) == index);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &index, size) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed";
                    UNREACHABLE();
                }
                break;
            }
            if (fixOrder == 1) {
                ASSERT(static_cast<uint16_t>(index) == index);
                uint16_t u16Index = index;
                uint8_t size = sizeof(uint16_t);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (memcpy_s(pc + FixInstructionIndex::FIX_FOUR, size, &u16Index, size) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed";
                    UNREACHABLE();
                }
                break;
            }
            break;
        }
        default:
            UNREACHABLE();
    }
}

void PandaFileTranslator::TranslateBytecode(JSPandaFile *jsPandaFile, uint32_t insSz, const uint8_t *insArr,
                                            const MethodLiteral *method)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    auto bcIns = BytecodeInstruction(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        if (bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) &&
            BytecodeInstruction::HasId(bcIns.GetFormat(), 0)) {
            auto index = jsPandaFile->GetOrInsertConstantPool(
                ConstPoolType::STRING, bcIns.GetId().AsFileId().GetOffset());
            FixInstructionId32(bcIns, index);
        } else {
            BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
            switch (opcode) {
                uint32_t index;
                uint32_t methodId;
                case BytecodeInstruction::Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
                    methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::BASE_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
                    methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::NC_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
                    methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::GENERATOR_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINEASYNCGENERATORFUNC_PREF_ID16_IMM16_V8:
                    methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::ASYNC_GENERATOR_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
                    methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::ASYNC_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8:
                    methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16:
                case BytecodeInstruction::Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
                    auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::OBJECT_LITERAL,
                        static_cast<uint16_t>(imm));
                    FixInstructionId32(bcIns, index);
                    break;
                }
                case BytecodeInstruction::Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16: {
                    auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::ARRAY_LITERAL,
                        static_cast<uint16_t>(imm));
                    FixInstructionId32(bcIns, index);
                    break;
                }
                case BytecodeInstruction::Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
                    methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::CLASS_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8>();
                    index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::CLASS_LITERAL,
                        static_cast<uint16_t>(imm));
                    FixInstructionId32(bcIns, index, 1);
                    break;
                }
                default:
                    break;
            }
        }
        // NOLINTNEXTLINE(hicpp-use-auto)
        auto pc = const_cast<uint8_t *>(bcIns.GetAddress());
        bcIns = bcIns.GetNext();
        FixOpcode(pc);
        UpdateICOffset(const_cast<MethodLiteral *>(method), pc);
    }
}
}  // namespace panda::ecmascript
