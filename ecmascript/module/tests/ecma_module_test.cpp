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

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "libpandafile/class_data_accessor-inl.h"

#include "ecmascript/base/path_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/linked_hash_table.h"


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
        objFactory->NewLocalExportEntry(exportNameHandle, localNameHandle, LocalExportEntry::LOCAL_DEFAULT_INDEX);
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
        exportLocalNameHandle, LocalExportEntry::LOCAL_DEFAULT_INDEX);
    JSHandle<SourceTextModule> moduleExport = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, moduleExport, localExportEntry, 0, 1);
    // store module value
    JSHandle<JSTaggedValue> exportValueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportValue));
    moduleExport->StoreModuleValue(thread, exportLocalNameHandle, exportValueHandle);

    JSTaggedValue importDefaultValue =
        moduleExport->GetModuleValue(thread, exportLocalNameHandle.GetTaggedValue(), false);
    EXPECT_EQ(exportValueHandle.GetTaggedValue(), importDefaultValue);
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
    moduleManager->AddResolveImportedModule(baseFileName.c_str(), moduleRecord);
    JSHandle<JSTaggedValue> res = moduleManager->HostResolveImportedModule(baseFileName.c_str());

    EXPECT_EQ(moduleRecord->GetRawData(), res.GetTaggedValue().GetRawData());
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
    JSHandle<EcmaString> moduleFilename = objectFactory->NewFromUtf8(baseFileName);
    module->SetEcmaModuleFilename(thread, moduleFilename);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->AddResolveImportedModule(baseFileName, JSHandle<JSTaggedValue>::Cast(module));
    JSHandle<ModuleNamespace> np =
    ModuleNamespace::ModuleNamespaceCreate(thread, JSHandle<JSTaggedValue>::Cast(module), localExportEntries);
    EXPECT_FALSE(np->IsExtensible());
    EXPECT_TRUE(ModuleNamespace::PreventExtensions());
}

HWTEST_F_L0(EcmaModuleTest, Instantiate_Evaluate_GetNamespace_SetNamespace)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    module->SetStatus(ModuleStatus::UNINSTANTIATED);
    ModuleRecord::Instantiate(thread, JSHandle<JSTaggedValue>(module));
    int res = ModuleRecord::Evaluate(thread, JSHandle<JSTaggedValue>(module));
    ModuleRecord::GetNamespace(module.GetTaggedValue());
    ModuleRecord::SetNamespace(thread, module.GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_TRUE(res == SourceTextModule::UNDEFINED_INDEX);
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
    const CUnorderedMap<CString, JSPandaFile::JSRecordInfo> &recordInfo = pf->GetJSRecordInfo();
    // Test moduleRequestName is npm package
    CString moduleRecordName = "node_modules/0/moduleTest4/index";
    CString moduleRequestName = "json/index";
    CString result = "node_modules/0/moduleTest4/node_modules/json/index";
    JSPandaFile::JSRecordInfo info;
    info.npmPackageName = "node_modules/0/moduleTest4";
    const_cast<CUnorderedMap<CString, JSPandaFile::JSRecordInfo> &>(recordInfo).insert({moduleRecordName, info});
    const_cast<CUnorderedMap<CString, JSPandaFile::JSRecordInfo> &>(recordInfo).insert({result, info});
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename, moduleRecordName,
                                                             moduleRequestName);
    EXPECT_EQ(result, entryPoint);
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

HWTEST_F_L0(EcmaModuleTest, ParseOhmUrl)
{
    // old pages url
    instance->SetBundleName("com.bundleName.test");
    instance->SetModuleName("moduleName");
    CString inputFileName = "moduleName/ets/pages/index.abc";
    CString outFileName = "";
    CString res1 = "com.bundleName.test/moduleName/ets/pages/index";
    CString entryPoint;
    ModulePathHelper::ParseOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res1);
    EXPECT_EQ(outFileName, "");

    // new pages url
    inputFileName = "@bundle:com.bundleName.test/moduleName/ets/pages/index.abc";
    ModulePathHelper::ParseOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res1);
    EXPECT_EQ(outFileName, "/data/storage/el1/bundle/moduleName/ets/modules.abc");

    // new pages url Intra-application cross hap
    inputFileName = "@bundle:com.bundleName.test/moduleName1/ets/pages/index.abc";
    CString outRes = "/data/storage/el1/bundle/moduleName1/ets/modules.abc";
    CString res2 = "com.bundleName.test/moduleName1/ets/pages/index";
    ModulePathHelper::ParseOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res2);
    EXPECT_EQ(outFileName, outRes);

    // new pages url Cross-application
    inputFileName = "@bundle:com.bundleName.test1/moduleName1/ets/pages/index.abc";
    CString outRes1 = "/data/storage/el1/bundle/com.bundleName.test1/moduleName1/moduleName1/ets/modules.abc";
    CString res3 = "com.bundleName.test1/moduleName1/ets/pages/index";
    ModulePathHelper::ParseOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res3);
    EXPECT_EQ(outFileName, outRes1);

    // worker url Intra-application cross hap
    inputFileName = "/data/storage/el1/bundle/entry/ets/mainAbility.abc";
    CString outRes2 = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString res4 = "com.bundleName.test/entry/ets/mainAbility";
    ModulePathHelper::ParseOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res4);
    EXPECT_EQ(outFileName, outRes2);

    // worker url
    outFileName = "";
    inputFileName = "/data/storage/el1/bundle/moduleName/ets/mainAbility.abc";
    CString res5 = "com.bundleName.test/moduleName/ets/mainAbility";
    ModulePathHelper::ParseOhmUrl(instance, inputFileName, outFileName, entryPoint);
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
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();

    CString inputFileName = "moduleName/ets/pages/index.abc";
    CString resName1 = "moduleName/ets/pages/";
    JSHandle<EcmaString> res1 = objectFactory->NewFromUtf8(resName1);
    JSHandle<EcmaString> outFileName = PathHelper::ResolveDirPath(thread, inputFileName);
    EXPECT_EQ(outFileName, res1);

    inputFileName = "moduleName\\ets\\pages\\index.abc";
    CString resName2 = "moduleName\\ets\\pages\\";
    JSHandle<EcmaString> res2 = objectFactory->NewFromUtf8(resName2);
    outFileName = PathHelper::ResolveDirPath(thread, inputFileName);
    EXPECT_EQ(outFileName, res2);

    inputFileName = "cjs";
    CString resName3 = "";
    JSHandle<EcmaString> res3 = objectFactory->NewFromUtf8(resName3);
    outFileName = PathHelper::ResolveDirPath(thread, inputFileName);
    EXPECT_EQ(outFileName, res3);
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
}  // namespace panda::test
