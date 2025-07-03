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

#include "ecmascript/js_object-inl.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_snapshot.h"
#include "ecmascript/tests/test_helper.h"

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
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed";
        }
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        CString path = GetSnapshotPath();
        CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
        if (remove(fileName.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed";
        }
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

    void InitEntries(JSHandle<SourceTextModule> module) const
    {
        ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
        size_t importEntryArrayLen = 2;
        JSHandle<JSTaggedValue> importName = val;
        JSHandle<JSTaggedValue> localName = val;
        JSHandle<ImportEntry> importEntry1 =
            objectFactory->NewImportEntry(index0, importName, localName, SharedTypes::UNSENDABLE_MODULE);
        SourceTextModule::AddImportEntry(thread, module, importEntry1, index0, importEntryArrayLen);
        JSHandle<JSTaggedValue> starString = thread->GlobalConstants()->GetHandledStarString();
        JSHandle<ImportEntry> importEntry2 =
            objectFactory->NewImportEntry(index1, starString, localName, SharedTypes::UNSENDABLE_MODULE);
        SourceTextModule::AddImportEntry(thread, module, importEntry2, index1, importEntryArrayLen);

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

        InitEntries(module);
        InitEnv(module, module2);

        thread->GetModuleManager()->AddResolveImportedModule(recordName, module.GetTaggedValue());
        thread->GetModuleManager()->AddResolveImportedModule(recordName1, module1.GetTaggedValue());
        thread->GetModuleManager()->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
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
        } else {
            EXPECT_EQ(serializeModule->GetStatus(), deserializeModule->GetStatus());
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

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    size_t index0 { 0 };
    size_t index1 { 1 };
    size_t index2 { 2 };
    size_t index3 { 3 };
};

HWTEST_F_L0(ModuleSnapshotTest, SerializeAndDeserializeTest)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    InitMockSourceTextModule();
    // serialize and persist
    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> serializeModule = moduleManager->HostGetImportedModule("a");
    JSHandle<SourceTextModule> serializeModule1 = moduleManager->HostGetImportedModule("b");
    JSHandle<SourceTextModule> serializeModule2 = moduleManager->HostGetImportedModule("c");
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    moduleManager->ClearResolvedModules();
    // deserialize
    ASSERT_TRUE(ModuleSnapshot::DeserializeData(vm, path, version));
    JSHandle<SourceTextModule> deserializeModule = moduleManager->HostGetImportedModule("a");
    JSHandle<SourceTextModule> deserializeModule1 = moduleManager->HostGetImportedModule("b");
    JSHandle<SourceTextModule> deserializeModule2 = moduleManager->HostGetImportedModule("c");
    CheckModule(serializeModule, deserializeModule);
    CheckModule(serializeModule1, deserializeModule1);
    CheckModule(serializeModule2, deserializeModule2);
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldNotSerializeWhenFileIsExists)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // return false when file is already exists
    ASSERT_FALSE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
}

HWTEST_F_L0(ModuleSnapshotTest, ShouldNotDeSerializeWhenFileIsNotExists)
{
    // construct Module
    CString path = GetSnapshotPath();
    CString fileName = path + ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
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
    CString version = "version 205.0.1.120(SP20)";
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
    CString version = "version 205.0.1.120(SP20)";
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
    CString version = "version 205.0.1.120(SP20)";
    EcmaVM *vm = thread->GetEcmaVM();
    InitMockSourceTextModule();
    // serialize and persist
    ASSERT_TRUE(MockModuleSnapshot::SerializeDataAndSaving(vm, path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // modify file content
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
    CString version = "version 205.0.1.120(SP20)";
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
    CString version = "version 205.0.1.120(SP20)";
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
    CString version = "version 205.0.1.120(SP20)";
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
}  // namespace panda::test
