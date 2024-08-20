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

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "libpandafile/class_data_accessor-inl.h"

#include "ecmascript/base/path_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_logger.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/module/module_tools.h"
#include "ecmascript/require/js_cjs_module.h"


using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class EcmaModuleTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }
    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/*
 * Feature: Module
 * Function: AddImportEntry
 * SubFunction: AddImportEntry
 * FunctionPoints: Add import entry
 * CaseDescription: Add two import item and check module import entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddImportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<ImportEntry> importEntry1 = objectFactory->NewImportEntry();
    SourceTextModule::AddImportEntry(thread, module, importEntry1, 0, 2);
    JSHandle<ImportEntry> importEntry2 = objectFactory->NewImportEntry();
    SourceTextModule::AddImportEntry(thread, module, importEntry2, 1, 2);
    JSHandle<TaggedArray> importEntries(thread, module->GetImportEntries());
    EXPECT_TRUE(importEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: AddLocalExportEntry
 * SubFunction: AddLocalExportEntry
 * FunctionPoints: Add local export entry
 * CaseDescription: Add two local export item and check module local export entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddLocalExportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<LocalExportEntry> localExportEntry1 = objectFactory->NewLocalExportEntry();
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry1, 0, 2);
    JSHandle<LocalExportEntry> localExportEntry2 = objectFactory->NewLocalExportEntry();
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry2, 1, 2);
    JSHandle<TaggedArray> localExportEntries(thread, module->GetLocalExportEntries());
    EXPECT_TRUE(localExportEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: AddIndirectExportEntry
 * SubFunction: AddIndirectExportEntry
 * FunctionPoints: Add indirect export entry
 * CaseDescription: Add two indirect export item and check module indirect export entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddIndirectExportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<IndirectExportEntry> indirectExportEntry1 = objectFactory->NewIndirectExportEntry();
    SourceTextModule::AddIndirectExportEntry(thread, module, indirectExportEntry1, 0, 2);
    JSHandle<IndirectExportEntry> indirectExportEntry2 = objectFactory->NewIndirectExportEntry();
    SourceTextModule::AddIndirectExportEntry(thread, module, indirectExportEntry2, 1, 2);
    JSHandle<TaggedArray> indirectExportEntries(thread, module->GetIndirectExportEntries());
    EXPECT_TRUE(indirectExportEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: StarExportEntries
 * SubFunction: StarExportEntries
 * FunctionPoints: Add start export entry
 * CaseDescription: Add two start export item and check module start export entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddStarExportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<StarExportEntry> starExportEntry1 = objectFactory->NewStarExportEntry();
    SourceTextModule::AddStarExportEntry(thread, module, starExportEntry1, 0, 2);
    JSHandle<StarExportEntry> starExportEntry2 = objectFactory->NewStarExportEntry();
    SourceTextModule::AddStarExportEntry(thread, module, starExportEntry2, 1, 2);
    JSHandle<TaggedArray> startExportEntries(thread, module->GetStarExportEntries());
    EXPECT_TRUE(startExportEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: StoreModuleValue
 * SubFunction: StoreModuleValue/GetModuleValue
 * FunctionPoints: store a module export item in module
 * CaseDescription: Simulated implementation of "export foo as bar", set foo as "hello world",
 *                  use "import bar" in same js file
 */
HWTEST_F_L0(EcmaModuleTest, StoreModuleValue)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    CString localName = "foo";
    CString exportName = "bar";
    CString value = "hello world";

    JSHandle<JSTaggedValue> localNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSHandle<JSTaggedValue> exportNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName));
    JSHandle<LocalExportEntry> localExportEntry =
        objFactory->NewLocalExportEntry(exportNameHandle, localNameHandle, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                        SharedTypes::UNSENDABLE_MODULE);
    JSHandle<SourceTextModule> module = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry, 0, 1);

    JSHandle<JSTaggedValue> storeKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSHandle<JSTaggedValue> valueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value));
    module->StoreModuleValue(thread, storeKey, valueHandle);

    JSHandle<JSTaggedValue> loadKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSTaggedValue loadValue = module->GetModuleValue(thread, loadKey.GetTaggedValue(), false);
    EXPECT_EQ(valueHandle.GetTaggedValue(), loadValue);
}

/*
 * Feature: Module
 * Function: GetModuleValue
 * SubFunction: StoreModuleValue/GetModuleValue
 * FunctionPoints: load module value from module
 * CaseDescription: Simulated implementation of "export default let foo = 'hello world'",
 *                  use "import C from 'xxx' to get default value"
 */
HWTEST_F_L0(EcmaModuleTest, GetModuleValue)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    // export entry
    CString exportLocalName = "*default*";
    CString exportName = "default";
    CString exportValue = "hello world";
    JSHandle<JSTaggedValue> exportLocalNameHandle =
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportLocalName));
    JSHandle<JSTaggedValue> exportNameHandle =
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName));
    JSHandle<LocalExportEntry> localExportEntry = objFactory->NewLocalExportEntry(exportNameHandle,
        exportLocalNameHandle, LocalExportEntry::LOCAL_DEFAULT_INDEX, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<SourceTextModule> moduleExport = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, moduleExport, localExportEntry, 0, 1);
    // store module value
    JSHandle<JSTaggedValue> exportValueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportValue));
    moduleExport->StoreModuleValue(thread, exportLocalNameHandle, exportValueHandle);

    JSTaggedValue importDefaultValue =
        moduleExport->GetModuleValue(thread, exportLocalNameHandle.GetTaggedValue(), false);
    EXPECT_EQ(exportValueHandle.GetTaggedValue(), importDefaultValue);
}

HWTEST_F_L0(EcmaModuleTest, GetExportedNames)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    JSHandle<TaggedArray> exportStarSet = factory->NewTaggedArray(2);
    exportStarSet->Set(thread, 0, module.GetTaggedValue());

    CVector<std::string> exportedNames = SourceTextModule::GetExportedNames(thread, module, exportStarSet);
    EXPECT_EQ(exportedNames.size(), 0);
}

HWTEST_F_L0(EcmaModuleTest, FindByExport)
{
    CString localName1 = "foo";
    CString localName2 = "foo2";
    CString localName3 = "foo3";
    CString exportName1 = "bar";
    CString exportName2 = "bar2";
    CString value = "hello world";
    CString value2 = "hello world2";

    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> localNameHandle1 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName1));
    JSHandle<JSTaggedValue> localNameHandle2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName2));
    JSHandle<JSTaggedValue> exportNameHandle1 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName1));
    JSHandle<JSTaggedValue> exportNameHandle2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName2));
    JSHandle<LocalExportEntry> localExportEntry1 =
        objFactory->NewLocalExportEntry(exportNameHandle1, localNameHandle1, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                        SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry1, 0, 2);

    JSHandle<LocalExportEntry> localExportEntry2 =
        objFactory->NewLocalExportEntry(exportNameHandle2, localNameHandle2, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                        SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry2, 1, 2);

    JSHandle<JSTaggedValue> storeKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName1));
    JSHandle<JSTaggedValue> valueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value));
    module->StoreModuleValue(thread, storeKey, valueHandle);

    JSHandle<JSTaggedValue> storeKey2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName2));
    JSHandle<JSTaggedValue> valueHandle2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value2));
    module->StoreModuleValue(thread, storeKey2, valueHandle2);

    // FindByExport cannot find key from exportEntries, returns Hole()
    JSHandle<JSTaggedValue> loadKey1 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName3));
    JSTaggedValue loadValue1 = module->GetModuleValue(thread, loadKey1.GetTaggedValue(), false);
    EXPECT_EQ(JSTaggedValue::Hole(), loadValue1);

    // FindByExport retrieves the key from exportEntries and returns the value corresponding to the key
    JSHandle<JSTaggedValue> loadKey2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName1));
    JSTaggedValue loadValue2 = module->GetModuleValue(thread, loadKey2.GetTaggedValue(), false);
    EXPECT_EQ(valueHandle.GetTaggedValue(), loadValue2);
}

HWTEST_F_L0(EcmaModuleTest, GetRecordName1)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module_base.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module_base");
    EXPECT_TRUE(result);
}

HWTEST_F_L0(EcmaModuleTest, GetRecordName2)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_A.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_A");
    EXPECT_TRUE(result);
}

HWTEST_F_L0(EcmaModuleTest, GetExportObjectIndex)
{
    ThreadNativeScope nativeScope(thread);
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    JSNApi::GetExportObject(instance, "module_test_module_test_B", "a");
    EXPECT_TRUE(result);
}

HWTEST_F_L0(EcmaModuleTest, HostResolveImportedModule)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    JSHandle<JSTaggedValue> moduleRecord(thread, module.GetTaggedValue());
    moduleManager->AddResolveImportedModule(baseFileName.c_str(), moduleRecord.GetTaggedValue());
    JSHandle<JSTaggedValue> res = moduleManager->HostResolveImportedModule(baseFileName.c_str());

    EXPECT_EQ(moduleRecord->GetRawData(), res.GetTaggedValue().GetRawData());

    // baseFileName is not an absolute path
    std::string baseFileName2 = "module_test_module_test_C.abc";
    JSHandle<JSTaggedValue> res2 = moduleManager->HostResolveImportedModule(baseFileName2.c_str());
    EXPECT_EQ(JSTaggedValue::Exception(), res2.GetTaggedValue());
}

HWTEST_F_L0(EcmaModuleTest, HostResolveImportedModule2)
{
    CString recordName = "module_test_module_test_A";
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();

    CString baseFileName = MODULE_ABC_PATH "module_test_module_test_A.abc";
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_NE(jsPandaFile, nullptr);
    JSHandle<JSTaggedValue> res1 = moduleManager->HostResolveImportedModule(jsPandaFile.get(), baseFileName);
    EXPECT_NE(res1.GetTaggedValue(), JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> res2 = moduleManager->HostResolveImportedModule(jsPandaFile.get(), baseFileName);
    EXPECT_NE(res2.GetTaggedValue(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, IsSharedModuleLoaded)
{
    CString recordName = "module_test_module_test_A";
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    bool res = moduleManager->IsSharedModuleLoaded(recordName);
    EXPECT_EQ(res, false);
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueInner)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    JSHandle<JSTaggedValue> moduleRecord = JSHandle<JSTaggedValue>::Cast(module);

    JSTaggedValue res1 = moduleManager->GetModuleValueInner(0, moduleRecord);
    EXPECT_EQ(res1, JSTaggedValue::Hole());

    JSTaggedValue res2 = moduleManager->GetModuleValueOutter(0, moduleRecord);
    EXPECT_TRUE(res2.IsInt());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueOutterInternal)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_B");
    JSHandle<JSTaggedValue> moduleRecord = JSHandle<JSTaggedValue>::Cast(module);

    // moduleEnvironment IsUndefined
    JSTaggedValue res = moduleManager->GetModuleValueOutter(0, moduleRecord);
    EXPECT_TRUE(res.IsSpecial());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleNamespaceInternal)
{
    CString localName = "foo";

    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> localNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();

    // moduleEnvironment IsUndefined
    JSHandle<SourceTextModule> module = objFactory->NewSourceTextModule();
    JSTaggedValue res1 = moduleManager->GetModuleNamespaceInternal(localNameHandle.GetTaggedValue(),
        module.GetTaggedValue());
    EXPECT_EQ(res1, thread->GlobalConstants()->GetUndefined());

    // FindEntry fail
    JSHandle<NameDictionary> map(NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(1)));
    module->SetEnvironment(thread, map);
    JSTaggedValue res2 = moduleManager->GetModuleNamespaceInternal(localNameHandle.GetTaggedValue(),
        module.GetTaggedValue());
    EXPECT_EQ(res2, thread->GlobalConstants()->GetUndefined());
}

HWTEST_F_L0(EcmaModuleTest, GetExportObjectIndex2)
{
    CString key = "module_test_module_test_A";
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();

    // EcmaModuleRecordNameString is empty
    int res1 = moduleManager->GetExportObjectIndex(thread->GetEcmaVM(), module, key);
    EXPECT_EQ(res1, 0);

    // EcmaModuleRecordNameString is not empty
    module->SetEcmaModuleRecordNameString("test");
    int res2 = moduleManager->GetExportObjectIndex(thread->GetEcmaVM(), module, key);
    EXPECT_EQ(res2, 0);
}

HWTEST_F_L0(EcmaModuleTest, GetModuleNameSpaceFromFile)
{
    CString recordName1 = "a";
    CString recordName2 = "module_test_module_test_B";
    CString baseFileName = MODULE_ABC_PATH "module_test_module_test_B.abc";
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();

    // ExecuteFromAbcFile is success
    JSHandle<JSTaggedValue> res2 = moduleManager->GetModuleNameSpaceFromFile(thread, recordName2, baseFileName);
    EXPECT_NE(res2, thread->GlobalConstants()->GetHandledUndefinedString());

    // ExecuteFromAbcFile is fail
    JSHandle<JSTaggedValue> res1 = moduleManager->GetModuleNameSpaceFromFile(thread, recordName1, baseFileName);
    EXPECT_EQ(res1, thread->GlobalConstants()->GetHandledUndefinedString());
}

HWTEST_F_L0(EcmaModuleTest, TryGetImportedModule)
{
    // TryGetImportedModule can not found Module
    CString moduleName = "moduleName";
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> res1 = moduleManager->TryGetImportedModule(moduleName);
    EXPECT_EQ(res1, thread->GlobalConstants()->GetHandledUndefined());

    // TryGetImportedModule can found Module
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    JSHandle<JSTaggedValue> moduleRecord(thread, module.GetTaggedValue());
    moduleManager->AddResolveImportedModule(moduleName, moduleRecord.GetTaggedValue());
    JSHandle<JSTaggedValue> res2 = moduleManager->TryGetImportedModule(moduleName);
    EXPECT_EQ(res2, moduleRecord);
}

HWTEST_F_L0(EcmaModuleTest, PreventExtensions_IsExtensible)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<LocalExportEntry> localExportEntry1 = objectFactory->NewLocalExportEntry();
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry1, 0, 2);
    JSHandle<LocalExportEntry> localExportEntry2 = objectFactory->NewLocalExportEntry();
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry2, 1, 2);
    JSHandle<TaggedArray> localExportEntries(thread, module->GetLocalExportEntries());
    CString baseFileName = "a.abc";
    module->SetEcmaModuleFilenameString(baseFileName);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> moduleRecord = JSHandle<JSTaggedValue>::Cast(module);
    moduleManager->AddResolveImportedModule(baseFileName, moduleRecord.GetTaggedValue());
    JSHandle<ModuleNamespace> np =
    ModuleNamespace::ModuleNamespaceCreate(thread, moduleRecord, localExportEntries);
    EXPECT_FALSE(np->IsExtensible());
    EXPECT_TRUE(ModuleNamespace::PreventExtensions());
}

HWTEST_F_L0(EcmaModuleTest, ConcatFileNameWithMerge1)
{
    CString baseFilename = "merge.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), baseFilename);

    // Test moduleRequestName start with "@bundle"
    CString moduleRecordName = "moduleTest1";
    CString moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    CString result = "com.bundleName.test/moduleName/requestModuleName1";
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename, moduleRecordName,
                                                             moduleRequestName);
    EXPECT_EQ(result, entryPoint);

    // Test cross application
    moduleRecordName = "@bundle:com.bundleName1.test/moduleName/requestModuleName1";
    CString newBaseFileName = "/data/storage/el1/bundle/com.bundleName.test/moduleName/moduleName/ets/modules.abc";
    ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename, moduleRecordName, moduleRequestName);
    EXPECT_EQ(baseFilename, newBaseFileName);
}

HWTEST_F_L0(EcmaModuleTest, ConcatFileNameWithMerge2)
{
    CString baseFilename = "merge.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), baseFilename);

    // Test moduleRequestName start with "./"
    CString moduleRecordName = "moduleTest2";
    CString moduleRequestName = "./requestModule.js";
    CString result = "requestModule";
    pf->InsertJSRecordInfo(result);
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename, moduleRecordName,
                                                             moduleRequestName);
    EXPECT_EQ(result, entryPoint);

    // Test moduleRecordName with "/"
    moduleRecordName = "moduleName/moduleTest2";
    moduleRequestName = "./requestModule.js";
    result = "moduleName/requestModule";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatFileNameWithMerge(
        thread, pf.get(), baseFilename, moduleRecordName, moduleRequestName);
    EXPECT_EQ(result, entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, ConcatFileNameWithMerge3)
{
    CString baseFilename = "merge.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), baseFilename);

    // Test RecordName is not in JSPandaFile
    CString moduleRecordName = "moduleTest3";
    CString moduleRequestName = "./secord.js";
    CString result = "secord";
    CString requestFileName = "secord.abc";
    CString entryPoint =
        ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename, moduleRecordName, moduleRequestName);
    EXPECT_EQ(baseFilename, requestFileName);
    EXPECT_EQ(result, entryPoint);

    // Test RecordName is not in JSPandaFile and baseFilename with "/" and moduleRequestName with "/"
    baseFilename = "test/merge.abc";
    std::unique_ptr<const File> pfPtr2 = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), baseFilename);

    moduleRecordName = "moduleTest3";
    moduleRequestName = "./test/secord.js";
    result = "secord";
    requestFileName = "test/test/secord.abc";
    entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, pf2.get(), baseFilename, moduleRecordName,
                                                     moduleRequestName);
    EXPECT_EQ(baseFilename, requestFileName);
    EXPECT_EQ(result, entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, ConcatFileNameWithMerge4)
{
    CString baseFilename = "merge.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), baseFilename);
    CUnorderedMap<CString, JSPandaFile::JSRecordInfo*> &recordInfo =
        const_cast<CUnorderedMap<CString, JSPandaFile::JSRecordInfo*>&>(pf->GetJSRecordInfo());
    
    CString moduleRecordName = "node_modules/0/moduleTest4/index";
    CString moduleRequestName = "json/index";
    CString result = "node_modules/0/moduleTest4/node_modules/json/index";
    JSPandaFile::JSRecordInfo *info = new JSPandaFile::JSRecordInfo();
    info->npmPackageName = "node_modules/0/moduleTest4";
    recordInfo.insert({moduleRecordName, info});
    recordInfo.insert({result, info});
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(
        thread, pf.get(), baseFilename, moduleRecordName, moduleRequestName);
    EXPECT_EQ(result, entryPoint);
    
    // delete info
    delete info;
    recordInfo.erase(moduleRecordName);
    recordInfo.erase(result);
}


HWTEST_F_L0(EcmaModuleTest, NormalizePath)
{
    CString res1 = "node_modules/0/moduleTest/index";
    CString moduleRecordName1 = "node_modules///0//moduleTest/index";

    CString res2 = "node_modules/0/moduleTest/index";
    CString moduleRecordName2 = "./node_modules///0//moduleTest/index";

    CString res3 = "node_modules/0/moduleTest/index";
    CString moduleRecordName3 = "../node_modules/0/moduleTest///index";

    CString res4 = "moduleTest/index";
    CString moduleRecordName4 = "./node_modules/..//moduleTest////index";

    CString res5 = "node_modules/moduleTest/index";
    CString moduleRecordName5 = "node_modules/moduleTest/index/";

    CString normalName1 = PathHelper::NormalizePath(moduleRecordName1);
    CString normalName2 = PathHelper::NormalizePath(moduleRecordName2);
    CString normalName3 = PathHelper::NormalizePath(moduleRecordName3);
    CString normalName4 = PathHelper::NormalizePath(moduleRecordName4);
    CString normalName5 = PathHelper::NormalizePath(moduleRecordName5);

    EXPECT_EQ(res1, normalName1);
    EXPECT_EQ(res2, normalName2);
    EXPECT_EQ(res3, normalName3);
    EXPECT_EQ(res4, normalName4);
    EXPECT_EQ(res5, normalName5);
}

HWTEST_F_L0(EcmaModuleTest, ParseAbcPathAndOhmUrl)
{
    // old pages url
    instance->SetBundleName("com.bundleName.test");
    instance->SetModuleName("moduleName");
    CString inputFileName = "moduleName/ets/pages/index.abc";
    CString outFileName = "";
    CString res1 = "com.bundleName.test/moduleName/ets/pages/index";
    CString entryPoint;
    ModulePathHelper::ParseAbcPathAndOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res1);
    EXPECT_EQ(outFileName, "");

    // new pages url
    inputFileName = "@bundle:com.bundleName.test/moduleName/ets/pages/index.abc";
    ModulePathHelper::ParseAbcPathAndOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res1);
    EXPECT_EQ(outFileName, "/data/storage/el1/bundle/moduleName/ets/modules.abc");

    // new pages url Intra-application cross hap
    inputFileName = "@bundle:com.bundleName.test/moduleName1/ets/pages/index.abc";
    CString outRes = "/data/storage/el1/bundle/moduleName1/ets/modules.abc";
    CString res2 = "com.bundleName.test/moduleName1/ets/pages/index";
    ModulePathHelper::ParseAbcPathAndOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res2);
    EXPECT_EQ(outFileName, outRes);

    // new pages url Cross-application
    inputFileName = "@bundle:com.bundleName.test1/moduleName1/ets/pages/index.abc";
    CString outRes1 = "/data/storage/el1/bundle/com.bundleName.test1/moduleName1/moduleName1/ets/modules.abc";
    CString res3 = "com.bundleName.test1/moduleName1/ets/pages/index";
    ModulePathHelper::ParseAbcPathAndOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res3);
    EXPECT_EQ(outFileName, outRes1);

    // worker url Intra-application cross hap
    inputFileName = "/data/storage/el1/bundle/entry/ets/mainAbility.abc";
    CString outRes2 = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString res4 = "com.bundleName.test/entry/ets/mainAbility";
    ModulePathHelper::ParseAbcPathAndOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res4);
    EXPECT_EQ(outFileName, outRes2);

    // worker url
    outFileName = "";
    inputFileName = "/data/storage/el1/bundle/moduleName/ets/mainAbility.abc";
    CString res5 = "com.bundleName.test/moduleName/ets/mainAbility";
    ModulePathHelper::ParseAbcPathAndOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res5);
    EXPECT_EQ(outFileName, "/data/storage/el1/bundle/moduleName/ets/modules.abc");
}

HWTEST_F_L0(EcmaModuleTest, CheckNativeModule)
{
    // load file
    CString requestName1 = "@bundle:bundleName/moduleName/ets/index";

    // load native modules
    CString requestName2 = "@ohos:router";
    CString requestName3 = "@app:bundleName/moduleName/lib*.so";
    CString requestName4 = "@native:system.app";
    CString requestName5 = "@xxx:internal";

    // load npm Packages
    CString requestName6 = "@package:pkg_modules/.ohpm/json5@2.2.3/pkg_modules/json5/dist/index";
    CString requestName7 = "@ohos/common";

    std::pair<bool, ModuleTypes> res1 = SourceTextModule::CheckNativeModule(requestName1);
    EXPECT_EQ(res1.first, false);
    EXPECT_EQ(res1.second, ModuleTypes::UNKNOWN);

    std::pair<bool, ModuleTypes> res2 = SourceTextModule::CheckNativeModule(requestName2);
    EXPECT_EQ(res2.first, true);
    EXPECT_EQ(res2.second, ModuleTypes::OHOS_MODULE);

    std::pair<bool, ModuleTypes> res3 = SourceTextModule::CheckNativeModule(requestName3);
    EXPECT_EQ(res3.first, true);
    EXPECT_EQ(res3.second, ModuleTypes::APP_MODULE);

    std::pair<bool, ModuleTypes> res4 = SourceTextModule::CheckNativeModule(requestName4);
    EXPECT_EQ(res4.first, true);
    EXPECT_EQ(res4.second, ModuleTypes::NATIVE_MODULE);

    std::pair<bool, ModuleTypes> res5 = SourceTextModule::CheckNativeModule(requestName5);
    EXPECT_EQ(res5.first, true);
    EXPECT_EQ(res5.second, ModuleTypes::INTERNAL_MODULE);

    std::pair<bool, ModuleTypes> res6 = SourceTextModule::CheckNativeModule(requestName6);
    EXPECT_EQ(res6.first, false);
    EXPECT_EQ(res6.second, ModuleTypes::UNKNOWN);

    std::pair<bool, ModuleTypes> res7 = SourceTextModule::CheckNativeModule(requestName7);
    EXPECT_EQ(res7.first, false);
    EXPECT_EQ(res7.second, ModuleTypes::UNKNOWN);
}

HWTEST_F_L0(EcmaModuleTest, ResolveDirPath)
{
    CString inputFileName = "moduleName/ets/pages/index.abc";
    CString resName1 = "moduleName/ets/pages/";
    CString outFileName = PathHelper::ResolveDirPath(inputFileName);
    EXPECT_EQ(outFileName, resName1);

    inputFileName = "moduleName\\ets\\pages\\index.abc";
    CString resName2 = "moduleName\\ets\\pages\\";
    outFileName = PathHelper::ResolveDirPath(inputFileName);
    EXPECT_EQ(outFileName, resName2);

    inputFileName = "cjs";
    CString resName3 = "";
    outFileName = PathHelper::ResolveDirPath(inputFileName);
    EXPECT_EQ(outFileName, resName3);
}

HWTEST_F_L0(EcmaModuleTest, DeleteNamespace)
{
    CString inputFileName = "moduleName@nameSpace";
    CString res1 = "moduleName";
    PathHelper::DeleteNamespace(inputFileName);
    EXPECT_EQ(inputFileName, res1);

    inputFileName = "moduleName";
    CString res2 = "moduleName";
    PathHelper::DeleteNamespace(inputFileName);
    EXPECT_EQ(inputFileName, res2);
}

HWTEST_F_L0(EcmaModuleTest, AdaptOldIsaRecord)
{
    CString inputFileName = "bundleName/moduleName@namespace/moduleName";
    CString res1 = "moduleName";
    PathHelper::AdaptOldIsaRecord(inputFileName);
    EXPECT_EQ(inputFileName, res1);
}

HWTEST_F_L0(EcmaModuleTest, GetStrippedModuleName)
{
    CString inputFileName = "@ohos:hilog";
    CString res1 = "hilog";
    CString outFileName = PathHelper::GetStrippedModuleName(inputFileName);
    EXPECT_EQ(outFileName, res1);
}

HWTEST_F_L0(EcmaModuleTest, GetInternalModulePrefix)
{
    CString inputFileName = "@ohos:hilog";
    CString res1 = "ohos";
    CString outFileName = PathHelper::GetInternalModulePrefix(inputFileName);
    EXPECT_EQ(outFileName, res1);
}

HWTEST_F_L0(EcmaModuleTest, IsNativeModuleRequest)
{
    CString inputFileName = "json5";
    bool res1 = ModulePathHelper::IsNativeModuleRequest(inputFileName);
    EXPECT_TRUE(!res1);

    inputFileName = "@ohos:hilog";
    bool res2 = ModulePathHelper::IsNativeModuleRequest(inputFileName);
    EXPECT_TRUE(res2);

    inputFileName = "@app:xxxx";
    bool res3 = ModulePathHelper::IsNativeModuleRequest(inputFileName);
    EXPECT_TRUE(res3);

    inputFileName = "@native:xxxx";
    bool res4 = ModulePathHelper::IsNativeModuleRequest(inputFileName);
    EXPECT_TRUE(res4);
}

HWTEST_F_L0(EcmaModuleTest, IsImportFile)
{
    CString inputFileName = "./test";
    bool res1 = ModulePathHelper::IsImportFile(inputFileName);
    EXPECT_TRUE(res1);
    CString outFileName = ModulePathHelper::RemoveSuffix(inputFileName);
    EXPECT_EQ(outFileName, inputFileName);

    inputFileName = "test";
    bool res2 = ModulePathHelper::IsImportFile(inputFileName);
    EXPECT_TRUE(!res2);
    outFileName = ModulePathHelper::RemoveSuffix(inputFileName);
    EXPECT_EQ(outFileName, inputFileName);

    CString result = "test";
    inputFileName = "test.js";
    bool res3 = ModulePathHelper::IsImportFile(inputFileName);
    EXPECT_TRUE(res3);
    outFileName = ModulePathHelper::RemoveSuffix(inputFileName);
    EXPECT_EQ(outFileName, result);

    inputFileName = "test.ts";
    bool res4 = ModulePathHelper::IsImportFile(inputFileName);
    EXPECT_TRUE(res4);
    outFileName = ModulePathHelper::RemoveSuffix(inputFileName);
    EXPECT_EQ(outFileName, result);

    inputFileName = "test.ets";
    bool res5 = ModulePathHelper::IsImportFile(inputFileName);
    EXPECT_TRUE(res5);
    outFileName = ModulePathHelper::RemoveSuffix(inputFileName);
    EXPECT_EQ(outFileName, result);

    inputFileName = "test.json";
    bool res6 = ModulePathHelper::IsImportFile(inputFileName);
    EXPECT_TRUE(res6);
    outFileName = ModulePathHelper::RemoveSuffix(inputFileName);
    EXPECT_EQ(outFileName, result);
}

HWTEST_F_L0(EcmaModuleTest, GetModuleNameWithPath)
{
    CString inputPath1 = "com.example.application/entry";
    CString res1 = "entry";
    CString outFileName1 = ModulePathHelper::GetModuleNameWithPath(inputPath1);
    EXPECT_EQ(outFileName1, res1);

    CString inputPath2 = "com.example.applicationentry";
    CString res2 = "";
    CString outFileName2 = ModulePathHelper::GetModuleNameWithPath(inputPath2);
    EXPECT_EQ(outFileName2, res2);
}

HWTEST_F_L0(EcmaModuleTest, ConcatPandaFilePath)
{
    CString inputPath1 = "entry";
    CString res1 = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString outFileName1 = ModulePathHelper::ConcatPandaFilePath(inputPath1);
    EXPECT_EQ(outFileName1, res1);

    CString inputPath2 = "";
    CString res2 = "";
    CString outFileName2 = ModulePathHelper::ConcatPandaFilePath(inputPath2);
    EXPECT_EQ(outFileName2, res2);

    CString inputPath3 = "entry1";
    CString res3 = "/data/storage/el1/bundle/entry1/ets/modules.abc";
    CString outFileName3 = ModulePathHelper::ConcatPandaFilePath(inputPath3);
    EXPECT_EQ(outFileName3, res3);
}

HWTEST_F_L0(EcmaModuleTest, ParseFileNameToVMAName)
{
    CString inputFileName = "test.abc";
    CString outFileName = ModulePathHelper::ParseFileNameToVMAName(inputFileName);
    CString exceptOutFileName = "ArkTS Code:test.abc";
    EXPECT_EQ(outFileName, exceptOutFileName);

    inputFileName = "";
    outFileName = ModulePathHelper::ParseFileNameToVMAName(inputFileName);
    exceptOutFileName = "ArkTS Code";
    EXPECT_EQ(outFileName, exceptOutFileName);

    inputFileName = "libutil.z.so/util.js";
    outFileName = ModulePathHelper::ParseFileNameToVMAName(inputFileName);
    exceptOutFileName = "ArkTS Code:libutil.z.so/util.js";
    EXPECT_EQ(outFileName, exceptOutFileName);

    inputFileName = "libutil.HashMap.z.so/util.HashMap.js";
    outFileName = ModulePathHelper::ParseFileNameToVMAName(inputFileName);
    exceptOutFileName = "ArkTS Code:libhashmap.z.so/HashMap.js";
    EXPECT_EQ(outFileName, exceptOutFileName);

    inputFileName = "/data/storage/el1/bundle/com.example.application/ets/modules.abc";
    outFileName = ModulePathHelper::ParseFileNameToVMAName(inputFileName);
    exceptOutFileName = "ArkTS Code:com.example.application/ets/modules.abc";
    EXPECT_EQ(outFileName, exceptOutFileName);
}

HWTEST_F_L0(EcmaModuleTest, ConcatUnifiedOhmUrl)
{
    CString pkgName = "entry";
    CString path = "/Index";
    CString version = "1.0.0";
    CString outFileName = ModulePathHelper::ConcatUnifiedOhmUrl("", pkgName, "", path, version);
    CString exceptOutFileName = "&entry/src/main/Index&1.0.0";
    EXPECT_EQ(outFileName, exceptOutFileName);

    CString path2 = "Index";
    outFileName = ModulePathHelper::ConcatUnifiedOhmUrl("", path2, version);
    exceptOutFileName = "&Index&1.0.0";
    EXPECT_EQ(outFileName, exceptOutFileName);
}

HWTEST_F_L0(EcmaModuleTest, ConcatImportFileNormalizedOhmurl)
{
    CString recordPath = "&entry/ets/";
    CString requestName = "test";
    CString outFileName = ModulePathHelper::ConcatImportFileNormalizedOhmurl(recordPath, requestName, "");
    CString exceptOutFileName = "&entry/ets/test&";
    EXPECT_EQ(outFileName, exceptOutFileName);
}

HWTEST_F_L0(EcmaModuleTest, ConcatNativeSoNormalizedOhmurl)
{
    CString pkgName = "libentry.so";
    CString outFileName = ModulePathHelper::ConcatNativeSoNormalizedOhmurl("", "", pkgName, "");
    CString exceptOutFileName = "@normalized:Y&&&libentry.so&";
    EXPECT_EQ(outFileName, exceptOutFileName);
}

HWTEST_F_L0(EcmaModuleTest, ConcatNotSoNormalizedOhmurl)
{
    CString pkgName = "har";
    CString path = "Index";
    CString version = "1.0.0";
    CString outFileName = ModulePathHelper::ConcatNotSoNormalizedOhmurl("", "", pkgName, path, version);
    CString exceptOutFileName = "@normalized:N&&&har/Index&1.0.0";
    EXPECT_EQ(outFileName, exceptOutFileName);
}

HWTEST_F_L0(EcmaModuleTest, TranslateExpressionToNormalized)
{
    instance->SetBundleName("com.example.myapplication");
    CString baseFilename = "merge.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), baseFilename);

    CString requestPath = "@native:system.app";
    CString baseFileName = "";
    CString recordName = "";
    ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, recordName, requestPath);
    EXPECT_EQ(requestPath, "@native:system.app");

    requestPath = "@ohos:hilog";
    ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, recordName, requestPath);
    EXPECT_EQ(requestPath, "@ohos:hilog");

    requestPath = "@normalized:N&&&har/Index&1.0.0";
    ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, recordName, requestPath);
    EXPECT_EQ(requestPath, "@normalized:N&&&har/Index&1.0.0");

    requestPath = "@arkui-x.bridge";
    CString result = ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, recordName,
        requestPath);
    EXPECT_EQ(result, "@ohos:bridge");

    requestPath = "ets/Test";
    recordName = "&entry/ets/pages/Index&";
    pf->InsertJSRecordInfo("&entry/ets/Test&");
    result = ModulePathHelper::TranslateExpressionToNormalized(thread, pf.get(), baseFileName, recordName,
        requestPath);
    EXPECT_EQ(result, "&entry/ets/Test&");
}

HWTEST_F_L0(EcmaModuleTest, TranslateExpressionToNormalized2)
{
    CMap<CString, CMap<CString, CVector<CString>>> pkgList;
    CMap<CString, CVector<CString>> entryList;
    entryList["entry"] = {
        "packageName", "entry",
        "bundleName", "",
        "moduleName", "",
        "version", "",
        "entryPath", "src/main/",
        "isSO", "false"
    };
    entryList["har"] = {
        "packageName", "har",
        "bundleName", "",
        "moduleName", "",
        "version", "1.2.0",
        "entryPath", "Index.ets",
        "isSO", "false"
    };
    pkgList["entry"] = entryList;
    instance->SetpkgContextInfoList(pkgList);

    CString requestPath = "har";
    CString baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString result = ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, "",
        requestPath);
    EXPECT_EQ(result, "@normalized:N&&&har/Index&1.2.0");

    requestPath = "har/src/main/Test";
    result = ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, "",
        requestPath);
    EXPECT_EQ(result, "@normalized:N&&&har/src/main/Test&1.2.0");
}

HWTEST_F_L0(EcmaModuleTest, SplitNormalizedRecordName)
{
    CString requestPath = "&har/Index&1.0.0";
    CVector<CString> res = ModulePathHelper::SplitNormalizedRecordName(requestPath);
    int exceptCount = 5;
    EXPECT_EQ(res.size(), exceptCount);
    CString emptyStr = "";
    EXPECT_EQ(res[0], emptyStr);
    EXPECT_EQ(res[1], emptyStr);
    EXPECT_EQ(res[2], emptyStr);

    CString importPath = "har/Index";
    EXPECT_EQ(res[3], importPath);
    CString version = "1.0.0";
    EXPECT_EQ(res[4], version);
}

HWTEST_F_L0(EcmaModuleTest, ConcatPreviewTestUnifiedOhmUrl)
{
    CString bundleName = "";
    CString pkgName = "entry";
    CString path = "/.test/testability/pages/Index";
    CString version = "";
    CString exceptOutUrl = "&entry/.test/testability/pages/Index&";
    CString res = ModulePathHelper::ConcatPreviewTestUnifiedOhmUrl(bundleName, pkgName, path, version);
    EXPECT_EQ(res, exceptOutUrl);
}

HWTEST_F_L0(EcmaModuleTest, NeedTranslateToNormalized)
{
    CString requestName = "@ohos:hilog";
    bool res = ModulePathHelper::NeedTranslateToNormalized(requestName);
    EXPECT_EQ(res, false);

    requestName = "@app:com.example.myapplication/entry";
    res = ModulePathHelper::NeedTranslateToNormalized(requestName);
    EXPECT_EQ(res, false);

    requestName = "@bundle:com.example.myapplication/library";
    res = ModulePathHelper::NeedTranslateToNormalized(requestName);
    EXPECT_EQ(res, false);

    requestName = "@package:pkg_modules/.ohpm/json5@2.2.3/pkg_modules/json5/dist/index";
    res = ModulePathHelper::NeedTranslateToNormalized(requestName);
    EXPECT_EQ(res, false);

    requestName = "@normalized:N&&&har/Index&1.0.0";
    res = ModulePathHelper::NeedTranslateToNormalized(requestName);
    EXPECT_EQ(res, false);

    requestName = "json5";
    res = ModulePathHelper::NeedTranslateToNormalized(requestName);
    EXPECT_EQ(res, true);

    requestName = "library";
    res = ModulePathHelper::NeedTranslateToNormalized(requestName);
    EXPECT_EQ(res, true);
}

HWTEST_F_L0(EcmaModuleTest, ModuleLogger) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "a";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName2 = "b";
    module2->SetEcmaModuleRecordNameString(recordName2);
    JSHandle<JSTaggedValue> moduleRequest = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("c"));
    JSHandle<JSTaggedValue> importName = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("ccc"));
    JSHandle<JSTaggedValue> localName = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("ccc"));
    JSHandle<ImportEntry> importEntry = objectFactory->NewImportEntry(moduleRequest, importName,
                                                                      localName, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module2, importEntry, 0, 1);
    JSHandle<SourceTextModule> module3 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName3 = "c";
    module2->SetEcmaModuleRecordNameString(recordName3);
    ModuleLogger *moduleLogger = new ModuleLogger(thread->GetEcmaVM());
    moduleLogger->SetStartTime(recordName1);
    moduleLogger->SetEndTime(recordName1);
    moduleLogger->SetStartTime(recordName2);
    moduleLogger->SetEndTime(recordName2);
    moduleLogger->SetStartTime(recordName3);
    moduleLogger->InsertEntryPointModule(module1);
    moduleLogger->InsertParentModule(module1, module2);
    moduleLogger->InsertModuleLoadInfo(module2, module3, -1);
    moduleLogger->InsertModuleLoadInfo(module2, module3, 0);
    moduleLogger->PrintModuleLoadInfo();
    Local<JSValueRef> nativeFunc = SourceTextModule::GetRequireNativeModuleFunc(thread->GetEcmaVM(),
                                                                                module3->GetTypes());
    bool isFunc = nativeFunc->IsFunction(thread->GetEcmaVM());
    EXPECT_EQ(isFunc, false);
}

HWTEST_F_L0(EcmaModuleTest, GetRequireNativeModuleFunc) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    uint16_t registerNum = module->GetRegisterCounts();
    module->SetStatus(ecmascript::ModuleStatus::INSTANTIATED);
    module->SetRegisterCounts(registerNum);
    Local<JSValueRef> nativeFunc = SourceTextModule::GetRequireNativeModuleFunc(thread->GetEcmaVM(),
                                                                                module->GetTypes());
    bool isFunc = nativeFunc->IsFunction(thread->GetEcmaVM());
    EXPECT_EQ(isFunc, false);
}

/*
 * Feature: Module
 * Function: StoreModuleValue
 * SubFunction: StoreModuleValue/GetModuleValue
 * FunctionPoints: store a module export item in module
 * CaseDescription: Simulated implementation of "export foo as bar", set foo as "hello world",
 *                  use "import bar" in same js file
 */
HWTEST_F_L0(EcmaModuleTest, StoreModuleValue2)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    CString localName = "foo";
    CString exportName = "bar";
    CString value = "hello world";
    CString value2 = "hello world1";
    int32_t index = 1;

    JSHandle<JSTaggedValue> localNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSHandle<JSTaggedValue> exportNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName));
    JSHandle<LocalExportEntry> localExportEntry =
        objFactory->NewLocalExportEntry(exportNameHandle, localNameHandle, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                        SharedTypes::UNSENDABLE_MODULE);
    JSHandle<SourceTextModule> module = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry, 0, 1);

    JSHandle<JSTaggedValue> storeKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSHandle<JSTaggedValue> valueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value));
    JSHandle<JSTaggedValue> valueHandle1 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value2));
    module->StoreModuleValue(thread, storeKey, valueHandle);
    module->StoreModuleValue(thread, index, valueHandle1);
    JSHandle<JSTaggedValue> loadKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSTaggedValue loadValue = module->GetModuleValue(thread, loadKey.GetTaggedValue(), false);
    JSTaggedValue loadValue1 = module->GetModuleValue(thread, index, false);
    EXPECT_EQ(valueHandle.GetTaggedValue(), loadValue);
    EXPECT_EQ(valueHandle1.GetTaggedValue(), loadValue1);
}

HWTEST_F_L0(EcmaModuleTest, MakeAppArgs1) {
    std::vector<Local<JSValueRef>> arguments;
    CString soPath = "@normalized:Y&&&libentry.so&";
    CString moduleName = "entry";
    CString requestName = "@normalized:";
    arguments.emplace_back(StringRef::NewFromUtf8(thread->GetEcmaVM(), soPath.c_str()));
    SourceTextModule::MakeAppArgs(thread->GetEcmaVM(), arguments, soPath, moduleName, requestName);
    std::string res1 = arguments[0]->ToString(thread->GetEcmaVM())->ToString(thread->GetEcmaVM());
    std::string res2 = arguments[1]->ToString(thread->GetEcmaVM())->ToString(thread->GetEcmaVM());
    std::string res3 = arguments[2]->ToString(thread->GetEcmaVM())->ToString(thread->GetEcmaVM());
    EXPECT_TRUE(res1 == "entry");
    EXPECT_TRUE(res2 == "true");
    EXPECT_TRUE(res3 == "/entry");
}

HWTEST_F_L0(EcmaModuleTest, MakeAppArgs2) {
    std::vector<Local<JSValueRef>> arguments;
    CString soPath = "@app:com.example.myapplication/entry";
    CString moduleName = "entry";
    CString requestName = "@app:";
    arguments.emplace_back(StringRef::NewFromUtf8(thread->GetEcmaVM(), soPath.c_str()));
    SourceTextModule::MakeAppArgs(thread->GetEcmaVM(), arguments, soPath, moduleName, requestName);
    std::string res1 = arguments[0]->ToString(thread->GetEcmaVM())->ToString(thread->GetEcmaVM());
    std::string res2 = arguments[1]->ToString(thread->GetEcmaVM())->ToString(thread->GetEcmaVM());
    std::string res3 = arguments[2]->ToString(thread->GetEcmaVM())->ToString(thread->GetEcmaVM());
    EXPECT_TRUE(res1 == "entry");
    EXPECT_TRUE(res2 == "true");
    EXPECT_TRUE(res3 == "@app:com.example.myapplication");
}

HWTEST_F_L0(EcmaModuleTest, ConcatHspFileNameCrossBundle)
{
    CString bundleName = "com.example.application";
    CString moduleName = "entry";
    CString expectRes = "/data/storage/el1/bundle/com.example.application/entry/entry/ets/modules.abc";
    CString res = ModulePathHelper::ConcatHspFileNameCrossBundle(bundleName, moduleName);
    EXPECT_EQ(res, expectRes);
}

HWTEST_F_L0(EcmaModuleTest, ConcatHspFileName)
{
    CString moduleName = "entry";
    CString expectRes = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString res = ModulePathHelper::ConcatHspFileName(moduleName);
    EXPECT_EQ(res, expectRes);
}

HWTEST_F_L0(EcmaModuleTest, ParseNormalizedOhmUrl)
{
    CString baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString recordName = "&har/Index&1.0.0";
    CString requestName = "@normalized:N&&&har/src/main/page/Test&1.0.0";
    CString expectRes = "&har/src/main/page/Test&1.0.0";
    CString exceptBaseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString res = ModulePathHelper::ParseNormalizedOhmUrl(thread, baseFileName, recordName, requestName);
    EXPECT_EQ(res, expectRes);
    EXPECT_EQ(baseFileName, exceptBaseFileName);

    baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    requestName = "@normalized:N&hsp&&hsp/src/main/page/Test&1.0.0";
    expectRes = "&hsp/src/main/page/Test&1.0.0";
    exceptBaseFileName = "/data/storage/el1/bundle/hsp/ets/modules.abc";
    CString res2 = ModulePathHelper::ParseNormalizedOhmUrl(thread, baseFileName, recordName, requestName);
    EXPECT_EQ(res2, expectRes);
    EXPECT_EQ(baseFileName, exceptBaseFileName);

    baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    requestName = "@normalized:N&hsp&com.example.application&hsp/src/main/page/Test&1.0.0";
    exceptBaseFileName = "/data/storage/el1/bundle/com.example.application/hsp/hsp/ets/modules.abc";
    expectRes = "com.example.application&hsp/src/main/page/Test&1.0.0";
    CString res3 = ModulePathHelper::ParseNormalizedOhmUrl(thread, baseFileName, recordName, requestName);
    EXPECT_EQ(res3, expectRes);
    EXPECT_EQ(baseFileName, exceptBaseFileName);

    baseFileName = "";
    recordName = "&har/Index&1.0.0";
    requestName = "@normalized:N&&&har/src/main/page/Test&1.0.0";
    CString res4 = ModulePathHelper::ParseNormalizedOhmUrl(thread, baseFileName, recordName, requestName);
    EXPECT_EQ(baseFileName, "");
}

HWTEST_F_L0(EcmaModuleTest, GetModuleNameWithBaseFile)
{
    CString baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString expectRes = "entry";
    CString res = ModulePathHelper::GetModuleNameWithBaseFile(baseFileName);
    EXPECT_EQ(res, expectRes);

    baseFileName = "/data/storage/el1/bundle/com.example.application/hsp/hsp/ets/modules.abc";
    expectRes = "hsp";
    res = ModulePathHelper::GetModuleNameWithBaseFile(baseFileName);
    EXPECT_EQ(res, expectRes);
}

HWTEST_F_L0(EcmaModuleTest, GetBundleNameWithRecordName)
{
    CString recordName = "com.example.myapplication/library";
    CString expectRes = "com.example.myapplication";
    CString res = ModulePathHelper::GetBundleNameWithRecordName(instance, recordName);
    EXPECT_EQ(res, expectRes);

    CMap<CString, CMap<CString, CVector<CString>>> list;
    CMap<CString, CVector<CString>> childList;
    list["hsp"] = childList;
    instance->SetpkgContextInfoList(list);

    recordName = "&hsp&com.example.application&hsp/src/main/page/Test&1.0.0";
    expectRes = "com.example.application";
    res = ModulePathHelper::GetBundleNameWithRecordName(instance, recordName);
    EXPECT_EQ(res, expectRes);

    instance->SetBundleName("com.example1.application");
    recordName = "&har/src/main/page/Test&1.0.0";
    expectRes = "com.example1.application";
    res = ModulePathHelper::GetBundleNameWithRecordName(instance, recordName);
    EXPECT_EQ(res, expectRes);
}

HWTEST_F_L0(EcmaModuleTest, TransformToNormalizedOhmUrl)
{
    CMap<CString, CMap<CString, CVector<CString>>> pkgList;
    CMap<CString, CVector<CString>> entryList;
    entryList["entry"] = {
        "packageName", "entry",
        "bundleName", "",
        "moduleName", "",
        "version", "",
        "entryPath", "src/main/",
        "isSO", "false"
    };
    entryList["har"] = {
        "packageName", "har",
        "bundleName", "",
        "moduleName", "",
        "version", "1.2.0",
        "entryPath", "",
        "isSO", "false"
    };
    pkgList["entry"] = entryList;
    CMap<CString, CVector<CString>> ohosTestList;
    ohosTestList["ohosTest"] = {
        "packageName", "ohosTest",
        "bundleName", "",
        "moduleName", "",
        "version", "",
        "entryPath", "src/",
        "isSO", "false"
    };
    pkgList["ohosTest"] = ohosTestList;
    instance->SetpkgContextInfoList(pkgList);

    CString inputFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString outBaseFileName = "";
    CString entryPoint = "ENTRY_MAIN_FUNCTION";
    CString res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "ENTRY_MAIN_FUNCTION");

    outBaseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    entryPoint = "com.example.myapplication/entry@har/ets/pages/Index";
    res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "&har/src/main/ets/pages/Index&1.2.0");

    outBaseFileName = "/data/storage/el1/bundle/ohosTest/ets/modules.abc";
    entryPoint = "com.example.myapplication/ohosTest/ets/pages/Index";
    res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "&ohosTest/src/ets/pages/Index&");

    outBaseFileName = "/data/storage/el1/bundle/entry/.test/ets/modules.abc";
    entryPoint = "com.example.myapplication/entry/.test/ets/pages/Index";
    res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "&entry/.test/ets/pages/Index&");
}

HWTEST_F_L0(EcmaModuleTest, TransformToNormalizedOhmUrl2)
{
    CString inputFileName = "/data/storage/el1/bundle/hsp/ets/modules.abc";
    CString outBaseFileName = "/data/storage/el1/bundle/com.example.application/hsp/hsp/ets/modules.abc";
    CString entryPoint = "com.example.myapplication/hsp/ets/pages/Index";
    CString res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "com.example.myapplication/hsp/ets/pages/Index");
}

HWTEST_F_L0(EcmaModuleTest, TranslateExpressionInputWithEts)
{
    instance->SetBundleName("com.example.myapplication");
    CString baseFilename = "merge.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), baseFilename);

    CString baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString requestName = "ets/Test";
    CString recordName = "com.example.myapplication/entry/ets/pages/Index";
    CString expectRes = "com.example.myapplication/entry/ets/Test";
    CString result = ModulePathHelper::TranslateExpressionInputWithEts(thread, pf.get(), baseFileName, requestName);
    EXPECT_EQ(result, "");

    pf->InsertJSRecordInfo(expectRes);
    result = ModulePathHelper::TranslateExpressionInputWithEts(thread, pf.get(), baseFileName, requestName);
    EXPECT_EQ(result, expectRes);
}

HWTEST_F_L0(EcmaModuleTest, ReviseLoadedModuleCount1) {
    CString moduleName = "testModule";
    ModuleDeregister::ReviseLoadedModuleCount(thread, moduleName);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    bool res = moduleManager->IsLocalModuleLoaded(moduleName);
    EXPECT_EQ(res, false);
}

HWTEST_F_L0(EcmaModuleTest, IncreaseRegisterCounts)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    std::set<CString> increaseModule;

    ModuleDeregister::IncreaseRegisterCounts(thread, module, increaseModule);
    module->SetRegisterCounts(INT8_MAX);
    ModuleDeregister::IncreaseRegisterCounts(thread, module, increaseModule);
    module2->SetRegisterCounts(INT8_MAX);
    ModuleDeregister::IncreaseRegisterCounts(thread, module2, increaseModule);
    module2->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
    ModuleDeregister::IncreaseRegisterCounts(thread, module2, increaseModule);
}

HWTEST_F_L0(EcmaModuleTest, DecreaseRegisterCounts)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    std::set<CString> decreaseModule;

    ModuleDeregister::DecreaseRegisterCounts(thread, module, decreaseModule);
    module->SetRegisterCounts(INT8_MAX);
    ModuleDeregister::DecreaseRegisterCounts(thread, module, decreaseModule);
    module2->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    ModuleDeregister::DecreaseRegisterCounts(thread, module2, decreaseModule);
}

HWTEST_F_L0(EcmaModuleTest, GetSendableModuleValueImpl) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    int32_t index = 2;
    JSTaggedValue currentModule1 = module1.GetTaggedValue();
    manager1->GetSendableModuleValueImpl(thread, index, currentModule1);

    SharedModuleManager* manager2 = SharedModuleManager::GetInstance();
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module2 = moduleManager->HostGetImportedModule("module_test_module_test_C");
    JSTaggedValue currentModule2 = module2.GetTaggedValue();
    manager2->GetSendableModuleValueImpl(thread, index, currentModule2);
}

HWTEST_F_L0(EcmaModuleTest, GetLazySendableModuleValueImpl) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    int32_t index = 2;
    JSTaggedValue currentModule1 = module1.GetTaggedValue();
    manager1->GetLazySendableModuleValueImpl(thread, index, currentModule1);
}

HWTEST_F_L0(EcmaModuleTest, ResolveImportedModuleWithMerge) {
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    module->SetSharedType(SharedTypes::SHARED_MODULE);

    CString recordName2 = "testModule";
    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    bool executeFromJob = false;
    manager1->ResolveImportedModuleWithMerge(thread, baseFileName.c_str(), recordName2, executeFromJob);
}

HWTEST_F_L0(EcmaModuleTest, ResolveImportedModuleWithMerge2) {
    CString moduleName1;
    CString recordName1;

    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    bool executeFromJob = false;
    manager1->ResolveImportedModuleWithMerge(thread, moduleName1, recordName1, executeFromJob);
}

HWTEST_F_L0(EcmaModuleTest, IsInstaniatedSModule) {
    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    bool res = manager1->IsInstaniatedSModule(thread, module);
    EXPECT_EQ(res, true);
}

HWTEST_F_L0(EcmaModuleTest, ProcessModuleLoadInfoForESM)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "a";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ImportEntry> importEntry =
        objectFactory->NewImportEntry(val, val, val, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module1, importEntry, 0, 1);

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName2 = "b";
    module2->SetEcmaModuleRecordNameString(recordName2);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module2->SetLocalExportEntries(thread, localExportEntries);
    module2->StoreModuleValue(thread, 0, val);
    module2->SetStatus(ModuleStatus::EVALUATED);
    ModuleLogger *moduleLogger = new ModuleLogger(thread->GetEcmaVM());
    thread->GetCurrentEcmaContext()->SetModuleLogger(moduleLogger);
    // test ResolvedIndexBinding
    JSHandle<ResolvedIndexBinding> indexBinding =
        objectFactory->NewResolvedIndexBindingRecord(module2, 0);
    JSTaggedValue res1 = ModuleTools::ProcessModuleLoadInfo(thread, module1, indexBinding.GetTaggedValue(), 0);
    EXPECT_EQ(res1, val.GetTaggedValue());

    JSTaggedValue res2 = ModuleTools::ProcessModuleNameSpaceLoadInfo(thread, module1, module2);
    EXPECT_NE(res2, JSTaggedValue::Exception());
    thread->GetCurrentEcmaContext()->SetModuleLogger(nullptr);
    delete moduleLogger;
}

HWTEST_F_L0(EcmaModuleTest, ProcessModuleLoadInfoForCJS)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "a";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ImportEntry> importEntry =
        objectFactory->NewImportEntry(val, val, val, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module1, importEntry, 0, 1);

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName2 = "cjs";
    module2->SetEcmaModuleRecordNameString(recordName2);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module2->SetLocalExportEntries(thread, localExportEntries);
    module2->StoreModuleValue(thread, 0, val);
    module2->SetTypes(ModuleTypes::CJS_MODULE);
    module2->SetStatus(ModuleStatus::EVALUATED);
    JSHandle<CjsModule> moduleCjs = objectFactory->NewCjsModule();
    JSHandle<JSTaggedValue> recordName2Hdl =
        JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(recordName2.c_str()));
    JSHandle<JSTaggedValue> baseFileNameHdl =
        JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(baseFileName.c_str()));
    CjsModule::InitializeModule(thread, moduleCjs, recordName2Hdl, baseFileNameHdl);
    CjsModule::PutIntoCache(thread, moduleCjs, recordName2Hdl);

    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution =
        JSHandle<JSTaggedValue>::Cast(objectFactory->NewResolvedBindingRecord(module2, val));
    envRec->Set(thread, 0, resolution);
    module1->SetEnvironment(thread, envRec);

    ModuleLogger *moduleLogger = new ModuleLogger(thread->GetEcmaVM());
    thread->GetCurrentEcmaContext()->SetModuleLogger(moduleLogger);

    // test ResolvedBinding
    JSTaggedValue res1 = ModuleTools::ProcessModuleNameSpaceLoadInfo(thread, module1, module2);
    EXPECT_NE(res1, JSTaggedValue::Exception());

    JSTaggedValue res2 = ModuleTools::ProcessModuleLoadInfo(thread, module1, resolution.GetTaggedValue(), 0);
    EXPECT_EQ(res2, JSTaggedValue::Exception());
    thread->GetCurrentEcmaContext()->SetModuleLogger(nullptr);
    delete moduleLogger;
}

HWTEST_F_L0(EcmaModuleTest, ProcessModuleLoadInfoForNativeModule)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "a";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ImportEntry> importEntry =
        objectFactory->NewImportEntry(val, val, val, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module1, importEntry, 0, 1);

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName2 = "nativeModule";
    module2->SetEcmaModuleRecordNameString(recordName2);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module2->SetLocalExportEntries(thread, localExportEntries);
    module2->StoreModuleValue(thread, 0, val);
    module2->SetTypes(ModuleTypes::NATIVE_MODULE);
    module2->SetStatus(ModuleStatus::EVALUATED);

    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(
        objectFactory->NewResolvedBindingRecord(module2, val));
    envRec->Set(thread, 0, resolution);
    module1->SetEnvironment(thread, envRec);

    ModuleLogger *moduleLogger = new ModuleLogger(thread->GetEcmaVM());
    thread->GetCurrentEcmaContext()->SetModuleLogger(moduleLogger);

    JSTaggedValue res1 = ModuleTools::ProcessModuleLoadInfo(thread, module1, resolution.GetTaggedValue(), 0);
    EXPECT_EQ(res1, JSTaggedValue::Exception());

    JSTaggedValue res2 = ModuleTools::ProcessModuleNameSpaceLoadInfo(thread, module1, module2);
    EXPECT_NE(res2, JSTaggedValue::Exception());
    thread->GetCurrentEcmaContext()->SetModuleLogger(nullptr);
    delete moduleLogger;
}

HWTEST_F_L0(EcmaModuleTest, ResolvedBindingForLog)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "a";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ImportEntry> importEntry =
        objectFactory->NewImportEntry(val, val, val, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module1, importEntry, 0, 1);

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName2 = "b";
    module2->SetEcmaModuleRecordNameString(recordName2);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module2->SetLocalExportEntries(thread, localExportEntries);
    module2->StoreModuleValue(thread, 0, val);
    module2->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName2, module2.GetTaggedValue());

    ModuleLogger *moduleLogger = new ModuleLogger(thread->GetEcmaVM());
    thread->GetCurrentEcmaContext()->SetModuleLogger(moduleLogger);
    // test ResolvedRecordIndexBinding
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName2);
    JSHandle<EcmaString> baseFileNameHdl = objectFactory->NewFromUtf8(baseFileName);
    JSHandle<ResolvedRecordIndexBinding> recordIndexBinding =
        objectFactory->NewSResolvedRecordIndexBindingRecord(recordNameHdl, baseFileNameHdl, 0);
    JSTaggedValue res1 = ModuleTools::ProcessModuleLoadInfo(thread, module1, recordIndexBinding.GetTaggedValue(), 0);
    EXPECT_EQ(res1, val.GetTaggedValue());

    // test ResolvedRecordBinding
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);
    JSTaggedValue res2 = ModuleTools::ProcessModuleLoadInfo(thread, module1, nameBinding.GetTaggedValue(), 0);
    EXPECT_EQ(res2, JSTaggedValue::Exception());
    thread->GetCurrentEcmaContext()->SetModuleLogger(nullptr);
    delete moduleLogger;
}

HWTEST_F_L0(EcmaModuleTest, ToStringWithPrecision)
{
    std::string res = ModuleLogger::ToStringWithPrecision(4.550, 3);
    EXPECT_EQ(res, "4.55");
}

}  // namespace panda::test
