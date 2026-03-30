/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <filesystem>
#include <string_view>

#include "ecmascript/js_object-inl.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_snapshot.h"
#include "ecmascript/module/module_value_accessor.h"
#include "ecmascript/object_fast_operator-inl.h"
#define private public
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#undef private
#include "ecmascript/tests/test_helper.h"

constexpr std::string_view TEST_ROM_VERSION = "version 205.0.1.120(SP20)";

using namespace panda::ecmascript;
namespace panda::test {
class MockModuleSnapshot : public ModuleSnapshot {
public:
    static bool SerializeDataAndSaving(const EcmaVM *vm, const CString &path, const CString &version)
    {
        CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
        if (FileExist(filePath.c_str())) {
            LOG_ECMA(INFO) << "Module serialize file already exist";
            return false;
        }
        JSThread *thread = vm->GetJSThread();
        std::unique_ptr<SerializeData> serializeData = GetSerializeData(thread);
        if (serializeData == nullptr) {
            return false;
        }
        return WriteDataToFile(thread, serializeData, filePath, version);
    }
    static bool MockSerializeAndSavingBufferEmpty(const EcmaVM *vm, const CString &path, const CString &version)
    {
        CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
        JSThread *thread = vm->GetJSThread();
        std::unique_ptr<SerializeData> serializeData = GetSerializeData(thread);
        serializeData->SetBuffer(nullptr);
        if (serializeData == nullptr) {
            return false;
        }
        return WriteDataToFile(thread, serializeData, filePath, version);
    }
    static bool MockSerializeAndSavingHasIncompleteData(const EcmaVM *vm, const CString &path, const CString &version)
    {
        CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
        JSThread *thread = vm->GetJSThread();
        std::unique_ptr<SerializeData> serializeData = GetSerializeData(thread);
        serializeData->SetIncompleteData(true);
        if (serializeData == nullptr) {
            return false;
        }
        return WriteDataToFile(thread, serializeData, filePath, version);
    }
    static bool MockWriteDataToFile(JSThread *thread, const std::unique_ptr<SerializeData>& data,
        const CString& filePath, const CString &version)
    {
        return WriteDataToFile(thread, data, filePath, version);
    }
    static bool MockReadDataFromFile(JSThread *thread, std::unique_ptr<SerializeData>& data,
        const CString& path, const CString &version)
    {
        return ReadDataFromFile(thread, data, path, version);
    }
    static JSHandle<TaggedArray> MockGetModuleSerializeArray(JSThread *thread)
    {
        return GetModuleSerializeArray(thread);
    }
    static void MockRestoreUpdatedBinding(JSThread *thread, JSHandle<TaggedArray> serializeArray)
    {
        RestoreUpdatedBinding(thread, serializeArray);
    }
    static std::unique_ptr<SerializeData> MockGetSerializeData(JSThread *thread)
    {
        return GetSerializeData(thread);
    }
    static size_t MockGetAlignUpPadding(const uint8_t *curPtr, void *originAddr, const size_t alignment)
    {
        return GetAlignUpPadding(curPtr, originAddr, alignment);
    }
    static bool MockSerializeAndSavingWithZeroBuffer(const EcmaVM *vm, const CString &path, const CString &version)
    {
        CString filePath = base::ConcatToCString(path, MODULE_SNAPSHOT_FILE_NAME);
        JSThread *thread = vm->GetJSThread();
        std::unique_ptr<SerializeData> serializeData = std::make_unique<SerializeData>(thread);
        if (serializeData == nullptr) {
            return false;
        }
        return WriteDataToFile(thread, serializeData, filePath, version);
    }
};
class ModuleSnapshotTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        CString path = GetSnapshotPath();
        CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
        if (remove(fileName.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed when setup";
        }
        CString stateFilePath = path + ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME.data();
        if (FileExist(stateFilePath.c_str())) {
            if (remove(stateFilePath.c_str()) != 0) {
                GTEST_LOG_(ERROR) << "remove '" << stateFilePath << "' failed when setup";
            }
        }
        disabledFeature = ModulesSnapshotHelper::g_featureState_;
        loadedFeature = ModulesSnapshotHelper::g_featureLoaded_;
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        CString path = GetSnapshotPath();
        CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
        if (remove(fileName.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed when teardown";
        }
        ModulesSnapshotHelper::g_featureState_ = disabledFeature;
        ModulesSnapshotHelper::g_featureLoaded_ = loadedFeature;
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    static CString GetSnapshotPath()
    {
        char buff[FILENAME_MAX];
        getcwd(buff, FILENAME_MAX);
        CString currentPath(buff);
        if (currentPath.back() != '/') {
            currentPath += "/";
        }
        return currentPath;
    }

    void InitEntries(JSHandle<SourceTextModule> module, JSHandle<SourceTextModule> jsonModule) const
    {
        ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
        JSHandle<JSTaggedValue> config = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("config"));
        size_t importEntryArrayLen = 3;
        JSHandle<JSTaggedValue> importName = val;
        JSHandle<JSTaggedValue> localName = val;
        JSHandle<ImportEntry> importEntry1 =
            objectFactory->NewImportEntry(index0, importName, localName, SharedTypes::UNSENDABLE_MODULE);
        SourceTextModule::AddImportEntry(thread, module, importEntry1, index0, importEntryArrayLen);
        JSHandle<JSTaggedValue> starString = thread->GlobalConstants()->GetHandledStarString();
        JSHandle<ImportEntry> importEntry2 =
            objectFactory->NewImportEntry(index1, starString, localName, SharedTypes::UNSENDABLE_MODULE);
        SourceTextModule::AddImportEntry(thread, module, importEntry2, index1, importEntryArrayLen);
        JSHandle<ImportEntry> importEntry3 =
            objectFactory->NewImportEntry(index2, config, config, SharedTypes::UNSENDABLE_MODULE);
        SourceTextModule::AddImportEntry(thread, module, importEntry3, index2, importEntryArrayLen);
        size_t localExportEntryLen = 1;
        JSHandle<LocalExportEntry> localExportEntry =
            objectFactory->NewLocalExportEntry(val, val, index0, SharedTypes::UNSENDABLE_MODULE);
        JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(localExportEntryLen);
        localExportEntries->Set(thread, index0, localExportEntry);
        SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry, index0, localExportEntryLen);

        size_t indirectExportEntryLen = 1;
        JSHandle<IndirectExportEntry> indirectExportEntry =
            objectFactory->NewIndirectExportEntry(val, index0, val, SharedTypes::UNSENDABLE_MODULE);
        JSHandle<TaggedArray> indirectExportEntries = objectFactory->NewTaggedArray(indirectExportEntryLen);
        indirectExportEntries->Set(thread, index0, indirectExportEntry);
        module->SetIndirectExportEntries(thread, indirectExportEntries);

        JSHandle<JSTaggedValue> defaultString = thread->GlobalConstants()->GetHandledDefaultString();
        JSHandle<LocalExportEntry> jsonLocalExportEntry =
            objectFactory->NewLocalExportEntry(defaultString, defaultString, index0, SharedTypes::UNSENDABLE_MODULE);
        JSHandle<TaggedArray> jsonLocalExportEntries = objectFactory->NewTaggedArray(localExportEntryLen);
        jsonLocalExportEntries->Set(thread, index0, jsonLocalExportEntry);
        SourceTextModule::AddLocalExportEntry(thread, jsonModule, jsonLocalExportEntry, index0, localExportEntryLen);
        JSHandle<JSTaggedValue> jsonVal = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("jsonVal"));
        SourceTextModule::StoreModuleValue(thread, jsonModule, index0, jsonVal);
    }

    void InitEnv(JSHandle<SourceTextModule> module, JSHandle<SourceTextModule> bindingModule) const
    {
        ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
        size_t environmentArraySize = 4;
        JSHandle<TaggedArray> environmentArray = objectFactory->NewTaggedArray(environmentArraySize);
        // sendable binding
        JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8("sendable binding recordName");
        JSHandle<EcmaString> baseFileNameHdl = objectFactory->NewFromUtf8("sendable binding baseFileNameHdl");
        JSHandle<ResolvedRecordIndexBinding> recordIndexBinding =
            objectFactory->NewSResolvedRecordIndexBindingRecord(recordNameHdl, baseFileNameHdl, index0);
        environmentArray->Set(thread, index0, recordIndexBinding.GetTaggedValue());

        JSHandle<JSTaggedValue> val2 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val2"));
        JSHandle<ResolvedRecordBinding> nameBinding =
            objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val2);
        environmentArray->Set(thread, index1, nameBinding.GetTaggedValue());
        // mormal binding
        JSHandle<ResolvedBinding> resolvedBinding = objectFactory->NewResolvedBindingRecord(bindingModule, val2);
        environmentArray->Set(thread, index2, resolvedBinding.GetTaggedValue());
        JSHandle<ResolvedIndexBinding> resolvedIndexBinding =
            objectFactory->NewResolvedIndexBindingRecord(bindingModule, index0);
        environmentArray->Set(thread, index3, resolvedIndexBinding.GetTaggedValue());
        module->SetEnvironment(thread, environmentArray);
    }

    void InitMockSourceTextModule() const
    {
        auto vm = thread->GetEcmaVM();
        ObjectFactory *objectFactory = vm->GetFactory();
        JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
        CString baseFileName = "modules.abc";
        CString recordName = "a";
        module->SetEcmaModuleFilenameString(baseFileName);
        module->SetEcmaModuleRecordNameString(recordName);
        module->SetTypes(ModuleTypes::ECMA_MODULE);
        module->SetStatus(ModuleStatus::INSTANTIATED);
        JSHandle<TaggedArray> requestedModules = objectFactory->NewTaggedArray(1);
        module->SetRequestedModules(thread, requestedModules.GetTaggedValue());
        JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
        CString recordName1 = "b";
        module1->SetEcmaModuleFilenameString(baseFileName);
        module1->SetEcmaModuleRecordNameString(recordName1);
        requestedModules->Set(thread, index0, module1);
        JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
        CString recordName2 = "c";
        module2->SetEcmaModuleFilenameString(baseFileName);
        module2->SetEcmaModuleRecordNameString(recordName2);
        module2->SetStatus(ModuleStatus::EVALUATED);
        JSHandle<SourceTextModule> module3 = objectFactory->NewSourceTextModule();
        CString recordName3 = "d";
        module3->SetEcmaModuleFilenameString(baseFileName);
        module3->SetEcmaModuleRecordNameString(recordName3);
        module3->SetStatus(ModuleStatus::EVALUATED);
        module3->SetTypes(ModuleTypes::JSON_MODULE);
        InitEntries(module, module3);
        InitEnv(module, module2);

        thread->GetModuleManager()->AddResolveImportedModule(recordName, module.GetTaggedValue());
        thread->GetModuleManager()->AddResolveImportedModule(recordName1, module1.GetTaggedValue());
        thread->GetModuleManager()->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
        thread->GetModuleManager()->AddResolveImportedModule(recordName3, module3.GetTaggedValue());
    }

    void CheckEntries(JSHandle<SourceTextModule> serializeModule, JSHandle<SourceTextModule> deserializeModule) const
    {
        // check import entry
        if (!serializeModule->GetImportEntries(thread).IsUndefined() &&
            !deserializeModule->GetImportEntries(thread).IsUndefined()) {
            JSHandle<TaggedArray> serializeImportArray(thread, serializeModule->GetImportEntries(thread));
            JSHandle<TaggedArray> deserializeImportArray(thread, deserializeModule->GetImportEntries(thread));
            ASSERT_EQ(serializeImportArray->GetLength(), deserializeImportArray->GetLength());
            for (size_t i = 0; i < serializeImportArray->GetLength(); i++) {
                JSHandle<ImportEntry> serializeEntry(thread, serializeImportArray->Get(thread, i));
                JSHandle<ImportEntry> deserializeEntry(thread, deserializeImportArray->Get(thread, i));
                EXPECT_EQ(serializeEntry->GetModuleRequestIndex(), deserializeEntry->GetModuleRequestIndex());
                EXPECT_EQ(serializeEntry->GetImportName(thread), deserializeEntry->GetImportName(thread));
                EXPECT_EQ(serializeEntry->GetLocalName(thread), deserializeEntry->GetLocalName(thread));
            }
        }

        // check local export entry
        if (!serializeModule->GetLocalExportEntries(thread).IsUndefined() &&
            !deserializeModule->GetLocalExportEntries(thread).IsUndefined()) {
            JSHandle<TaggedArray> serializeEntries(thread, serializeModule->GetLocalExportEntries(thread));
            JSHandle<TaggedArray> deserializeEntries(thread, deserializeModule->GetLocalExportEntries(thread));
            ASSERT_EQ(serializeEntries->GetLength(), deserializeEntries->GetLength());
            for (size_t i = 0; i < serializeEntries->GetLength(); i++) {
                JSHandle<LocalExportEntry> serializeEntry(thread, serializeEntries->Get(thread, i));
                JSHandle<LocalExportEntry> deserializeEntry(thread, deserializeEntries->Get(thread, i));
                EXPECT_EQ(serializeEntry->GetLocalIndex(), deserializeEntry->GetLocalIndex());
                EXPECT_EQ(serializeEntry->GetExportName(thread), deserializeEntry->GetExportName(thread));
                EXPECT_EQ(serializeEntry->GetLocalName(thread), deserializeEntry->GetLocalName(thread));
            }
        }
        // check indirect export entry
        if (!serializeModule->GetIndirectExportEntries(thread).IsUndefined() &&
            !deserializeModule->GetIndirectExportEntries(thread).IsUndefined()) {
            JSHandle<TaggedArray> serializeEntries(thread, serializeModule->GetIndirectExportEntries(thread));
            JSHandle<TaggedArray> deserializeEntries(thread, deserializeModule->GetIndirectExportEntries(thread));
            ASSERT_EQ(serializeEntries->GetLength(), deserializeEntries->GetLength());
            for (size_t i = 0; i < serializeEntries->GetLength(); i++) {
                JSHandle<IndirectExportEntry> serializeEntry(thread, serializeEntries->Get(thread, i));
                JSHandle<IndirectExportEntry> deserializeEntry(thread, deserializeEntries->Get(thread, i));
                EXPECT_EQ(serializeEntry->GetModuleRequestIndex(), deserializeEntry->GetModuleRequestIndex());
                EXPECT_EQ(serializeEntry->GetExportName(thread), deserializeEntry->GetExportName(thread));
                EXPECT_EQ(serializeEntry->GetImportName(thread), deserializeEntry->GetImportName(thread));
            }
        }
    }

    void CheckEnv(JSHandle<SourceTextModule> serializeModule, JSHandle<SourceTextModule> deserializeModule) const
    {
        if (!serializeModule->GetEnvironment(thread).IsUndefined() &&
            !deserializeModule->GetEnvironment(thread).IsUndefined()) {
            JSHandle<TaggedArray> serializeEnvironment(thread, serializeModule->GetEnvironment(thread));
            JSHandle<TaggedArray> deserializeEnvironment(thread, deserializeModule->GetEnvironment(thread));
            ASSERT_EQ(serializeEnvironment->GetLength(), deserializeEnvironment->GetLength());
            for (size_t i = 0; i < serializeEnvironment->GetLength(); i++) {
                JSTaggedValue serializeResolvedBinding = serializeEnvironment->Get(thread, i);
                JSTaggedValue deserializeResolvedBinding = deserializeEnvironment->Get(thread, i);
                if (serializeResolvedBinding.IsResolvedIndexBinding()) {
                    ASSERT_TRUE(serializeResolvedBinding.IsResolvedIndexBinding());
                    auto serializeBinding = ResolvedIndexBinding::Cast(serializeResolvedBinding.GetTaggedObject());
                    auto deserializeBinding = ResolvedIndexBinding::Cast(deserializeResolvedBinding.GetTaggedObject());
                    EXPECT_EQ(serializeBinding->GetIndex(), deserializeBinding->GetIndex());
                    JSHandle<SourceTextModule> seriBindingModule(thread, serializeBinding->GetModule(thread));
                    JSHandle<SourceTextModule> deSeriBindingModule(thread, deserializeBinding->GetModule(thread));
                    CheckModule(seriBindingModule, deSeriBindingModule);
                } else if (serializeResolvedBinding.IsResolvedBinding()) {
                    ASSERT_TRUE(serializeResolvedBinding.IsResolvedBinding());
                    auto serializeBinding = ResolvedBinding::Cast(serializeResolvedBinding.GetTaggedObject());
                    auto deserializeBinding = ResolvedBinding::Cast(deserializeResolvedBinding.GetTaggedObject());
                    EXPECT_EQ(serializeBinding->GetBindingName(thread), deserializeBinding->GetBindingName(thread));
                    JSHandle<SourceTextModule> seriBindingModule(thread, serializeBinding->GetModule(thread));
                    JSHandle<SourceTextModule> deSeriBindingModule(thread, deserializeBinding->GetModule(thread));
                    CheckModule(seriBindingModule, deSeriBindingModule);
                } else if (serializeResolvedBinding.IsResolvedRecordIndexBinding()) {
                    ASSERT_TRUE(serializeResolvedBinding.IsResolvedRecordIndexBinding());
                    auto serializeBinding =
                        ResolvedRecordIndexBinding::Cast(serializeResolvedBinding.GetTaggedObject());
                    auto deserializeBinding =
                        ResolvedRecordIndexBinding::Cast(deserializeResolvedBinding.GetTaggedObject());
                    EXPECT_EQ(serializeBinding->GetModuleRecord(thread), deserializeBinding->GetModuleRecord(thread));
                    EXPECT_EQ(serializeBinding->GetAbcFileName(thread), deserializeBinding->GetAbcFileName(thread));
                    EXPECT_EQ(serializeBinding->GetIndex(), deserializeBinding->GetIndex());
                } else if (serializeResolvedBinding.IsResolvedRecordBinding()) {
                    ASSERT_TRUE(serializeResolvedBinding.IsResolvedRecordBinding());
                    auto serializeBinding = ResolvedRecordBinding::Cast(serializeResolvedBinding.GetTaggedObject());
                    auto deserializeBinding = ResolvedRecordBinding::Cast(deserializeResolvedBinding.GetTaggedObject());
                    EXPECT_EQ(serializeBinding->GetModuleRecord(thread), deserializeBinding->GetModuleRecord(thread));
                    EXPECT_EQ(serializeBinding->GetBindingName(thread), deserializeBinding->GetBindingName(thread));
                }
            }
        }
    }

    void CheckModule(JSHandle<SourceTextModule> serializeModule, JSHandle<SourceTextModule> deserializeModule) const
    {
        EXPECT_EQ(serializeModule->GetEcmaModuleFilenameString(), deserializeModule->GetEcmaModuleFilenameString());
        EXPECT_EQ(serializeModule->GetEcmaModuleRecordNameString(), deserializeModule->GetEcmaModuleRecordNameString());
        EXPECT_EQ(serializeModule->GetTypes(), deserializeModule->GetTypes());
        if (serializeModule->GetStatus() > ModuleStatus::INSTANTIATED) {
            EXPECT_EQ(deserializeModule->GetStatus(), ModuleStatus::INSTANTIATED);
        }
        // check request module
        if (!serializeModule->GetRequestedModules(thread).IsUndefined() &&
            !deserializeModule->GetRequestedModules(thread).IsUndefined()) {
            JSHandle<TaggedArray> serializeRequestedModules(thread,
                serializeModule->GetRequestedModules(thread));
            JSHandle<TaggedArray> deserializeRequestedModules(thread,
                deserializeModule->GetRequestedModules(thread));
            ASSERT_EQ(serializeRequestedModules->GetLength(), deserializeRequestedModules->GetLength());
            size_t requestModuleLen = serializeRequestedModules->GetLength();
            for (size_t i = 0; i < requestModuleLen; i++) {
                JSHandle<SourceTextModule> serializeImportModule(thread,
                    serializeRequestedModules->Get(thread, i));
                JSHandle<SourceTextModule> deserializeImportModule(thread,
                    deserializeRequestedModules->Get(thread, i));
                CheckModule(serializeImportModule, deserializeImportModule);
            }
            // check lazy array
            for (size_t i = 0; i < requestModuleLen; i++) {
                EXPECT_EQ(serializeModule->IsLazyImportModule(i), deserializeModule->IsLazyImportModule(i));
            }
        }
        CheckEntries(serializeModule, deserializeModule);
        CheckEnv(serializeModule, deserializeModule);
    }

    void InitMockUpdateBindingModule(bool isDictionaryMode) const
    {
        auto vm = thread->GetEcmaVM();
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<SourceTextModule> etsModule = factory->NewSourceTextModule();
        CString baseFileName = "modules.abc";
        CString recordName = "etsModule";
        etsModule->SetEcmaModuleFilenameString(baseFileName);
        etsModule->SetEcmaModuleRecordNameString(recordName);
        etsModule->SetTypes(ModuleTypes::ECMA_MODULE);
        etsModule->SetStatus(ModuleStatus::INSTANTIATED);
        JSHandle<TaggedArray> requestedModules = factory->NewTaggedArray(1);
        etsModule->SetRequestedModules(thread, requestedModules.GetTaggedValue());
        JSHandle<SourceTextModule> nativeModule = factory->NewSourceTextModule();
        CString nativeModuleRecordName = "nativeModule";
        nativeModule->SetEcmaModuleFilenameString(baseFileName);
        nativeModule->SetEcmaModuleRecordNameString(nativeModuleRecordName);
        nativeModule->SetTypes(ModuleTypes::APP_MODULE);
        nativeModule->SetStatus(ModuleStatus::INSTANTIATED);
        requestedModules->Set(thread, index0, nativeModule);
        JSHandle<EcmaString> str = factory->NewFromStdString("nativeModuleValue");
        size_t localExportEntryLen = 1;
        JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(str);
        JSHandle<LocalExportEntry> localExportEntry =
            factory->NewLocalExportEntry(val, val, index0, SharedTypes::UNSENDABLE_MODULE);
        SourceTextModule::AddLocalExportEntry(thread, nativeModule, localExportEntry, index0, localExportEntryLen);
        JSHandle<JSTaggedValue> objFuncProto = vm->GetGlobalEnv()->GetObjectFunctionPrototype();
        JSHandle<JSHClass> hClassHandle = factory->NewEcmaHClass(JSObject::SIZE, 0, JSType::JS_OBJECT, objFuncProto);
        hClassHandle->SetIsDictionaryMode(isDictionaryMode);
        JSHandle<JSObject> object = factory->NewJSObject(hClassHandle);
        if (isDictionaryMode) {
            JSMutableHandle<NameDictionary> dict(thread,
                NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(1)));
            dict->SetKey(thread, index0, str.GetTaggedValue());
            object->SetProperties(thread, dict);
        } else {
            PropertyDescriptor desc(thread, thread->GlobalConstants()->GetHandledUndefined(), true, true, true);
            JSObject::DefineOwnProperty(thread, object, JSHandle<JSTaggedValue>::Cast(str), desc);
        }
        SourceTextModule::StoreModuleValue(thread, nativeModule, index0, JSHandle<JSTaggedValue>::Cast(object));
        size_t environmentArraySize = 1;
        JSHandle<TaggedArray> environmentArray = factory->NewTaggedArray(environmentArraySize);
        JSHandle<ResolvedIndexBinding> resolvedIndexBinding =
            factory->NewResolvedIndexBindingRecord(nativeModule, index0);
        resolvedIndexBinding->SetIsUpdatedFromResolvedBinding(true);
        environmentArray->Set(thread, index0, resolvedIndexBinding.GetTaggedValue());
        etsModule->SetEnvironment(thread, environmentArray);
        thread->GetModuleManager()->AddResolveImportedModule(recordName, etsModule.GetTaggedValue());
        thread->GetModuleManager()->AddResolveImportedModule(nativeModuleRecordName, nativeModule.GetTaggedValue());
    }

    void CheckRestoreUpdatedBindingBeforeSerialize(bool isDictionaryMode) const
    {
        // construct Module
        CString path = GetSnapshotPath();
        CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
        CString version = TEST_ROM_VERSION.data();
        EcmaVM *vm = thread->GetEcmaVM();
        InitMockUpdateBindingModule(isDictionaryMode);
        // the origin binding is indexBinding
        ModuleManager *moduleManager = thread->GetModuleManager();
        JSHandle<SourceTextModule> serializeModule = moduleManager->HostGetImportedModule("etsModule");
        JSHandle<SourceTextModule> serializeNativeModule = moduleManager->HostGetImportedModule("nativeModule");
        JSHandle<TaggedArray> serializeEnvironment(thread, serializeModule->GetEnvironment(thread));
        JSTaggedValue serializeResolvedBinding = serializeEnvironment->Get(thread, index0);
        ASSERT_TRUE(serializeResolvedBinding.IsResolvedIndexBinding());
        auto serializeBinding = ResolvedIndexBinding::Cast(serializeResolvedBinding.GetTaggedObject());
        ASSERT_TRUE(serializeBinding->GetIsUpdatedFromResolvedBinding());
        ASSERT_EQ(serializeBinding->GetModule(thread), serializeNativeModule.GetTaggedValue());
        // serialize and persist
        ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
        // after serialize, indexBinding has been restore to binding
        serializeResolvedBinding = serializeEnvironment->Get(thread, index0);
        ASSERT_TRUE(serializeResolvedBinding.IsResolvedBinding());
        auto newSerializeBinding = ResolvedBinding::Cast(serializeResolvedBinding.GetTaggedObject());
        ASSERT_EQ(newSerializeBinding->GetModule(thread), serializeNativeModule.GetTaggedValue());
        auto str = EcmaStringAccessor(newSerializeBinding->GetBindingName(thread)).Utf8ConvertToString(thread);
        ASSERT_EQ(str, "nativeModuleValue");
        moduleManager->ClearResolvedModules();
        // deserialize
        ASSERT_TRUE(ModuleSnapshot::DeserializeData(vm, path, version));
        // deserializeModule stored binding
        JSHandle<SourceTextModule> deserializeModule = moduleManager->HostGetImportedModule("etsModule");
        JSHandle<SourceTextModule> deserializeNativeModule = moduleManager->HostGetImportedModule("nativeModule");
        JSHandle<TaggedArray> deserializeEnvironment(thread, deserializeModule->GetEnvironment(thread));
        ASSERT_EQ(serializeEnvironment->GetLength(), deserializeEnvironment->GetLength());
        JSTaggedValue deserializeResolvedBinding = deserializeEnvironment->Get(thread, index0);
        ASSERT_TRUE(deserializeResolvedBinding.IsResolvedBinding());
        auto deserializeBinding = ResolvedBinding::Cast(deserializeResolvedBinding.GetTaggedObject());
        ASSERT_EQ(deserializeBinding->GetModule(thread), deserializeNativeModule.GetTaggedValue());
        str = EcmaStringAccessor(deserializeBinding->GetBindingName(thread)).Utf8ConvertToString(thread);
        ASSERT_EQ(str, "nativeModuleValue");
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    int disabledFeature { 0 };
    int loadedFeature { 0 };
    size_t index0 { 0 };
    size_t index1 { 1 };
    size_t index2 { 2 };
    size_t index3 { 3 };
};

HWTEST_F_L0(ModuleSnapshotTest, SerializeAndDeserializeTest)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString version = TEST_ROM_VERSION.data();
    InitMockSourceTextModule();
    // serialize and persist
    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> serializeModule = moduleManager->HostGetImportedModule("a");
    JSHandle<SourceTextModule> serializeModule1 = moduleManager->HostGetImportedModule("b");
    JSHandle<SourceTextModule> serializeModule2 = moduleManager->HostGetImportedModule("c");
    JSHandle<SourceTextModule> serializeModule3 = moduleManager->HostGetImportedModule("d");
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    moduleManager->ClearResolvedModules();
    // deserialize
    ASSERT_TRUE(ModuleSnapshot::DeserializeData(vm, path, version));
    JSHandle<SourceTextModule> deserializeModule = moduleManager->HostGetImportedModule("a");
    JSHandle<SourceTextModule> deserializeModule1 = moduleManager->HostGetImportedModule("b");
    JSHandle<SourceTextModule> deserializeModule2 = moduleManager->HostGetImportedModule("c");
    JSHandle<SourceTextModule> deserializeModule3 = moduleManager->HostGetImportedModule("d");
    CheckModule(serializeModule, deserializeModule);
    CheckModule(serializeModule1, deserializeModule1);
    CheckModule(serializeModule2, deserializeModule2);
    CheckModule(serializeModule3, deserializeModule3);
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldNotSerializeWhenFileIsExists)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // return false when file is already exists
    ASSERT_FALSE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldNotWritableAfterSave)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    auto permissions = std::filesystem::status(fileName.c_str()).permissions();
    ASSERT_TRUE(static_cast<bool>(permissions & std::filesystem::perms::owner_read));
    ASSERT_FALSE(static_cast<bool>(permissions & std::filesystem::perms::owner_write));
    ASSERT_FALSE(static_cast<bool>(permissions & std::filesystem::perms::owner_exec));
    ASSERT_FALSE(static_cast<bool>(permissions & std::filesystem::perms::others_all));
    ASSERT_FALSE(static_cast<bool>(permissions & std::filesystem::perms::group_all));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldNotDeSerializeWhenFileIsNotExists)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // return false when file is not exists
    ASSERT_FALSE(FileExist(fileName.c_str()));
    ASSERT_FALSE(ModuleSnapshot::DeserializeData(vm, path, version));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldSerializeFailedWhenBufferIsNotMatchBufferSize)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // return false when bufferSize > 0 and buffer is nullptr
    ASSERT_FALSE(MockModuleSnapshot::MockSerializeAndSavingBufferEmpty(vm, path, version));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldDeSerializeFailedWhenFileIsEmpty)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    std::ofstream ofStream(fileName.c_str());
    ofStream.close();
    // return false when file is empty
    ASSERT_TRUE(FileExist(fileName.c_str()));
    ASSERT_FALSE(ModuleSnapshot::DeserializeData(vm, path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldDeSerializeFailedWhenCheckSumIsNotMatch)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // modify file content
    Chmod(fileName, "rw");
    std::ofstream ofStream(fileName.c_str(), std::ios::app);
    uint32_t mockCheckSum = 123456;
    ofStream << mockCheckSum;
    ofStream.close();
    // deserialize failed when checksum is not match
    ASSERT_FALSE(ModuleSnapshot::DeserializeData(vm, path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldDeSerializeFailedWhenAppVersionCodeIsNotMatch)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // modify app version code
    thread->GetEcmaVM()->SetApplicationVersionCode(1);
    // deserialize failed when app version code is not match
    ASSERT_FALSE(ModuleSnapshot::DeserializeData(vm, path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldDeSerializeFailedWhenVersionCodeIsNotMatch)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // deserialize failed when version code is not match
    CString updatedVersion = "version 205.0.1.125(SP20)";
    ASSERT_FALSE(ModuleSnapshot::DeserializeData(vm, path, updatedVersion));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldDeSerializeFailedWhenHasIncompleteData)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::MockSerializeAndSavingHasIncompleteData(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // deserialize failed when has incomplete data
    ASSERT_FALSE(ModuleSnapshot::DeserializeData(vm, path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldRestoreUpdatedBindingBeforeSerializeWhenIsDictionaryMode)
{
    CheckRestoreUpdatedBindingBeforeSerialize(true);
}
HWTEST_F_L0(ModuleSnapshotTest, ShouldRestoreUpdatedBindingBeforeSerializeWhenIsNotDictionaryMode)
{
    CheckRestoreUpdatedBindingBeforeSerialize(false);
}

HWTEST_F_L0(ModuleSnapshotTest, SerializeJsonModule)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    CString baseFileName = MODULE_ABC_PATH "deregister_test.abc";
    CString recordName = "serialize_json";
    CString recordNameA = "1";
    
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteFromFile(thread, baseFileName, recordName);
    EXPECT_TRUE(result);

    JSHandle<JSTaggedValue> value = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8("orverride json"));
    JSHandle<JSTaggedValue> nameKey = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8("name"));
    JSHandle<SourceTextModule> moduleA =
        thread->GetModuleManager()->GetImportedModule(recordNameA);
    JSTaggedValue jsonObj = ModuleValueAccessor::GetModuleValueInner(thread, 0, JSHandle<JSTaggedValue>(moduleA));
    JSTaggedValue res = ObjectFastOperator::FastGetPropertyByName(thread, jsonObj, nameKey.GetTaggedValue());
    // json value change after execution
    EXPECT_EQ(value.GetTaggedValue(), res);

    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // switch moduleManager mock in different process
    ModuleManager *moduleManager1 = thread->GetModuleManager();
    ModuleManager *moduleManager2 = new ModuleManager(vm);
    JSHandle<JSNativePointer> nativePointer(vm->GetGlobalEnv()->GetModuleManagerNativePointer());
    nativePointer->SetExternalPointer(moduleManager2);
    vm->AddModuleManager(moduleManager2);
    EXPECT_NE(moduleManager1, thread->GetModuleManager());
    EXPECT_EQ(thread->GetModuleManager()->GetResolvedModulesSize(), 0);

    ASSERT_TRUE(ModuleSnapshot::DeserializeData(vm, path, version));
    moduleA = thread->GetModuleManager()->GetImportedModule(recordNameA);
    res = moduleA->GetNameDictionary(thread);
    EXPECT_TRUE(res.IsUndefined());

    result = JSPandaFileExecutor::ExecuteFromFile(thread, baseFileName, recordName);
    EXPECT_TRUE(result);

    moduleA = thread->GetModuleManager()->GetImportedModule(recordNameA);
    jsonObj = ModuleValueAccessor::GetModuleValueInner(thread, 0, JSHandle<JSTaggedValue>(moduleA));
    res = ObjectFastOperator::FastGetPropertyByName(thread, jsonObj, nameKey.GetTaggedValue());
    EXPECT_EQ(value.GetTaggedValue(), res);
}

HWTEST_F_L0(ModuleSnapshotTest, SerializeSlicedString)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    CString recordName = "SlicedString";
    module->SetEcmaModuleFilenameString(recordName);
    module->SetEcmaModuleRecordNameString(recordName);
    thread->GetModuleManager()->AddResolveImportedModule(recordName, module.GetTaggedValue());
    // create subString
    JSHandle<EcmaString> fullString(factory->NewFromUtf8("Imcreateformoduleserializetotestforsubstring"));
    JSHandle<JSTaggedValue> subString(thread,
        JSTaggedValue(EcmaStringAccessor::GetSubString(vm, fullString, 5, 15))); // 5, 15 : create substring
    module->SetImportEntries(thread, subString.GetTaggedValue());
    ASSERT_TRUE(EcmaStringAccessor(JSHandle<EcmaString>(subString)).IsSlicedString());

    // serialize SlicedString
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // switch moduleManager mock in different process
    ModuleManager *moduleManager1 = thread->GetModuleManager();
    ModuleManager *moduleManager2 = new ModuleManager(vm);
    JSHandle<JSNativePointer> nativePointer(vm->GetGlobalEnv()->GetModuleManagerNativePointer());
    nativePointer->SetExternalPointer(moduleManager2);
    vm->AddModuleManager(moduleManager2);
    EXPECT_NE(moduleManager1, thread->GetModuleManager());
    EXPECT_EQ(thread->GetModuleManager()->GetResolvedModulesSize(), 0);

    ASSERT_TRUE(ModuleSnapshot::DeserializeData(vm, path, version));
    EXPECT_EQ(thread->GetModuleManager()->GetResolvedModulesSize(), 1);
    module = thread->GetModuleManager()->GetImportedModule(recordName);
    JSTaggedValue res = module->GetImportEntries(thread);
    ASSERT_TRUE(res.IsSlicedString());
    EXPECT_TRUE(JSTaggedValue::SameValue(thread, subString.GetTaggedValue(), res));
}

HWTEST_F_L0(ModuleSnapshotTest, DisableSnapshotFeatureByOption)
{
    auto oldLoaded = ModulesSnapshotHelper::g_featureState_;
    ModulesSnapshotHelper::g_featureState_ = 0;
    ModulesSnapshotHelper::MarkSnapshotDisabledByOption();
    ASSERT_EQ(ModulesSnapshotHelper::g_featureState_,
        static_cast<int>(SnapshotFeatureState::MODULE) |
            static_cast<int>(SnapshotFeatureState::PANDAFILE));
    ASSERT_EQ(ModulesSnapshotHelper::g_featureLoaded_, oldLoaded);
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldDisableModuleSnapshot)
{
    ModulesSnapshotHelper::g_featureLoaded_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
    // Mark as snapshot feature loaded at first
    ModulesSnapshotHelper::MarkJSPandaFileSnapshotLoaded();
    ModulesSnapshotHelper::MarkModuleSnapshotLoaded();
    ASSERT_TRUE(ModulesSnapshotHelper::g_featureLoaded_ & static_cast<int>(SnapshotFeatureState::PANDAFILE));
    ASSERT_TRUE(ModulesSnapshotHelper::g_featureLoaded_ & static_cast<int>(SnapshotFeatureState::MODULE));

    ModulesSnapshotHelper::g_featureState_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
    CString filePath = GetSnapshotPath();
    // snapshot directory is existed, and state file is not existed, all snapshot features should enabled
    ModulesSnapshotHelper::UpdateFromStateFile(filePath);
    ASSERT_EQ(ModulesSnapshotHelper::g_featureState_, static_cast<int>(SnapshotFeatureState::DEFAULT));

    // disable module snapshot feature at first
    ModulesSnapshotHelper::TryDisableSnapshot();
    ModulesSnapshotHelper::UpdateFromStateFile(filePath);
    ASSERT_TRUE(ModulesSnapshotHelper::g_featureState_ & static_cast<int>(SnapshotFeatureState::MODULE));
    ASSERT_FALSE(ModulesSnapshotHelper::g_featureState_ & static_cast<int>(SnapshotFeatureState::PANDAFILE));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldDisablePandafileSnapshot)
{
    ModulesSnapshotHelper::g_featureLoaded_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
    // Mark as snapshot feature loaded at first
    ModulesSnapshotHelper::MarkJSPandaFileSnapshotLoaded();
    ASSERT_TRUE(ModulesSnapshotHelper::g_featureLoaded_ & static_cast<int>(SnapshotFeatureState::PANDAFILE));

    ModulesSnapshotHelper::g_featureState_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
    CString filePath = GetSnapshotPath();
    // snapshot directory is existed, and state file is not existed, all snapshot features should enabled
    ModulesSnapshotHelper::UpdateFromStateFile(filePath);
    ASSERT_EQ(ModulesSnapshotHelper::g_featureState_, static_cast<int>(SnapshotFeatureState::DEFAULT));

    // disable module snapshot feature at first
    ModulesSnapshotHelper::TryDisableSnapshot();
    ModulesSnapshotHelper::UpdateFromStateFile(filePath);
    ASSERT_TRUE(ModulesSnapshotHelper::g_featureState_ & static_cast<int>(SnapshotFeatureState::MODULE));
    ASSERT_TRUE(ModulesSnapshotHelper::g_featureState_ & static_cast<int>(SnapshotFeatureState::PANDAFILE));
}

HWTEST_F_L0(ModuleSnapshotTest, RestoreUpdatedBindingWithUndefinedEnvironment)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    // Create a module with undefined environment
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    CString recordName = "undefinedEnvModule";
    module->SetEcmaModuleFilenameString(baseFileName);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::ECMA_MODULE);
    module->SetStatus(ModuleStatus::INSTANTIATED);
    // Environment is undefined by default, do NOT set environment
    ASSERT_TRUE(module->GetEnvironment(thread).IsUndefined());

    // Create array with this module
    JSHandle<TaggedArray> serializeArray = factory->NewTaggedArray(1);
    serializeArray->Set(thread, 0, module.GetTaggedValue());

    // Call RestoreUpdatedBinding - should skip the module with undefined env (continue path)
    MockModuleSnapshot::MockRestoreUpdatedBinding(thread, serializeArray);
    // Verify environment is still undefined
    ASSERT_TRUE(module->GetEnvironment(thread).IsUndefined());
}

HWTEST_F_L0(ModuleSnapshotTest, RestoreUpdatedBindingWithNonUpdatedBinding)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    // Create a module with environment that has non-updated ResolvedIndexBinding
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    CString recordName = "nonUpdatedModule";
    module->SetEcmaModuleFilenameString(baseFileName);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::ECMA_MODULE);
    module->SetStatus(ModuleStatus::INSTANTIATED);

    JSHandle<SourceTextModule> bindingModule = factory->NewSourceTextModule();
    CString bindingRecordName = "bindingNonUpdated";
    bindingModule->SetEcmaModuleFilenameString(baseFileName);
    bindingModule->SetEcmaModuleRecordNameString(bindingRecordName);

    // Create environment with a ResolvedIndexBinding where IsUpdatedFromResolvedBinding is false
    size_t envSize = 2;
    JSHandle<TaggedArray> envArray = factory->NewTaggedArray(envSize);
    JSHandle<ResolvedIndexBinding> resolvedIndexBinding =
        factory->NewResolvedIndexBindingRecord(bindingModule, index0);
    // IsUpdatedFromResolvedBinding defaults to false, do NOT set it to true
    ASSERT_FALSE(resolvedIndexBinding->GetIsUpdatedFromResolvedBinding());
    envArray->Set(thread, index0, resolvedIndexBinding.GetTaggedValue());
    // Also put an undefined (hole) entry
    envArray->Set(thread, index1, JSTaggedValue::Undefined());
    module->SetEnvironment(thread, envArray);

    JSHandle<TaggedArray> serializeArray = factory->NewTaggedArray(1);
    serializeArray->Set(thread, 0, module.GetTaggedValue());

    // Call RestoreUpdatedBinding - should NOT modify binding since IsUpdatedFromResolvedBinding is false
    MockModuleSnapshot::MockRestoreUpdatedBinding(thread, serializeArray);

    // Verify binding is still ResolvedIndexBinding (not converted)
    JSHandle<TaggedArray> resultEnv(thread, module->GetEnvironment(thread));
    ASSERT_TRUE(resultEnv->Get(thread, index0).IsResolvedIndexBinding());
}

HWTEST_F_L0(ModuleSnapshotTest, RestoreUpdatedBindingMixedEnvEntries)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    // Module with mixed environment: one updated binding + one ResolvedBinding + one undefined
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    CString recordName = "mixedEnvModule";
    module->SetEcmaModuleFilenameString(baseFileName);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::ECMA_MODULE);
    module->SetStatus(ModuleStatus::INSTANTIATED);

    JSHandle<SourceTextModule> nativeModule = factory->NewSourceTextModule();
    CString nativeRecordName = "nativeMixed";
    nativeModule->SetEcmaModuleFilenameString(baseFileName);
    nativeModule->SetEcmaModuleRecordNameString(nativeRecordName);
    nativeModule->SetTypes(ModuleTypes::APP_MODULE);
    nativeModule->SetStatus(ModuleStatus::INSTANTIATED);

    // Add local export entry and store value for nativeModule
    JSHandle<EcmaString> str = factory->NewFromStdString("mixedValue");
    size_t localExportEntryLen = 1;
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(str);
    JSHandle<LocalExportEntry> localExportEntry =
        factory->NewLocalExportEntry(val, val, index0, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, nativeModule, localExportEntry, index0, localExportEntryLen);
    JSHandle<JSTaggedValue> objFuncProto = vm->GetGlobalEnv()->GetObjectFunctionPrototype();
    JSHandle<JSHClass> hClassHandle = factory->NewEcmaHClass(JSObject::SIZE, 0, JSType::JS_OBJECT, objFuncProto);
    JSHandle<JSObject> object = factory->NewJSObject(hClassHandle);
    PropertyDescriptor desc(thread, thread->GlobalConstants()->GetHandledUndefined(), true, true, true);
    JSObject::DefineOwnProperty(thread, object, JSHandle<JSTaggedValue>::Cast(str), desc);
    SourceTextModule::StoreModuleValue(thread, nativeModule, index0, JSHandle<JSTaggedValue>::Cast(object));

    size_t envSize = 3;
    JSHandle<TaggedArray> envArray = factory->NewTaggedArray(envSize);

    // index0: updated ResolvedIndexBinding
    JSHandle<ResolvedIndexBinding> updatedBinding =
        factory->NewResolvedIndexBindingRecord(nativeModule, index0);
    updatedBinding->SetIsUpdatedFromResolvedBinding(true);
    envArray->Set(thread, index0, updatedBinding.GetTaggedValue());

    // index1: regular ResolvedBinding (should not be modified)
    JSHandle<JSTaggedValue> val2 = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8("val2"));
    JSHandle<ResolvedBinding> resolvedBinding = factory->NewResolvedBindingRecord(nativeModule, val2);
    envArray->Set(thread, index1, resolvedBinding.GetTaggedValue());

    // index2: undefined entry
    envArray->Set(thread, index2, JSTaggedValue::Undefined());
    module->SetEnvironment(thread, envArray);

    thread->GetModuleManager()->AddResolveImportedModule(recordName, module.GetTaggedValue());
    thread->GetModuleManager()->AddResolveImportedModule(nativeRecordName, nativeModule.GetTaggedValue());

    JSHandle<TaggedArray> serializeArray = factory->NewTaggedArray(1);
    serializeArray->Set(thread, 0, module.GetTaggedValue());

    // Call RestoreUpdatedBinding
    MockModuleSnapshot::MockRestoreUpdatedBinding(thread, serializeArray);

    // Verify: index0 should be converted from ResolvedIndexBinding to ResolvedBinding
    JSHandle<TaggedArray> resultEnv(thread, module->GetEnvironment(thread));
    ASSERT_TRUE(resultEnv->Get(thread, index0).IsResolvedBinding());
    // index1 should still be ResolvedBinding (not affected)
    ASSERT_TRUE(resultEnv->Get(thread, index1).IsResolvedBinding());
    // index2 should still be undefined
    ASSERT_TRUE(resultEnv->Get(thread, index2).IsUndefined());
}

HWTEST_F_L0(ModuleSnapshotTest, GetModuleSerializeArrayTest)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    // Add normal modules
    JSHandle<SourceTextModule> module1 = factory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    CString recordName1 = "serArrayMod1";
    module1->SetEcmaModuleFilenameString(baseFileName);
    module1->SetEcmaModuleRecordNameString(recordName1);
    module1->SetTypes(ModuleTypes::ECMA_MODULE);
    module1->SetStatus(ModuleStatus::INSTANTIATED);
    thread->GetModuleManager()->AddResolveImportedModule(recordName1, module1.GetTaggedValue());

    JSHandle<SourceTextModule> module2 = factory->NewSourceTextModule();
    CString recordName2 = "serArrayMod2";
    module2->SetEcmaModuleFilenameString(baseFileName);
    module2->SetEcmaModuleRecordNameString(recordName2);
    module2->SetTypes(ModuleTypes::ECMA_MODULE);
    module2->SetStatus(ModuleStatus::INSTANTIATED);
    thread->GetModuleManager()->AddResolveImportedModule(recordName2, module2.GetTaggedValue());

    // Get module serialize array and verify it includes all modules
    JSHandle<TaggedArray> serializeArray = MockModuleSnapshot::MockGetModuleSerializeArray(thread);
    uint32_t normalModuleSize = thread->GetModuleManager()->GetResolvedModulesSize();
    uint32_t sharedModuleSize = SharedModuleManager::GetInstance()->GetResolvedSharedModulesSize();
    ASSERT_EQ(serializeArray->GetLength(), normalModuleSize + sharedModuleSize);
    ASSERT_GE(serializeArray->GetLength(), static_cast<uint32_t>(2));
}

HWTEST_F_L0(ModuleSnapshotTest, SerializeAndDeserializeWithZeroBuffer)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize with zero buffer (bufferSize_ == 0)
    ASSERT_TRUE(MockModuleSnapshot::MockSerializeAndSavingWithZeroBuffer(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // read back the file to verify the zero-buffer read path (bufferSize_ == 0 branch in ReadDataFromFile)
    std::unique_ptr<SerializeData> readData = std::make_unique<SerializeData>(thread);
    ASSERT_TRUE(MockModuleSnapshot::MockReadDataFromFile(thread, readData, path, version));
    // Verify the read data has zero buffer size and nullptr buffer
    ASSERT_EQ(readData->Size(), static_cast<size_t>(0));
    ASSERT_EQ(readData->Data(), nullptr);
}

HWTEST_F_L0(ModuleSnapshotTest, WriteDataToFileMmapFailed)
{
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    JSThread *jsThread = vm->GetJSThread();
    std::unique_ptr<SerializeData> serializeData = MockModuleSnapshot::MockGetSerializeData(jsThread);
    ASSERT_NE(serializeData, nullptr);
    // Use an invalid/non-existent directory path to cause mmap failure
    CString invalidPath = base::ConcatToCString("/non_existent_dir/deep/nested/path/",
        ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME);
    CString version = TEST_ROM_VERSION.data();
    ASSERT_FALSE(MockModuleSnapshot::MockWriteDataToFile(jsThread, serializeData, invalidPath, version));
}

HWTEST_F_L0(ModuleSnapshotTest, GetAlignUpPaddingTest)
{
    // Test GetAlignUpPadding with various alignments
    uint8_t buffer[64];
    void *origin = buffer;

    // When already aligned (offset 0 from origin), padding should be 0
    size_t padding = MockModuleSnapshot::MockGetAlignUpPadding(buffer, origin, sizeof(uint64_t));
    ASSERT_EQ(padding, 0u);

    // When offset by 1, padding should be alignment - 1
    padding = MockModuleSnapshot::MockGetAlignUpPadding(buffer + 1, origin, sizeof(uint64_t));
    ASSERT_EQ(padding, 7u); // 8 - 1 = 7

    // When offset by 4, padding should be 4
    padding = MockModuleSnapshot::MockGetAlignUpPadding(buffer + 4, origin, sizeof(uint64_t));
    ASSERT_EQ(padding, 4u); // 8 - 4 = 4

    // When offset by 8 (already aligned to 8), padding should be 0
    padding = MockModuleSnapshot::MockGetAlignUpPadding(buffer + 8, origin, sizeof(uint64_t));
    ASSERT_EQ(padding, 0u);

    // Test with sizeof(size_t) alignment
    padding = MockModuleSnapshot::MockGetAlignUpPadding(buffer + 3, origin, sizeof(size_t));
    ASSERT_EQ(padding, sizeof(size_t) - 3);
}

HWTEST_F_L0(ModuleSnapshotTest, SerializeDataAndPostSavingJobFileExists)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // First serialize
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // Call SerializeDataAndPostSavingJob when file already exists - should return early
    ModuleSnapshot::SerializeDataAndPostSavingJob(vm, path, version);
    // File should still exist (not modified)
    ASSERT_TRUE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ModuleSnapshotTest, SerializeDataAndPostSavingJobNormal)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // Ensure file does not exist
    ASSERT_FALSE(FileExist(fileName.c_str()));
    // Call the real SerializeDataAndPostSavingJob
    ModuleSnapshot::SerializeDataAndPostSavingJob(vm, path, version);
    // Wait briefly for the async task to complete
    usleep(500000); // 500ms
    // The file should be created by the async task
    ASSERT_TRUE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ModuleSnapshotTest, GetSerializeDataTest)
{
    InitMockSourceTextModule();
    JSThread *jsThread = thread->GetEcmaVM()->GetJSThread();
    // Test GetSerializeData returns valid data
    std::unique_ptr<SerializeData> serializeData = MockModuleSnapshot::MockGetSerializeData(jsThread);
    ASSERT_NE(serializeData, nullptr);
    // Verify serialize data has buffer
    ASSERT_NE(serializeData->Data(), nullptr);
    ASSERT_GT(serializeData->Size(), static_cast<size_t>(0));
}

HWTEST_F_L0(ModuleSnapshotTest, ReadDataFromFileTest)
{
    CString path = GetSnapshotPath();
    CString version = TEST_ROM_VERSION.data();
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // First serialize
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    // Read data from file using MockReadDataFromFile
    std::unique_ptr<SerializeData> data = std::make_unique<SerializeData>(thread);
    ASSERT_TRUE(MockModuleSnapshot::MockReadDataFromFile(thread, data, path, version));
    ASSERT_NE(data, nullptr);
}

HWTEST_F_L0(ModuleSnapshotTest, ReadDataFromFileWithInvalidPath)
{
    CString invalidPath = "/non_existent_path/";
    CString version = TEST_ROM_VERSION.data();
    // Read from non-existent file - should fail
    std::unique_ptr<SerializeData> data = std::make_unique<SerializeData>(thread);
    ASSERT_FALSE(MockModuleSnapshot::MockReadDataFromFile(thread, data, invalidPath, version));
}

HWTEST_F_L0(ModuleSnapshotTest, RestoreUpdatedBindingWithMultipleModules)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    CString baseFileName = "modules.abc";

    // Module 1: has environment with updated binding
    JSHandle<SourceTextModule> module1 = factory->NewSourceTextModule();
    CString recordName1 = "multiMod1";
    module1->SetEcmaModuleFilenameString(baseFileName);
    module1->SetEcmaModuleRecordNameString(recordName1);
    module1->SetTypes(ModuleTypes::ECMA_MODULE);
    module1->SetStatus(ModuleStatus::INSTANTIATED);

    JSHandle<SourceTextModule> nativeModule = factory->NewSourceTextModule();
    CString nativeRecordName = "nativeMulti";
    nativeModule->SetEcmaModuleFilenameString(baseFileName);
    nativeModule->SetEcmaModuleRecordNameString(nativeRecordName);
    nativeModule->SetTypes(ModuleTypes::APP_MODULE);
    nativeModule->SetStatus(ModuleStatus::INSTANTIATED);

    // Setup native module export
    JSHandle<EcmaString> str = factory->NewFromStdString("multiValue");
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(str);
    JSHandle<LocalExportEntry> localExportEntry =
        factory->NewLocalExportEntry(val, val, index0, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, nativeModule, localExportEntry, index0, 1);
    JSHandle<JSTaggedValue> objFuncProto = vm->GetGlobalEnv()->GetObjectFunctionPrototype();
    JSHandle<JSHClass> hClassHandle = factory->NewEcmaHClass(JSObject::SIZE, 0, JSType::JS_OBJECT, objFuncProto);
    JSHandle<JSObject> object = factory->NewJSObject(hClassHandle);
    PropertyDescriptor desc(thread, thread->GlobalConstants()->GetHandledUndefined(), true, true, true);
    JSObject::DefineOwnProperty(thread, object, JSHandle<JSTaggedValue>::Cast(str), desc);
    SourceTextModule::StoreModuleValue(thread, nativeModule, index0, JSHandle<JSTaggedValue>::Cast(object));

    // Environment with updated binding
    JSHandle<TaggedArray> envArray1 = factory->NewTaggedArray(1);
    JSHandle<ResolvedIndexBinding> updatedBinding = factory->NewResolvedIndexBindingRecord(nativeModule, index0);
    updatedBinding->SetIsUpdatedFromResolvedBinding(true);
    envArray1->Set(thread, index0, updatedBinding.GetTaggedValue());
    module1->SetEnvironment(thread, envArray1);

    // Module 2: has undefined environment
    JSHandle<SourceTextModule> module2 = factory->NewSourceTextModule();
    CString recordName2 = "multiMod2";
    module2->SetEcmaModuleFilenameString(baseFileName);
    module2->SetEcmaModuleRecordNameString(recordName2);
    module2->SetTypes(ModuleTypes::ECMA_MODULE);
    module2->SetStatus(ModuleStatus::INSTANTIATED);
    // Keep environment undefined

    // Create serialize array with both modules
    JSHandle<TaggedArray> serializeArray = factory->NewTaggedArray(2);
    serializeArray->Set(thread, 0, module1.GetTaggedValue());
    serializeArray->Set(thread, 1, module2.GetTaggedValue());

    MockModuleSnapshot::MockRestoreUpdatedBinding(thread, serializeArray);

    // module1: binding should be converted
    JSHandle<TaggedArray> resultEnv1(thread, module1->GetEnvironment(thread));
    ASSERT_TRUE(resultEnv1->Get(thread, index0).IsResolvedBinding());
    // module2: environment should still be undefined
    ASSERT_TRUE(module2->GetEnvironment(thread).IsUndefined());
}
}  // namespace panda::test
