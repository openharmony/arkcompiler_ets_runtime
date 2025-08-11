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
#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
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
#include "ecmascript/module/accessor/module_data_accessor.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/module/module_tools.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/module/module_value_accessor.h"
#include "ecmascript/module/module_resolver.h"
#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/dfx/native_module_failure_info.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;
using namespace testing::ext;

namespace panda::test {
using FunctionCallbackInfo = JSHandle<JSTaggedValue>(*)(JsiRuntimeCallInfo *);
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

    static JSHandle<JSTaggedValue> MockRequireNapiFailure(JsiRuntimeCallInfo *runtimeCallInfo);
    static JSHandle<JSTaggedValue> MockRequireNapiValue(JsiRuntimeCallInfo *runtimeCallInfo);
    static JSHandle<JSTaggedValue> MockRequireNapiException(JsiRuntimeCallInfo *runtimeCallInfo);

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

JSHandle<JSTaggedValue> EcmaModuleTest::MockRequireNapiFailure(JsiRuntimeCallInfo *runtimeCallInfo)
{
    EcmaVM *vm = runtimeCallInfo->GetVM();
    vm->SetErrorInfoEnhance(true);
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, thread->GlobalConstants()->GetHandledUndefined());
    ecmascript::ThreadManagedScope managedScope(thread);
    if (EcmaVM::GetErrorInfoEnhance()) {
        JSHandle<NativeModuleFailureInfo> nativeModuleErrorFailureInfo =
            NativeModuleFailureInfo::CreateNativeModuleFailureInfo(vm, "NativeModuleFailureInfo");
        return JSHandle<JSTaggedValue>::Cast(nativeModuleErrorFailureInfo);
    }
    return JSHandle<JSTaggedValue>(thread->GlobalConstants()->GetHandledUndefined());
}

JSHandle<JSTaggedValue> EcmaModuleTest::MockRequireNapiValue(JsiRuntimeCallInfo *runtimeCallInfo)
{
    EcmaVM *vm = runtimeCallInfo->GetVM();
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> current(factory->NewFromUtf8("exportVal"));
    return current;
}

JSHandle<JSTaggedValue> EcmaModuleTest::MockRequireNapiException(JsiRuntimeCallInfo *runtimeCallInfo)
{
    EcmaVM *vm = runtimeCallInfo->GetVM();
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> message(factory->NewFromUtf8("load native module failed."));
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::ERROR, "load native module failed.", StackCheck::NO);
    JSHandle<JSTaggedValue> code(thread, JSTaggedValue(ecmascript::containers::ErrorFlag::REFERENCE_ERROR));
    JSHandle<EcmaString> key = factory->NewFromUtf8("code");
    JSHandle<EcmaString> name = factory->NewFromUtf8("name");
    JSHandle<EcmaString> value = factory->NewFromUtf8("BusinessError");
    JSObject::CreateDataPropertyOrThrow(thread, error, JSHandle<JSTaggedValue>::Cast(key), code);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSObject::CreateDataPropertyOrThrow(thread, error, JSHandle<JSTaggedValue>::Cast(name),
                                        JSHandle<JSTaggedValue>::Cast(value));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSTaggedValue err = error.GetTaggedValue();
    if (thread->HasPendingException()) {
        LOG_ECMA(DEBUG) << "An exception has already occurred before, keep old exception here.";
    } else {
        thread->SetException(err);
    }

    return message;
}

class MockModuleValueAccessor : public ModuleValueAccessor {
public:
    static JSTaggedValue GetModuleNamespaceInternal(JSThread *thread, int32_t index, JSTaggedValue curModule)
    {
        return ModuleValueAccessor::GetModuleNamespaceInternal(thread, index, curModule);
    }
    template <bool isLazy>
    static JSTaggedValue GetSendableModuleValueOuterInternal(JSThread *thread, int32_t index, JSTaggedValue curModule)
    {
        return ModuleValueAccessor::GetSendableModuleValueOuterInternal<isLazy>(thread, index, curModule);
    }
    template <bool isLazy>
    static JSTaggedValue GetModuleValueFromRecordIndexBinding(JSThread *thread, JSHandle<SourceTextModule> module,
        JSTaggedValue resolvedBinding, bool isSendable)
    {
        GetModuleValueFromBindingInfo info { thread, module, resolvedBinding, 0, isSendable };
        return ModuleValueAccessor::GetModuleValueFromRecordIndexBinding<isLazy>(info);
    }

    template <bool isLazy>
    static JSTaggedValue GetModuleValueFromIndexBinding(JSThread *thread, JSHandle<SourceTextModule> module,
        JSTaggedValue resolvedBinding, bool isSendable)
    {
        GetModuleValueFromBindingInfo info { thread, module, resolvedBinding, 0, isSendable };
        return ModuleValueAccessor::GetModuleValueFromIndexBinding<isLazy>(info);
    }

    template <bool isLazy>
    static JSTaggedValue GetModuleValueFromRecordBinding(JSThread *thread, JSHandle<SourceTextModule> module,
        JSTaggedValue resolvedBinding, int32_t index, bool isSendable)
    {
        GetModuleValueFromBindingInfo info { thread, module, resolvedBinding, index, isSendable };
        return ModuleValueAccessor::GetModuleValueFromRecordBinding<isLazy>(info);
    }
    template <bool isLazy>
    static JSHandle<SourceTextModule> GetResolvedModuleFromRecordIndexBinding(JSThread *thread,
        JSHandle<SourceTextModule> module, JSHandle<ResolvedRecordIndexBinding> binding)
    {
        return ModuleValueAccessor::GetResolvedModule<isLazy, ResolvedRecordIndexBinding>(
            thread, module, binding, ModulePathHelper::Utf8ConvertToString(thread, binding->GetModuleRecord(thread)));
    }
    template <bool isLazy>
    static JSHandle<SourceTextModule> GetResolvedModuleFromRecordBinding(JSThread *thread,
        JSHandle<SourceTextModule> module, JSHandle<ResolvedRecordBinding> binding)
    {
        return ModuleValueAccessor::GetResolvedModule<isLazy, ResolvedRecordBinding>(
            thread, module, binding, ModulePathHelper::Utf8ConvertToString(thread, binding->GetModuleRecord(thread)));
    }
};
class MockDeprecatedModuleValueAccessor : public DeprecatedModuleValueAccessor {
public:
    static JSTaggedValue GetModuleNamespaceInternal(JSThread *thread, JSTaggedValue localName, JSTaggedValue curModule)
    {
        return DeprecatedModuleValueAccessor::GetModuleNamespaceInternal(thread, localName, curModule);
    }
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
    JSHandle<TaggedArray> importEntries(thread, module->GetImportEntries(thread));
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
    JSHandle<TaggedArray> localExportEntries(thread, module->GetLocalExportEntries(thread));
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
    JSHandle<TaggedArray> indirectExportEntries(thread, module->GetIndirectExportEntries(thread));
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
    JSHandle<TaggedArray> startExportEntries(thread, module->GetStarExportEntries(thread));
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
    SourceTextModule::StoreModuleValue(thread, module, storeKey, valueHandle);

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
    SourceTextModule::StoreModuleValue(thread, moduleExport, exportLocalNameHandle, exportValueHandle);

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
    SourceTextModule::StoreModuleValue(thread, module, storeKey, valueHandle);

    JSHandle<JSTaggedValue> storeKey2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName2));
    JSHandle<JSTaggedValue> valueHandle2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value2));
    SourceTextModule::StoreModuleValue(thread, module, storeKey2, valueHandle2);

    // FindByExport cannot find key from exportEntries, returns Hole()
    JSHandle<JSTaggedValue> loadKey1 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName3));
    JSTaggedValue loadValue1 = module->GetModuleValue(thread, loadKey1.GetTaggedValue(), false);
    EXPECT_EQ(JSTaggedValue::Hole(), loadValue1);

    // FindByExport retrieves the key from exportEntries and returns the value corresponding to the key
    JSHandle<JSTaggedValue> loadKey2 = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName1));
    JSTaggedValue loadValue2 = module->GetModuleValue(thread, loadKey2.GetTaggedValue(), false);
    EXPECT_EQ(valueHandle.GetTaggedValue(), loadValue2);
}

HWTEST_F_L0(EcmaModuleTest, ReformatPath)
{
    // start with pkg_modules
    CString requestPath = "pkg_modules@entry.@hw-agconnect.hmcore";
    CString entryPoint = ModulePathHelper::ReformatPath(requestPath);
    EXPECT_EQ("@entry.@hw-agconnect.hmcore", entryPoint);

    // start with @package:
    requestPath = "@package:pkg_modules@entry.@hw-agconnect.hmcore";
    entryPoint = ModulePathHelper::ReformatPath(requestPath);
    EXPECT_EQ("@entry.@hw-agconnect.hmcore", entryPoint);

    // start with @app:
    requestPath = "@app:bundleName/moduleName/lib*.so";
    entryPoint = ModulePathHelper::ReformatPath(requestPath);
    EXPECT_EQ("lib*.so", entryPoint);

    // no special prefix
    requestPath = "bundleName/moduleName/lib*.so";
    entryPoint = ModulePathHelper::ReformatPath(requestPath);
    EXPECT_EQ("bundleName/moduleName/lib*.so", entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, ParseUrl)
{
    CString recordName = "com.bundleName.test";
    CString res = ModulePathHelper::ParseUrl(instance, recordName);
    EXPECT_EQ(res, CString());
}

HWTEST_F_L0(EcmaModuleTest, MakeNewRecord)
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

    // is not start with BUNDLE_INSTALL_PATH
    CString moduleRecordName = "moduleTest1";
    CString moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    CString result = "requestModuleName1";
    CString entryPoint = ModulePathHelper::MakeNewRecord(thread, pf.get(), baseFilename, moduleRecordName,
                                                             moduleRequestName);
    EXPECT_EQ(result, entryPoint);

    // start with BUNDLE_INSTALL_PATH, is bundle pack
    baseFilename = "/data/storage/el1/bundle/";
    moduleRecordName = "moduleTest1";
    moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    result = "requestModuleName1";
    entryPoint = ModulePathHelper::MakeNewRecord(thread, pf.get(), baseFilename, moduleRecordName,
                                                             moduleRequestName);
    EXPECT_EQ(result, entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, FindPackageInTopLevelWithNamespace)
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

    // start with pkg_modules, pos equal CString::npos
    CString moduleRecordName = "pkg_modules@entry.@hw-agconnect.hmcore";
    CString moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    CString entryPoint = ModulePathHelper::FindPackageInTopLevelWithNamespace(
        pf.get(), moduleRequestName, moduleRecordName);
    EXPECT_EQ(CString(), entryPoint);

    // start with pkg_modules, pos not equal CString::npos
    moduleRecordName = "pkg_modules@entry.@hw-agconnect.hmcore/test";
    moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    entryPoint = ModulePathHelper::FindPackageInTopLevelWithNamespace(pf.get(), moduleRequestName, moduleRecordName);
    EXPECT_EQ(CString(), entryPoint);

    // moduleName has @
    moduleRecordName = "@bundle:com.bundleName.test/@test";
    moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    entryPoint = ModulePathHelper::FindPackageInTopLevelWithNamespace(pf.get(), moduleRequestName, moduleRecordName);
    EXPECT_EQ(CString(), entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, ParseOhpmPackage)
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

    // start with pkg_modules
    CString moduleRecordName = "pkg_modules@entry.@hw-agconnect.hmcore1";
    CString moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    pf->InsertJSRecordInfo(moduleRecordName);
    CString entryPoint = ModulePathHelper::ParseOhpmPackage(pf.get(), moduleRecordName, moduleRequestName);
    EXPECT_EQ(CString(), entryPoint);

    // start with pkg_modules, packageName has pkg_modules
    CString moduleRecordName2 = "pkg_modules@entry.@hw-agconnect.hmcore2";
    CString moduleRequestName2 = "@bundle:com.bundleName.test/moduleName/requestModuleName1";
    CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo*> &recordInfo =
        const_cast<CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo*>&>(pf->GetJSRecordInfo());
    JSPandaFile::JSRecordInfo *info = new JSPandaFile::JSRecordInfo();
    info->npmPackageName = moduleRecordName2;
    recordInfo.insert({std::string_view(moduleRecordName2.c_str(), moduleRecordName2.size()), info});
    entryPoint = ModulePathHelper::ParseOhpmPackage(pf.get(), moduleRecordName2, moduleRequestName2);
    EXPECT_EQ(CString(), entryPoint);

    // delete info
    delete info;
    recordInfo.erase(moduleRecordName2);
}

HWTEST_F_L0(EcmaModuleTest, FindPackageInTopLevel)
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

    // start with packagePath
    CString moduleRequestName = "@bundle:com.bundleName.test/moduleName/requestModuleName";
    CString packagePath = "pkg_modules";
    CString result = "pkg_modules/0/@bundle:com.bundleName.test/moduleName/requestModuleName";
    pf->InsertJSRecordInfo(result);
    CString entryPoint = ModulePathHelper::FindPackageInTopLevel(pf.get(), moduleRequestName, packagePath);
    EXPECT_EQ(result, entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, NeedTranstale)
{
    // start with @bundle
    CString requestName = "@bundle:com.bundleName.test/moduleName/requestModuleName";
    bool res = ModulePathHelper::NeedTranstale(requestName);
    EXPECT_EQ(res, false);

    // start with @package:
    requestName = "@package:test";
    res = ModulePathHelper::NeedTranstale(requestName);
    EXPECT_EQ(res, false);

    // start with .
    requestName = "./test";
    res = ModulePathHelper::NeedTranstale(requestName);
    EXPECT_EQ(res, false);

    // start with @, has :
    requestName = "@test:";
    res = ModulePathHelper::NeedTranstale(requestName);
    EXPECT_EQ(res, false);

    // start with @, don't has :
    requestName = "@test";
    res = ModulePathHelper::NeedTranstale(requestName);
    EXPECT_EQ(res, true);

    // other branches
    requestName = "test";
    res = ModulePathHelper::NeedTranstale(requestName);
    EXPECT_EQ(res, true);
}

HWTEST_F_L0(EcmaModuleTest, TranstaleExpressionInput)
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

    // start with @arkui-x.
    CString requestPath = "@arkui-x.test/moduleName/requestModuleName";
    ModulePathHelper::TranstaleExpressionInput(pf.get(), requestPath);
    EXPECT_EQ(requestPath, "@ohos:test/moduleName/requestModuleName");

    requestPath = "@ohos.app:@native.system.app";
    CString fieldName = requestPath;
    pf->InsertNpmEntries(requestPath, fieldName);
    ModulePathHelper::TranstaleExpressionInput(pf.get(), requestPath);
    EXPECT_EQ(requestPath, "@ohos:app:@native.system.app");
}

HWTEST_F_L0(EcmaModuleTest, ParseFileNameToVMAName2)
{
    // has .js
    CString filename = "test.js";
    CString res = ModulePathHelper::ParseFileNameToVMAName(filename);
    EXPECT_EQ(res, "ArkTS Code");

    // other branches
    filename = "test.ts";
    res = ModulePathHelper::ParseFileNameToVMAName(filename);
    EXPECT_EQ(res, "ArkTS Code");
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
    ecmascript::ThreadNativeScope nativeScope(thread);
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    JSNApi::GetExportObject(instance, "module_test_module_test_B", "a");
    EXPECT_TRUE(result);
}

HWTEST_F_L0(EcmaModuleTest, HostResolveImportedModuleBundlePack)
{
    CString baseFileName = MODULE_ABC_PATH "module_test_module_test_A.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    JSHandle<JSTaggedValue> moduleRecord(thread, module.GetTaggedValue());
    moduleManager->AddResolveImportedModule(baseFileName, moduleRecord.GetTaggedValue());
    JSHandle<JSTaggedValue> res = ModuleResolver::HostResolveImportedModule(thread, baseFileName);

    EXPECT_EQ(moduleRecord->GetRawData(), res.GetTaggedValue().GetRawData());
}

HWTEST_F_L0(EcmaModuleTest, HostResolveImportedModuleBundlePack2)
{
    CString recordName = "module_test_module_test_A";
    CString baseFileName = MODULE_ABC_PATH "module_test_module_test_A.abc";
    CString record = JSPandaFile::ENTRY_MAIN_FUNCTION;
    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFileName, record);
    EXPECT_NE(jsPandaFile, nullptr);
    JSHandle<JSTaggedValue> res1 =
        ModuleResolver::HostResolveImportedModule(thread, baseFileName, record, jsPandaFile.get(), false);
    EXPECT_NE(res1.GetTaggedValue(), JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> res2 =
        ModuleResolver::HostResolveImportedModule(thread, baseFileName, record, jsPandaFile.get(), false);
    EXPECT_NE(res2.GetTaggedValue(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, IsSharedModuleLoaded)
{
    CString recordName = "module_test_module_test_A";
    ModuleManager *moduleManager = thread->GetModuleManager();
    bool res = moduleManager->IsSharedModuleLoaded(recordName);
    EXPECT_EQ(res, false);
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueInner)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    JSHandle<JSTaggedValue> moduleRecord = JSHandle<JSTaggedValue>::Cast(module);

    JSTaggedValue res1 = ModuleValueAccessor::GetModuleValueInner(thread, 0, moduleRecord);
    EXPECT_EQ(res1, JSTaggedValue::Hole());

    JSTaggedValue res2 = ModuleValueAccessor::GetModuleValueOuter(thread, 0, moduleRecord);
    EXPECT_TRUE(res2.IsInt());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueOuterInternal)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_B");
    JSHandle<JSTaggedValue> moduleRecord = JSHandle<JSTaggedValue>::Cast(module);

    // moduleEnvironment IsUndefined
    JSTaggedValue res = ModuleValueAccessor::GetModuleValueOuter(thread, 0, moduleRecord);
    EXPECT_TRUE(res.IsSpecial());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleNamespaceInternal)
{
    CString localName = "foo";

    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> localNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));

    // moduleEnvironment IsUndefined
    JSHandle<SourceTextModule> module = objFactory->NewSourceTextModule();
    JSTaggedValue res1 = MockDeprecatedModuleValueAccessor::GetModuleNamespaceInternal(thread,
        localNameHandle.GetTaggedValue(), module.GetTaggedValue());
    EXPECT_EQ(res1, thread->GlobalConstants()->GetUndefined());

    // FindEntry fail
    JSHandle<NameDictionary> map(NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(1)));
    module->SetEnvironment(thread, map);
    JSTaggedValue res2 = MockDeprecatedModuleValueAccessor::GetModuleNamespaceInternal(thread,
        localNameHandle.GetTaggedValue(), module.GetTaggedValue());
    EXPECT_EQ(res2, thread->GlobalConstants()->GetUndefined());
}

HWTEST_F_L0(EcmaModuleTest, GetExportObjectIndex2)
{
    CString key = "module_test_module_test_A";
    ModuleManager *moduleManager = thread->GetModuleManager();
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

    // ExecuteFromAbcFile is success
    JSHandle<JSTaggedValue> res2 = ecmascript::NapiModuleLoader::GetModuleNameSpace(thread, recordName2, baseFileName);
    EXPECT_NE(res2, thread->GlobalConstants()->GetHandledUndefinedString());

    // ExecuteFromAbcFile is fail
    JSHandle<JSTaggedValue> res1 = ecmascript::NapiModuleLoader::GetModuleNameSpace(thread, recordName1, baseFileName);
    EXPECT_EQ(res1.GetTaggedValue(), JSTaggedValue::Exception());
}

HWTEST_F_L0(EcmaModuleTest, TryGetImportedModule)
{
    // TryGetImportedModule can not found Module
    CString moduleName = "moduleName";
    ModuleManager *moduleManager = thread->GetModuleManager();
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
    JSHandle<TaggedArray> localExportEntries(thread, module->GetLocalExportEntries(thread));
    CString baseFileName = "a.abc";
    module->SetEcmaModuleFilenameString(baseFileName);
    ModuleManager *moduleManager = thread->GetModuleManager();
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

    // Test moduleRequestName start with "@package:"
    moduleRequestName = "@package:test";
    result = "test";
    entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename, moduleRecordName,
                                                           moduleRequestName);
    EXPECT_EQ(result, entryPoint);

    // Test cross application
    moduleRecordName = "@bundle:com.bundleName1.test/moduleName/requestModuleName1";
    CString newBaseFileName = "/data/storage/el1/bundle/com.bundleName.test/moduleName/moduleName/ets/modules.abc";
    ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename, moduleRecordName, moduleRequestName);
    EXPECT_EQ(baseFilename, newBaseFileName);
}

HWTEST_F_L0(EcmaModuleTest, ConcatFileNameWithMerge_Normalized)
{
    const CString baseFilename = "merge.abc";
    CString baseFilename1 = baseFilename;
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
    // Test moduleRequestName start with "@normalized"
    CString moduleRecordName1 = "moduleTest1";
    CString moduleRequestName1 = "@normalized:N&&&entryPath&version";
    CString result1 = "&entryPath&version";
    CString entryPoint1 = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename1, moduleRecordName1,
                                                                    moduleRequestName1);
    EXPECT_EQ(result1, entryPoint1);
    EXPECT_EQ(baseFilename1, baseFilename);

    // Test cross application
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
    CString moduleRequestName2 = "@normalized:N&moduleName&bundleNameBB&entryPath&version";
    CString result2 = "bundleNameBB&entryPath&version";
    CString newBaseFileName2 = "/data/storage/el1/bundle/bundleNameBB/moduleName/moduleName/ets/modules.abc";
    CString entryPoint2 = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFilename1, moduleRecordName1,
        moduleRequestName2);
    EXPECT_EQ(entryPoint2, result2);
    EXPECT_EQ(baseFilename1, newBaseFileName2);
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
    CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo*> &recordInfo =
        const_cast<CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo*>&>(pf->GetJSRecordInfo());
    
    CString moduleRecordName = "node_modules/0/moduleTest4/index";
    CString moduleRequestName = "json/index";
    CString result = "node_modules/0/moduleTest4/node_modules/json/index";
    JSPandaFile::JSRecordInfo *info = new JSPandaFile::JSRecordInfo();
    info->npmPackageName = "node_modules/0/moduleTest4";
    recordInfo.insert({std::string_view(moduleRecordName.c_str(), moduleRecordName.size()), info});
    recordInfo.insert({std::string_view(result.c_str(), result.size()), info});
    CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(
        thread, pf.get(), baseFilename, moduleRecordName, moduleRequestName);
    EXPECT_EQ(result, entryPoint);
    
    // delete info
    delete info;
    recordInfo.erase(std::string_view(moduleRecordName.c_str(), moduleRecordName.size()));
    recordInfo.erase(std::string_view(result.c_str(), result.size()));
}

HWTEST_F_L0(EcmaModuleTest, ConcatFileNameWithMerge5)
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
    CString result = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFileName, recordName,
                                                               requestName);
    EXPECT_EQ(result, "");

    pf->InsertJSRecordInfo(expectRes);
    result = ModulePathHelper::ConcatFileNameWithMerge(thread, pf.get(), baseFileName, recordName,
                                                       requestName);
    EXPECT_EQ(result, expectRes);
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

    inputFileName = "com.bundleName.test/moduleName/ets/pages/index";
    CString outRes3 = "/data/storage/el1/bundle/moduleName/ets/modules.abc";
    CString res6 = "com.bundleName.test/com.bundleName.test/moduleName/ets/pages/index";
    ModulePathHelper::ParseAbcPathAndOhmUrl(instance, inputFileName, outFileName, entryPoint);
    EXPECT_EQ(entryPoint, res6);
    EXPECT_EQ(outFileName, outRes3);
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

    EXPECT_EQ(SourceTextModule::IsNativeModule(requestName1), false);
    EXPECT_EQ(SourceTextModule::IsNativeModule(requestName6), false);
    EXPECT_EQ(SourceTextModule::IsNativeModule(requestName7), false);

    EXPECT_EQ(SourceTextModule::IsNativeModule(requestName2), true);
    EXPECT_EQ(SourceTextModule::GetNativeModuleType(requestName2), ModuleTypes::OHOS_MODULE);

    EXPECT_EQ(SourceTextModule::IsNativeModule(requestName3), true);
    EXPECT_EQ(SourceTextModule::GetNativeModuleType(requestName3), ModuleTypes::APP_MODULE);

    EXPECT_EQ(SourceTextModule::IsNativeModule(requestName4), true);
    EXPECT_EQ(SourceTextModule::GetNativeModuleType(requestName4), ModuleTypes::NATIVE_MODULE);

    EXPECT_EQ(SourceTextModule::IsNativeModule(requestName5), true);
    EXPECT_EQ(SourceTextModule::GetNativeModuleType(requestName5), ModuleTypes::INTERNAL_MODULE);
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

HWTEST_F_L0(EcmaModuleTest, TransformToNormalizedOhmUrl2)
{
    CMap<CString, CMap<CString, CVector<CString>>> pkgList;
    CMap<CString, CVector<CString>> entryList;
    entryList["hsp"] = {
        "packageName", "hsp",
        "bundleName", "com.hsp.application",
        "moduleName", "",
        "version", "",
        "entryPath", "",
        "isSO", "false"
    };
    pkgList["entry"] = entryList;
    instance->SetpkgContextInfoList(pkgList);

    CString inputFileName = "/data/storage/el1/bundle/hsp/ets/modules.abc";
    CString outBaseFileName = "/data/storage/el1/bundle/com.example.application/hsp/hsp/ets/modules.abc";
    CString entryPoint = "com.example.myapplication/hsp/ets/pages/Index";
    CString res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "com.example.myapplication/hsp/ets/pages/Index");

    outBaseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    entryPoint = "com.hsp.application/hsp/ets/pages/Index";
    res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "com.hsp.application&hsp/src/main/ets/pages/Index&");
}

HWTEST_F_L0(EcmaModuleTest, TransformToNormalizedOhmUrl3)
{
    // currentModuleName.size() is 0
    CString inputFileName = "/data/storage/el1/bundle";
    CString outBaseFileName = "/data/storage/el1/bundle";
    CString entryPoint = "com.example.myapplication/entry@har/ets/pages/Index";
    CString res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "com.example.myapplication/entry@har/ets/pages/Index");

    // pkgname.size() is 0
    CMap<CString, CString> list;
    list["har"] = "";
    instance->SetPkgNameList(list);
    inputFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    outBaseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    entryPoint = "com.example.myapplication/entry@har/ets/pages/Index";
    res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, entryPoint);

    // oldEntryPoint equal ENTRY_MAIN_FUNCTION
    entryPoint = "_GLOBAL::func_main_0";
    res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "_GLOBAL::func_main_0");

    // oldEntryPoint is not equal ENTRY_MAIN_FUNCTION but start with &
    entryPoint = "&test";
    res = ModulePathHelper::TransformToNormalizedOhmUrl(instance, inputFileName, outBaseFileName, entryPoint);
    EXPECT_EQ(res, "&test");
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
    CString recordInfo = "&entry/ets/Test&";
    pf->InsertJSRecordInfo(recordInfo);
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

HWTEST_F_L0(EcmaModuleTest, TranslateExpressionToNormalized3)
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

    CString requestPath = "./@normalized:N&&&har/Index&1.0.0";
    CString baseFileName = "";
    CString recordName = "";
    CString recordInfo = "@normalized:N&&&har/Index&1.0.0&";
    pf->InsertJSRecordInfo(recordInfo);
    CString result = ModulePathHelper::TranslateExpressionToNormalized(thread, pf.get(), baseFileName, recordName,
        requestPath);
    EXPECT_EQ(result, "./@normalized:N&&&har/Index&1.0.0");

    requestPath = "ets/Test";
    recordName = "&entry/ets/pages/Index&";
    result = ModulePathHelper::TranslateExpressionToNormalized(thread, pf.get(), baseFileName, recordName,
        requestPath);
    EXPECT_EQ(result, "&entry/ets/Test&");
}

HWTEST_F_L0(EcmaModuleTest, TranslateExpressionToNormalized4)
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
        "entryPath", "./Index.ets",
        "isSO", "true"
    };
    pkgList["entry"] = entryList;
    instance->SetpkgContextInfoList(pkgList);

    // ConcatNormalizedOhmurlWithData testcase
    CString requestPath = "har";
    CString baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString result = ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, "",
        requestPath);
    EXPECT_EQ(result, "@normalized:Y&&&har&1.2.0");
    
    baseFileName = "D:\\test\\entry\\.preview\\default\\intermediates\\assets\\default\\ets\\modules.abc";
    instance->SetModuleName("entry");
    result = ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, "",
        requestPath);
    EXPECT_EQ(result, "@normalized:Y&&&har&1.2.0");
}

HWTEST_F_L0(EcmaModuleTest, TranslateExpressionToNormalized5)
{
    CMap<CString, CMap<CString, CVector<CString>>> pkgList;
    CMap<CString, CVector<CString>> entryList;
    entryList["har"] = {
        "packageName", "har",
        "bundleName", "",
        "moduleName", "",
        "version", "1.2.0",
        "entryPath", "./Index.ets",
        "isSO", "false"
    };
    pkgList["entry"] = entryList;
    instance->SetpkgContextInfoList(pkgList);

    CMap<CString, CString> aliasMap;
    aliasMap["@ohos/library"] = "har";
    instance->SetPkgAliasList(aliasMap);

    CString requestPath = "@ohos/library/src/main/ets/Test";
    CString baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    CString result = ModulePathHelper::TranslateExpressionToNormalized(thread, nullptr, baseFileName, "",
        requestPath);
    EXPECT_EQ(result, "@normalized:N&&&har/src/main/ets/Test&1.2.0");
}

HWTEST_F_L0(EcmaModuleTest, TranslateNapiFileRequestPath)
{
    CString modulePath = "modulePath";
    CString requestName = "requestName";
    CString result = ModulePathHelper::TranslateNapiFileRequestPath(thread, modulePath, requestName);
    EXPECT_EQ(result, "modulePath/requestName");

    CMap<CString, CMap<CString, CVector<CString>>> list;
    CMap<CString, CVector<CString>> childList;
    list["hsp"] = childList;
    instance->SetpkgContextInfoList(list);

    result = ModulePathHelper::TranslateNapiFileRequestPath(thread, modulePath, requestName);
    EXPECT_EQ(result, "&/src/main/requestName&");
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

HWTEST_F_L0(EcmaModuleTest, SplitNormalizedRecordName2)
{
    CString requestPath = "har/Index";
    CVector<CString> res = ModulePathHelper::SplitNormalizedRecordName(requestPath);
    int exceptCount = 5;
    EXPECT_EQ(res.size(), exceptCount);
    CString emptyStr = "";
    EXPECT_EQ(res[0], emptyStr);
    EXPECT_EQ(res[1], emptyStr);
    EXPECT_EQ(res[2], emptyStr);
    EXPECT_EQ(res[3], emptyStr);
    CString importPath = "har/Index";
    EXPECT_EQ(res[4], importPath);
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

HWTEST_F_L0(EcmaModuleTest, GetCurrentModuleName)
{
    ecmascript::ThreadNativeScope nativeScope(thread);
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    JSNApi::Execute(instance, baseFileName, "module_test_module_test_module");
    Local<ObjectRef> res = JSNApi::GetExportObject(instance, "module_test_module_test_module", "moduleName");
    JSHandle<JSTaggedValue> result = JSNApiHelper::ToJSHandle(res);
    CString moduleName = ConvertToString(thread, result.GetTaggedValue());
    EXPECT_EQ(moduleName, "");
}

HWTEST_F_L0(EcmaModuleTest, ReviseLoadedModuleCount2) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    CString recordName2 = "b";
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
    module2->SetLoadingTypes(LoadingTypes::STABLE_MODULE);

    ModuleDeregister::ReviseLoadedModuleCount(thread, recordName2);
    EXPECT_EQ(module2->GetLoadingTypes(), LoadingTypes::STABLE_MODULE);
}

HWTEST_F_L0(EcmaModuleTest, IncreaseRegisterCounts2)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    std::set<CString> increaseModule;
    increaseModule.insert("module_test_module_test_B");
    increaseModule.insert("b");
    module->SetSharedType(SharedTypes::SHARED_MODULE);
    ModuleDeregister::IncreaseRegisterCounts(thread, module, increaseModule);
    EXPECT_EQ(module->GetModuleRequests(thread).IsUndefined(), false);
}

HWTEST_F_L0(EcmaModuleTest, DecreaseRegisterCounts2)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    std::set<CString> decreaseModule;
    decreaseModule.insert("module_test_module_test_B");
    decreaseModule.insert("b");
    module->SetSharedType(SharedTypes::SHARED_MODULE);
    ModuleDeregister::DecreaseRegisterCounts(thread, module, decreaseModule);
    EXPECT_EQ(module->GetModuleRequests(thread).IsUndefined(), false);
}

HWTEST_F_L0(EcmaModuleTest, GenerateSendableFuncModule)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetSharedType(SharedTypes::SHARED_MODULE);
    JSHandle<JSTaggedValue> moduleRecord = JSHandle<JSTaggedValue>::Cast(module);
    SendableClassModule::GenerateSendableFuncModule(thread, moduleRecord);
    JSHandle<SourceTextModule> currentModule = JSHandle<SourceTextModule>::Cast(moduleRecord);
    EXPECT_TRUE(SourceTextModule::IsModuleInSharedHeap(currentModule));
}

HWTEST_F_L0(EcmaModuleTest, ConcatMergeFileNameToNormalized)
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

    CString requestPath = "@normalized:N&&&har/Index&1.0.0";
    CString recordName = "";
    CString result = "&har/Index&1.0.0";
    CString entryPoint = ModulePathHelper::ConcatMergeFileNameToNormalized(thread, pf.get(), baseFilename, recordName,
        requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "&index";
    result = "&index";
    entryPoint = ModulePathHelper::ConcatMergeFileNameToNormalized(thread, pf.get(), baseFilename, recordName,
        requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "./@normalized:N&&&har/Index&1.0.0";
    result = "@normalized:N&&&har/Index&1.0.0&";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatMergeFileNameToNormalized(thread, pf.get(), baseFilename, recordName,
        requestPath);
    EXPECT_EQ(result, entryPoint);

    recordName = "pkg_modules/.ohpm/validator@13.12.0/pkg_modules/validator/index";
    requestPath = "./lib/toDate";
    result = "pkg_modules/.ohpm/validator@13.12.0/pkg_modules/validator/lib/toDate";
    pf->InsertJSRecordInfo(result);
    CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo*> &recordInfo =
        const_cast<CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo*>&>(pf->GetJSRecordInfo());
    JSPandaFile::JSRecordInfo *info = new JSPandaFile::JSRecordInfo();
    info->npmPackageName = result;
    recordInfo.insert({std::string_view(recordName.c_str(), recordName.size()), info});

    entryPoint = ModulePathHelper::ConcatMergeFileNameToNormalized(thread, pf.get(), baseFilename, recordName,
        requestPath);
    EXPECT_EQ(result, entryPoint);
    
    delete info;
    recordInfo.erase(std::string_view(recordName.c_str(), recordName.size()));
}

HWTEST_F_L0(EcmaModuleTest, ConcatImportFileNormalizedOhmurlWithRecordName)
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

    CString requestPath = "@normalized:N&&&har/Index&1.0.0";
    CString recordName = "";
    CString entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(),
        baseFilename, recordName, requestPath);
    EXPECT_EQ(CString(), entryPoint);

    requestPath = "@normalized:N&&&har/Index&1.0.0";
    CString result = "&har/Index&1:0.0";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(), baseFilename,
        recordName, requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "@normalized:N&&&har/Index&1.0.0";
    result = "@normalized:N&&&har/Index&1.0.0/index&";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(), baseFilename,
        recordName, requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "@normalized:N&&&har/Index&1.0.0";
    result = "@normalized:N&&&har/Index&1.0.0&";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(), baseFilename,
        recordName, requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "./@normalized:N&&&har/Index&1.0.0";
    entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(), baseFilename,
        recordName, requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "./Test2";
    recordName = "&entry/ets/pages/Index&";
    result = "&entry/ets/pages/Test2&";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(), baseFilename,
        recordName, requestPath);
    EXPECT_EQ(result, entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, ConcatImportFileNormalizedOhmurlWithRecordName2)
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

    CString requestPath = "./Test";
    CString recordName = "com.demo.application&hsp/Index&";
    CString result = "com.demo.application&hsp/Test&";
    pf->InsertJSRecordInfo(result);
    CString entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(),
        baseFilename, recordName, requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "../Test2";
    recordName = "com.demo.application&hsp/src/main/ets/pages/Index&";
    result = "com.demo.application&hsp/src/main/ets/Test2&";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(),
        baseFilename, recordName, requestPath);
    EXPECT_EQ(result, entryPoint);

    requestPath = "../Test3";
    recordName = "com.demo.application&hsp/src/main/ets/pages/Index&1.0.1";
    result = "com.demo.application&hsp/src/main/ets/Test3&1.0.1";
    pf->InsertJSRecordInfo(result);
    entryPoint = ModulePathHelper::ConcatImportFileNormalizedOhmurlWithRecordName(thread, pf.get(),
        baseFilename, recordName, requestPath);
    EXPECT_EQ(result, entryPoint);
}

HWTEST_F_L0(EcmaModuleTest, HostResolveImportedModuleWithMerge)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("test");
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    CString recordName2 = "@ohos:hilog";
    module2->SetEcmaModuleRecordNameString(recordName2);
    module2->SetTypes(ModuleTypes::NATIVE_MODULE);

    JSHandle<JSTaggedValue> nativeName = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("@ohos:hilog"));
    JSHandle<JSTaggedValue> res1 =
        ModuleResolver::HostResolveImportedModule(thread, module1, nativeName);
    EXPECT_TRUE(res1->IsSourceTextModule());

    thread->GetModuleManager()->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
    JSHandle<JSTaggedValue> res2 =
        ModuleResolver::HostResolveImportedModule(thread, module1, nativeName);
    EXPECT_TRUE(res2->IsSourceTextModule());
}

HWTEST_F_L0(EcmaModuleTest, ModuleResolverHostResolveImportedModule)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    CString recordName2 = "@ohos:hilog";
    module2->SetEcmaModuleRecordNameString(recordName2);
    module2->SetTypes(ModuleTypes::NATIVE_MODULE);

    JSHandle<JSTaggedValue> nativeName = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("@ohos:hilog"));
    thread->GetModuleManager()->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
    JSHandle<JSTaggedValue> res1 =
        ModuleResolver::HostResolveImportedModule(thread, module1, nativeName);
    EXPECT_TRUE(res1->IsSourceTextModule());
}

HWTEST_F_L0(EcmaModuleTest, ResolveExportObject)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();

    JSHandle<JSTaggedValue> exportName = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("notExist"));
    JSHandle<NativeModuleFailureInfo> nativeModuleFailureInfo = objectFactory->NewNativeModuleFailureInfo();
    JSHandle<JSTaggedValue> res1 = SourceTextModule::ResolveExportObject(
        thread, module1, JSHandle<JSTaggedValue>::Cast(nativeModuleFailureInfo), exportName);
    EXPECT_TRUE(res1->IsResolvedIndexBinding());

    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    JSHandle<JSObject> obj(objectFactory->NewJSObjectByConstructor(ctor));
    JSHandle<TaggedArray> exportArr = objectFactory->NewTaggedArray(2);
    obj->SetProperties(thread, exportArr);
    JSHandle<JSHClass> jsHclass(thread, obj->GetJSHClass());
    JSHandle<LayoutInfo> layout = objectFactory->CreateLayoutInfo(0, MemSpaceType::OLD_SPACE, GrowMode::KEEP);
    jsHclass->SetLayout(thread, layout);
    JSHandle<JSTaggedValue> res2 = SourceTextModule::ResolveExportObject(
        thread, module1, JSHandle<JSTaggedValue>::Cast(obj), exportName);
    EXPECT_TRUE(res2->IsHole());

    JSHandle<JSObject> obj1(objectFactory->NewJSObjectByConstructor(ctor));
    JSHandle<NameDictionary> dict = NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(2));
    obj1->SetProperties(thread, JSHandle<JSTaggedValue>::Cast(dict));
    JSHandle<JSTaggedValue> res3 = SourceTextModule::ResolveExportObject(
        thread, module1, JSHandle<JSTaggedValue>::Cast(obj1), exportName);
    EXPECT_TRUE(res3->IsHole());
}

HWTEST_F_L0(EcmaModuleTest, ResolveNativeStarExport)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString recordName = "@ohos:hilog";
    module->SetEcmaModuleRecordNameString(recordName);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);

    JSHandle<JSTaggedValue> res1 = SourceTextModule::ResolveNativeStarExport(thread, module, val);
    EXPECT_TRUE(res1->IsNull());

    module->SetStatus(ModuleStatus::EVALUATED);
    JSHandle<JSTaggedValue> res2 = SourceTextModule::ResolveNativeStarExport(thread, module, val);
    EXPECT_TRUE(res2->IsNull());
}

HWTEST_F_L0(EcmaModuleTest, MakeInternalArgs)
{
    auto vm = thread->GetEcmaVM();
    CString soName = "@ohos:hilig";
    JSHandle<JSTaggedValue> func(vm->GetFactory()->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(Callback::RegisterCallback)));
    SourceTextModule::MakeInternalArgs(vm, func, soName, soName);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModuleImpl)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    ModuleLogger *moduleLogger = new ModuleLogger(vm);
    thread->SetModuleLogger(moduleLogger);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiValue),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    // app module
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    SourceTextModule::LoadNativeModuleImpl(vm, thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());

    // ohos module
    module->SetEcmaModuleRecordNameString("@ohos:xxxxx");
    SourceTextModule::LoadNativeModuleImpl(vm, thread, module, ModuleTypes::OHOS_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());

    // internal module
    module->SetEcmaModuleRecordNameString("@hms:xxxxx");
    SourceTextModule::LoadNativeModuleImpl(vm, thread, module, ModuleTypes::INTERNAL_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());

    thread->SetModuleLogger(nullptr);
    delete moduleLogger;
    SourceTextModule::LoadNativeModuleImpl(vm, thread, module, ModuleTypes::INTERNAL_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModuleMayThrowError1)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiFailure),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    SourceTextModule::LoadNativeModuleMayThrowError(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModuleMayThrowError2)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSTaggedValue::SetProperty(thread, global, keyValue, thread->GlobalConstants()->GetHandledUndefined());
    SourceTextModule::LoadNativeModuleMayThrowError(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModuleMayThrowError3)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiException),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    SourceTextModule::LoadNativeModuleMayThrowError(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModuleMayThrowError4)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiValue),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    SourceTextModule::LoadNativeModuleMayThrowError(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModule1)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiValue),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    SourceTextModule::LoadNativeModule(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModule2)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiFailure),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    SourceTextModule::LoadNativeModule(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, EvaluateNativeModule1)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    SourceTextModule::EvaluateNativeModule(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, EvaluateNativeModule2)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    module->SetStatus(ModuleStatus::INSTANTIATED);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiFailure),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    SourceTextModule::EvaluateNativeModule(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, EvaluateNativeModule3)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    module->SetStatus(ModuleStatus::INSTANTIATED);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiValue),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    SourceTextModule::EvaluateNativeModule(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}


HWTEST_F_L0(EcmaModuleTest, ModuleInstantiation)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString("modules.abc");
    module->SetEcmaModuleRecordNameString("b");
    module->SetTypes(ModuleTypes::ECMA_MODULE);
    module->SetStatus(ModuleStatus::UNINSTANTIATED);
    module->SetIsNewBcVersion(false);
    CVector<JSHandle<SourceTextModule>> stack;
    SourceTextModule::PreModuleInstantiation(thread, module, ExecuteTypes::STATIC);
    JSHandle<JSTaggedValue> exception(thread, thread->GetException());
    int index = SourceTextModule::FinishModuleInstantiation(thread, module, stack, 1, exception);
    EXPECT_EQ(index, 2);
}

HWTEST_F_L0(EcmaModuleTest, ModuleDeclarationEnvironmentSetup)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    CString recordName = "a";
    CString recordName1 = "@ohos:hilog";
    module->SetEcmaModuleFilenameString(baseFileName);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    module->SetStatus(ModuleStatus::UNINSTANTIATED);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    JSHandle<TaggedArray> requestedModules = objectFactory->NewTaggedArray(2);
    requestedModules->Set(thread, 0, module1);
    requestedModules->Set(thread, 1, module1);
    module->SetRequestedModules(thread, requestedModules.GetTaggedValue());
    JSHandle<JSTaggedValue> importName = val;
    JSHandle<JSTaggedValue> localName = val;
    JSHandle<ImportEntry> importEntry1 =
        objectFactory->NewImportEntry(0, importName, localName, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module, importEntry1, 0, 2);
    JSHandle<JSTaggedValue> starString = thread->GlobalConstants()->GetHandledStarString();
    JSHandle<ImportEntry> importEntry2 =
        objectFactory->NewImportEntry(1, starString, localName, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module, importEntry2, 1, 2);

    module1->SetEcmaModuleFilenameString(baseFileName);
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module1->SetLocalExportEntries(thread, localExportEntries);
    module1->SetTypes(ModuleTypes::NATIVE_MODULE);
    module1->SetStatus(ModuleStatus::EVALUATED);
    
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName.c_str(), module.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordName1.c_str(), module1.GetTaggedValue());
    CVector<JSHandle<SourceTextModule>> stack;
    SourceTextModule::ModuleDeclarationEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    module->SetEcmaModuleRecordNameString("");
    SourceTextModule::ModuleDeclarationEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, ModuleDeclarationArrayEnvironmentSetup)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    module->SetSharedType(SharedTypes::SHARED_MODULE);
    SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    module->SetStatus(ModuleStatus::EVALUATED);
    SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    module->SetSharedType(SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, Evaluate)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString recordName = "a";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetStatus(ModuleStatus::EVALUATED);
    module->SetCycleRoot(thread, module);
    SourceTextModule::Evaluate(thread, module, nullptr, 0, ExecuteTypes::STATIC);
    EXPECT_TRUE(!thread->HasPendingException());
    SourceTextModule::Evaluate(thread, module, nullptr, 0, ExecuteTypes::DYNAMIC);
    EXPECT_TRUE(!thread->HasPendingException());
    ModuleLogger *moduleLogger = new ModuleLogger(vm);
    thread->SetModuleLogger(moduleLogger);
    SourceTextModule::Evaluate(thread, module, nullptr, 0, ExecuteTypes::STATIC);
    EXPECT_TRUE(!thread->HasPendingException());
    SourceTextModule::Evaluate(thread, module, nullptr, 0, ExecuteTypes::DYNAMIC);
    EXPECT_TRUE(!thread->HasPendingException());
    thread->SetModuleLogger(nullptr);
    delete moduleLogger;
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModule)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<JSTaggedValue> res = moduleManager->LoadNativeModule(
        thread, "@app:bundleName/moduleName/lib*.so");
    EXPECT_EQ(res.GetTaggedValue(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, ExecuteNativeModuleMayThrowError)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    CString recordName = "@app:bundleName/moduleName/lib*.so";
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<JSTaggedValue> res = moduleManager->ExecuteNativeModuleMayThrowError(
        thread, recordName);
    EXPECT_EQ(res.GetTaggedValue(), JSTaggedValue::Undefined());

    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    JSHandle<JSTaggedValue> res2 = moduleManager->ExecuteNativeModuleMayThrowError(
        thread, recordName);
    EXPECT_NE(res2.GetTaggedValue(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, ExecuteNativeModule)
{
    auto vm = thread->GetEcmaVM();
    CString recordName = "@app:bundleName/moduleName/lib*.so";
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    
    ModuleManager *moduleManager = thread->GetModuleManager();
    module->SetStatus(ecmascript::ModuleStatus::INSTANTIATED);
    moduleManager->AddResolveImportedModule(
        recordName, module.GetTaggedValue());
    moduleManager->ExecuteNativeModule(thread, recordName);
    module->SetStatus(ModuleStatus::EVALUATED);
    JSHandle<JSTaggedValue> res = moduleManager->ExecuteNativeModule(thread, recordName);
    EXPECT_NE(res.GetTaggedValue(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, ExecuteNativeModule2)
{
    auto vm = thread->GetEcmaVM();
    CString recordName = "@app:bundleName/moduleName/lib*.so";
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<JSTaggedValue> res = moduleManager->ExecuteNativeModule(thread, recordName);
    EXPECT_NE(res.GetTaggedValue(), JSTaggedValue::Undefined());
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
    JSHandle<JSTaggedValue> importName = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("ccc"));
    JSHandle<JSTaggedValue> localName = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("ccc"));
    JSHandle<SourceTextModule> module3 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName3 = "c";
    module2->SetEcmaModuleRecordNameString(recordName3);
    JSHandle<TaggedArray> requestedModules = objectFactory->NewTaggedArray(1);
    requestedModules->Set(thread, 0, module3);
    module1->SetRequestedModules(thread, requestedModules.GetTaggedValue());
    JSHandle<ImportEntry> importEntry = objectFactory->NewImportEntry(0, importName,
                                                                      localName, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module2, importEntry, 0, 1);

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
    auto globalConstants = thread->GetEcmaVM()->GetJSThread()->GlobalConstants();
    auto funcName = (module3->GetTypes() == ModuleTypes::NATIVE_MODULE) ?
        globalConstants->GetHandledRequireNativeModuleString() :
        globalConstants->GetHandledRequireNapiString();
    JSTaggedValue nativeFunc = funcName.GetTaggedValue();
    bool isFunc = nativeFunc.IsJSFunction();
    EXPECT_EQ(isFunc, false);
}

HWTEST_F_L0(EcmaModuleTest, GetRequireNativeModuleFunc) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    uint16_t registerNum = module->GetRegisterCounts();
    module->SetStatus(ecmascript::ModuleStatus::INSTANTIATED);
    module->SetRegisterCounts(registerNum);
    auto globalConstants = thread->GetEcmaVM()->GetJSThread()->GlobalConstants();
    auto funcName = (module->GetTypes() == ModuleTypes::NATIVE_MODULE) ?
        globalConstants->GetHandledRequireNativeModuleString() :
        globalConstants->GetHandledRequireNapiString();
    JSTaggedValue nativeFunc = funcName.GetTaggedValue();
    bool isFunc = nativeFunc.IsJSFunction();
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
    SourceTextModule::StoreModuleValue(thread, module, storeKey, valueHandle);
    SourceTextModule::StoreModuleValue(thread, module, index, valueHandle1);
    JSHandle<JSTaggedValue> loadKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSTaggedValue loadValue = module->GetModuleValue(thread, loadKey.GetTaggedValue(), false);
    JSTaggedValue loadValue1 = module->GetModuleValue(thread, index, false);
    EXPECT_EQ(valueHandle.GetTaggedValue(), loadValue);
    EXPECT_EQ(valueHandle1.GetTaggedValue(), loadValue1);
}

HWTEST_F_L0(EcmaModuleTest, MakeAppArgs1) {
    auto vm = thread->GetEcmaVM();
    CString soPath = "@normalized:Y&&&libentry.so&";
    CString moduleName = "entry";
    CString requestName = "@normalized:";
    JSHandle<JSTaggedValue> func(vm->GetFactory()->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(Callback::RegisterCallback)));
    EcmaRuntimeCallInfo* info = SourceTextModule::MakeAppArgs(vm, func, soPath, moduleName, requestName);
    std::string res1 = EcmaStringAccessor(JSTaggedValue::ToString(thread, info->GetCallArg(0))).ToStdString(thread);
    std::string res2 = EcmaStringAccessor(JSTaggedValue::ToString(thread, info->GetCallArg(1))).ToStdString(thread);
    std::string res3 = EcmaStringAccessor(JSTaggedValue::ToString(thread, info->GetCallArg(2))).ToStdString(thread);
    EXPECT_TRUE(res1 == "entry");
    EXPECT_TRUE(res2 == "true");
    EXPECT_TRUE(res3 == "/entry");
}

HWTEST_F_L0(EcmaModuleTest, MakeAppArgs2) {
    auto vm = thread->GetEcmaVM();
    CString soPath = "@app:com.example.myapplication/entry";
    CString moduleName = "entry";
    CString requestName = "@app:";
    JSHandle<JSTaggedValue> func(vm->GetFactory()->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(Callback::RegisterCallback)));
    EcmaRuntimeCallInfo* info = SourceTextModule::MakeAppArgs(vm, func, soPath, moduleName, requestName);
    std::string res1 = EcmaStringAccessor(JSTaggedValue::ToString(thread, info->GetCallArg(0))).ToStdString(thread);
    std::string res2 = EcmaStringAccessor(JSTaggedValue::ToString(thread, info->GetCallArg(1))).ToStdString(thread);
    std::string res3 = EcmaStringAccessor(JSTaggedValue::ToString(thread, info->GetCallArg(2))).ToStdString(thread);
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

    requestName = "@normalized:N&&har/src/main/page/Test&1.0.0";
    res = ModulePathHelper::ParseNormalizedOhmUrl(thread, baseFileName, recordName, requestName);
    EXPECT_TRUE((thread)->HasPendingException());
    EXPECT_EQ(res, requestName);

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
    
    baseFileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    requestName = "@normalized:N&&com.example.application&hsp/src/main/page/Test&1.0.0";
    expectRes = "com.example.application&hsp/src/main/page/Test&1.0.0";
    CString res5 = ModulePathHelper::ParseNormalizedOhmUrl(thread, baseFileName, recordName, requestName);
    EXPECT_EQ(res5, expectRes);
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

HWTEST_F_L0(EcmaModuleTest, ModuleInstantiation_ReEnterTest)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSSourceTextModule();
    module->SetEcmaModuleFilenameString("modules.abc");
    module->SetEcmaModuleRecordNameString("b");
    module->SetTypes(ModuleTypes::ECMA_MODULE);
    module->SetStatus(ModuleStatus::EVALUATING);
    module->SetIsNewBcVersion(true);
    module->SetSharedType(SharedTypes::SHARED_MODULE);
    CVector<JSHandle<SourceTextModule>> stack;
    SourceTextModule::PreModuleInstantiation(thread, module, ExecuteTypes::STATIC);
    JSHandle<JSTaggedValue> exception(thread, thread->GetException());
    int index = SourceTextModule::FinishModuleInstantiation(thread, module, stack, 1, exception);
    EXPECT_EQ(index, 1);
    module->SetStatus(ModuleStatus::EVALUATING);
    module->SetSharedType(SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::PreModuleInstantiation(thread, module, ExecuteTypes::STATIC);
    index = SourceTextModule::FinishModuleInstantiation(thread, module, stack, 1, exception);
    EXPECT_EQ(index, 2);
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
    ModuleManager *moduleManager = thread->GetModuleManager();
    bool res = moduleManager->IsLocalModuleLoaded(moduleName);
    EXPECT_EQ(res, false);
}

HWTEST_F_L0(EcmaModuleTest, IncreaseRegisterCounts)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    std::set<CString> increaseModule;

    ModuleDeregister::IncreaseRegisterCounts(thread, module, increaseModule);
    EXPECT_EQ(module->GetModuleRequests(thread).IsUndefined(), false);

    module->SetRegisterCounts(INT8_MAX);
    ModuleDeregister::IncreaseRegisterCounts(thread, module, increaseModule);
    EXPECT_EQ(module->GetModuleRequests(thread).IsUndefined(), false);

    module2->SetRegisterCounts(INT8_MAX);
    ModuleDeregister::IncreaseRegisterCounts(thread, module2, increaseModule);
    EXPECT_EQ(module2->GetModuleRequests(thread).IsUndefined(), true);

    module2->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
    ModuleDeregister::IncreaseRegisterCounts(thread, module2, increaseModule);
    bool res = module2->GetLoadingTypes() == LoadingTypes::STABLE_MODULE;
    EXPECT_EQ(res, true);
}

HWTEST_F_L0(EcmaModuleTest, DecreaseRegisterCounts)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    std::set<CString> decreaseModule;

    ModuleDeregister::DecreaseRegisterCounts(thread, module, decreaseModule);
    bool res1 = module->GetLoadingTypes() != LoadingTypes::DYNAMITC_MODULE;
    EXPECT_EQ(res1, true);

    module->SetRegisterCounts(INT8_MAX);
    ModuleDeregister::DecreaseRegisterCounts(thread, module, decreaseModule);
    EXPECT_EQ(module->GetModuleRequests(thread).IsUndefined(), false);

    module2->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    ModuleDeregister::DecreaseRegisterCounts(thread, module2, decreaseModule);
    bool res2 = module2->GetLoadingTypes() != LoadingTypes::DYNAMITC_MODULE;
    EXPECT_EQ(res2, false);
}

HWTEST_F_L0(EcmaModuleTest, GetSendableModuleValueImpl) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    int32_t index = 2;
    JSTaggedValue currentModule1 = module1.GetTaggedValue();
    JSTaggedValue res1 = MockModuleValueAccessor::GetSendableModuleValueOuterInternal<false>(
        thread, index, currentModule1);
    EXPECT_EQ(res1, thread->GlobalConstants()->GetUndefined());

    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module2 = moduleManager->HostGetImportedModule("module_test_module_test_C");
    JSTaggedValue currentModule2 = module2.GetTaggedValue();
    JSTaggedValue res2 = MockModuleValueAccessor::GetSendableModuleValueOuterInternal<false>(
        thread, index, currentModule2);
    EXPECT_NE(res2, thread->GlobalConstants()->GetUndefined());
}

HWTEST_F_L0(EcmaModuleTest, GetLazySendableModuleValueImpl) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    int32_t index = 2;
    JSTaggedValue currentModule1 = module1.GetTaggedValue();
    JSTaggedValue res1 = MockModuleValueAccessor::GetSendableModuleValueOuterInternal<true>(
        thread, index, currentModule1);
    EXPECT_EQ(res1, thread->GlobalConstants()->GetUndefined());
}

HWTEST_F_L0(EcmaModuleTest, ResolveImportedModuleWithMerge) {
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    module->SetSharedType(SharedTypes::SHARED_MODULE);

    CString recordName2 = "testModule";
    ExecuteTypes executeType = ExecuteTypes::STATIC;
    JSHandle<JSTaggedValue> res = ModuleResolver::HostResolveImportedModule(
        thread, baseFileName.c_str(), recordName2, nullptr, executeType);
    EXPECT_EQ(res.GetTaggedValue(), JSTaggedValue::Exception());
}

HWTEST_F_L0(EcmaModuleTest, ResolveImportedModuleWithMerge2) {
    CString moduleName1;
    CString recordName1;

    ExecuteTypes executeType = ExecuteTypes::STATIC;
    JSHandle<JSTaggedValue> res = ModuleResolver::HostResolveImportedModule(
        thread, moduleName1, recordName1, nullptr, executeType);
    EXPECT_EQ(res.GetTaggedValue(), JSTaggedValue::Exception());
}

HWTEST_F_L0(EcmaModuleTest, IsInstantiatedSModule) {
    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    bool res = manager1->IsInstantiatedSModule(thread, module);
    EXPECT_EQ(res, true);
}

HWTEST_F_L0(EcmaModuleTest, ToStringWithPrecision)
{
    std::string res = ModuleLogger::ToStringWithPrecision(4.550, 3);
    EXPECT_EQ(res, "4.55");
}

HWTEST_F_L0(EcmaModuleTest, GetResolvedRecordIndexBindingModule)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();

    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    std::string baseFileNameStr = MODULE_ABC_PATH "module_unexecute.abc";
    CString baseFileName = baseFileNameStr.c_str();
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "module_unexecute";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ImportEntry> importEntry =
        objectFactory->NewImportEntry(0, val, val, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module1, importEntry, 0, 1);

    // test GetResolvedRecordIndexBindingModule
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName1);
    JSHandle<EcmaString> baseFileNameHdl = objectFactory->NewFromUtf8(baseFileName);
    JSHandle<ResolvedRecordIndexBinding> recordIndexBinding =
        objectFactory->NewSResolvedRecordIndexBindingRecord(recordNameHdl, baseFileNameHdl, 0);

    JSHandle<SourceTextModule> resolvedModule = MockModuleValueAccessor::
        GetResolvedModuleFromRecordIndexBinding<false>(thread, module1, recordIndexBinding);
    EXPECT_TRUE(resolvedModule->GetStatus() == ModuleStatus::EVALUATED);
}

HWTEST_F_L0(EcmaModuleTest, GetResolvedRecordBindingModule)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    std::string baseFileNameStr = MODULE_ABC_PATH "module_unexecute_A.abc";
    CString baseFileName = baseFileNameStr.c_str();
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "module_unexecute_A";
    module1->SetEcmaModuleRecordNameString(recordName1);

    // test GetResolvedRecordBindingModule
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);

    JSHandle<SourceTextModule> resolvedModule = MockModuleValueAccessor::
        GetResolvedModuleFromRecordBinding<false>(thread, module1, nameBinding);
    EXPECT_TRUE(resolvedModule->GetStatus() == ModuleStatus::EVALUATED);
}

HWTEST_F_L0(EcmaModuleTest, GetLazyModuleValueFromIndexBindingTest)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();

    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    std::string baseFileNameStr = MODULE_ABC_PATH "module_unexecute.abc";
    CString baseFileName = baseFileNameStr.c_str();
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "module_unexecute";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("stringOut"));
    JSHandle<ImportEntry> importEntry =
        objectFactory->NewImportEntry(0, val, val, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module1, importEntry, 0, 1);

    // test GetLazyModuleValueFromIndexBinding
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName1);
    JSHandle<EcmaString> baseFileNameHdl = objectFactory->NewFromUtf8(baseFileName);
    JSHandle<ResolvedRecordIndexBinding> recordIndexBinding =
        objectFactory->NewSResolvedRecordIndexBindingRecord(recordNameHdl, baseFileNameHdl, 0);

    JSTaggedValue value = MockModuleValueAccessor::GetModuleValueFromRecordIndexBinding<true>(
        thread, module1, recordIndexBinding.GetTaggedValue(), false);

    EXPECT_TRUE(value.IsString());
}

HWTEST_F_L0(EcmaModuleTest, GetLazyModuleValueFromRecordBindingTest)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSSourceTextModule();
    std::string baseFileNameStr1 = MODULE_ABC_PATH "module_unexecute_C.abc";
    CString baseFileName1 = baseFileNameStr1.c_str();
    module1->SetEcmaModuleFilenameString(baseFileName1);
    CString recordName1 = "module_unexecute_C";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("default"));

    // test GetLazyModuleValueFromRecordBinding
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName1);
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);

    JSTaggedValue resolvedModuleVal1 = MockModuleValueAccessor::
        GetModuleValueFromRecordBinding<true>(thread, module1, nameBinding.GetTaggedValue(), 0, false);
    EXPECT_TRUE(resolvedModuleVal1.IsString());
}

HWTEST_F_L0(EcmaModuleTest, ExecuteCjsModule)
{
    std::string baseFileName = MODULE_ABC_PATH "module_unexecute_C.abc";
    const CString baseFileNameStr = baseFileName.c_str();
    CString recordName = "module_unexecute_C";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    JSNApi::Execute(instance, baseFileName, "module_unexecute_C");

    std::shared_ptr<JSPandaFile> jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFileNameStr, recordName, false);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->ExecuteCjsModule(thread, recordName, jsPandaFile.get());
    EXPECT_TRUE(moduleManager->IsEvaluatedModule(recordName));
}

HWTEST_F_L0(EcmaModuleTest, GetModuleNamespace)
{
    CString localName1 = "foo";
    CString exportName1 = "bar";
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> localNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(localName1));
    JSHandle<JSTaggedValue> exportNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(exportName1));
    JSHandle<LocalExportEntry> localExportEntry1 =
        objectFactory->NewLocalExportEntry(exportNameHandle1, localNameHandle1, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                        SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry1, 0, 1);
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(localName1));
    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    JSTaggedValue res = DeprecatedModuleValueAccessor::GetModuleNamespace(thread, key.GetTaggedValue(), jsFunc);
    EXPECT_EQ(res, JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueInner3)
{
    CString localName1 = "foo";
    CString exportName1 = "bar";
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> localNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(localName1));
    JSHandle<JSTaggedValue> exportNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(exportName1));
    JSHandle<LocalExportEntry> localExportEntry1 =
        objectFactory->NewLocalExportEntry(exportNameHandle1, localNameHandle1, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                        SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry1, 0, 1);
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(exportName1));
    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    DeprecatedModuleValueAccessor::GetModuleValueInner(thread, key.GetTaggedValue(), jsFunc);
    JSTaggedValue res = DeprecatedModuleValueAccessor::GetModuleValueOuter(thread, key.GetTaggedValue(), jsFunc);
    EXPECT_EQ(res, JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueOuter)
{
    CString localName1 = "foo";
    CString exportName1 = "bar";
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> localNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(localName1));
    JSHandle<JSTaggedValue> exportNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(exportName1));
    JSHandle<LocalExportEntry> localExportEntry1 =
        objectFactory->NewLocalExportEntry(exportNameHandle1, localNameHandle1, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                        SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry1, 0, 1);
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(exportName1));


    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());

    JSHandle<NameDictionary> map(NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(1)));
    module->SetEnvironment(thread, map);

    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    DeprecatedModuleValueAccessor::GetModuleValueInner(thread, key.GetTaggedValue(), jsFunc);
    JSTaggedValue res = DeprecatedModuleValueAccessor::GetModuleValueOuter(thread, key.GetTaggedValue(), jsFunc);
    EXPECT_EQ(res, JSTaggedValue::Undefined());
}

HWTEST_F_L0(EcmaModuleTest, StoreModuleValue3)
{
    CString localName1 = "foo";
    CString exportName1 = "bar";
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> localNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(localName1));
    JSHandle<JSTaggedValue> exportNameHandle1 = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(exportName1));
    JSHandle<LocalExportEntry> localExportEntry1 =
        objectFactory->NewLocalExportEntry(exportNameHandle1, localNameHandle1, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                                           SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry1, 0, 1);
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(exportName1));
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));

    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());

    JSHandle<NameDictionary> map(NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(1)));
    module->SetEnvironment(thread, map);

    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    DeprecatedModuleValueAccessor::StoreModuleValue(thread, key.GetTaggedValue(), val.GetTaggedValue(), jsFunc);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetLazyModuleValueFromRecordBinding)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName2 = "b";
    module2->SetEcmaModuleRecordNameString(recordName2);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module2->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module2, 0, val);
    module2->SetStatus(ModuleStatus::EVALUATED);
    module2->SetTypes(ModuleTypes::NATIVE_MODULE);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName2, module2.GetTaggedValue());

    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName2);
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>::Cast(nameBinding);
    MockModuleValueAccessor::GetModuleValueFromRecordBinding<true>(thread, module2, key.GetTaggedValue(), 0, false);
    MockModuleValueAccessor::GetModuleValueFromRecordBinding<false>(thread, module2, key.GetTaggedValue(), 0, false);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, ReplaceModuleThroughFeature1)
{
    auto vm = thread->GetEcmaVM();
    CString moduleName = "a";
    CString mockName = "a_mock";
    std::map<std::string, std::string> list;
    list.emplace(moduleName, mockName);
    vm->SetMockModuleList(list);
    ModuleResolver::ReplaceModuleThroughFeature(thread, moduleName);
    EXPECT_EQ(moduleName, mockName);
}

HWTEST_F_L0(EcmaModuleTest, ReplaceModuleThroughFeature2)
{
    auto vm = thread->GetEcmaVM();
    CString moduleName = "a";
    std::vector<panda::HmsMap> map;
    HmsMap tmp;
    tmp.targetPath = "a_target";
    tmp.originalPath = "a";
    map.push_back(tmp);
    vm->SetHmsModuleList(map);
    ModuleResolver::ReplaceModuleThroughFeature(thread, moduleName);
    EXPECT_EQ(moduleName, tmp.targetPath.c_str());
}

HWTEST_F_L0(EcmaModuleTest, CheckResolvedBinding)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString recordName = "a";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());

    SourceTextModule::CheckResolvedBinding(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    CString recordName2 = "@ohos:hilog";
    module2->SetEcmaModuleRecordNameString(recordName2);
    module2->SetTypes(ModuleTypes::NATIVE_MODULE);
    module2->SetStatus(ModuleStatus::EVALUATED);
    moduleManager->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<IndirectExportEntry> indirectExportEntry = objectFactory->NewIndirectExportEntry(val,
        0, val, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> indirectExportEntries = objectFactory->NewTaggedArray(1);
    indirectExportEntries->Set(thread, 0, indirectExportEntry);
    module->SetIndirectExportEntries(thread, indirectExportEntries);

    JSHandle<TaggedArray> requestedModules = objectFactory->NewTaggedArray(1);
    requestedModules->Set(thread, 0, module2);
    module->SetRequestedModules(thread, requestedModules.GetTaggedValue());

    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module2->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::CheckResolvedBinding(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    module->SetEcmaModuleRecordNameString("");
    module->SetEcmaModuleFilenameString(recordName);
    JSHandle<TaggedArray> moduleRequests = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> name = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(recordName2.c_str()));
    moduleRequests->Set(thread, 0, name.GetTaggedValue());
    module->SetModuleRequests(thread, moduleRequests.GetTaggedValue());
    module2->SetLocalExportEntries(thread, thread->GlobalConstants()->GetUndefined());
    SourceTextModule::CheckResolvedBinding(thread, module);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, CheckResolvedIndexBinding)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString recordName = "a";
    module->SetEcmaModuleFilenameString(recordName);
    module->SetStatus(ModuleStatus::EVALUATED);
    CString recordName2 = "@ohos:hilog";
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString(recordName2);
    module2->SetTypes(ModuleTypes::NATIVE_MODULE);
    module2->SetStatus(ModuleStatus::EVALUATED);

    JSHandle<TaggedArray> moduleRequests = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> name = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(recordName2.c_str()));
    moduleRequests->Set(thread, 0, name.GetTaggedValue());
    module->SetModuleRequests(thread, moduleRequests.GetTaggedValue());
    JSHandle<TaggedArray> requestedModules = objectFactory->NewTaggedArray(1);
    requestedModules->Set(thread, 0, module2);
    module->SetRequestedModules(thread, requestedModules.GetTaggedValue());

    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<IndirectExportEntry> indirectExportEntry = objectFactory->NewIndirectExportEntry(val,
        0, val, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> indirectExportEntries = objectFactory->NewTaggedArray(1);
    indirectExportEntries->Set(thread, 0, indirectExportEntry);
    module->SetIndirectExportEntries(thread, indirectExportEntries);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
    SourceTextModule::CheckResolvedIndexBinding(thread, module);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, SetExportName)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString recordName = "@ohos:hilog";
    module->SetEcmaModuleFilenameString(recordName);
    CVector<std::string> exportNames;
    JSHandle<TaggedArray> exportStarSet = objectFactory->NewTaggedArray(2);
    SourceTextModule::SetExportName(thread, module, exportNames, exportStarSet);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, IsCircular)
{
    CList<CString> referenceList;
    CString moduleName = "a";
    bool res1 = SourceTextModule::IsCircular(referenceList, moduleName);
    EXPECT_EQ(res1, false);

    referenceList.push_back(moduleName);
    referenceList.push_back("b");
    bool res2 = SourceTextModule::IsCircular(referenceList, moduleName);
    EXPECT_EQ(res2, true);

    bool res3 = SourceTextModule::IsCircular(referenceList, "c");
    EXPECT_EQ(res3, false);
}

HWTEST_F_L0(EcmaModuleTest, SearchCircularImport)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "a";
    CString recordName2 = "@ohos:hilog";
    module->SetEcmaModuleRecordNameString(recordName);
    JSHandle<TaggedArray> requestArr = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8(recordName2.c_str()));
    requestArr->Set(thread, 0, val.GetTaggedValue());
    module->SetModuleRequests(thread, requestArr);

    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString(recordName2);
    module2->SetTypes(ModuleTypes::NATIVE_MODULE);
    module2->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName2, module2.GetTaggedValue());

    CList<CString> referenceList;
    referenceList.push_back(recordName);
    referenceList.push_back("b");
    referenceList.push_back(recordName2);
    SourceTextModule::SearchCircularImport(thread, recordName, module, referenceList, recordName2, true);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, IsDynamicModule)
{
    bool res = SourceTextModule::IsDynamicModule(LoadingTypes::DYNAMITC_MODULE);
    EXPECT_EQ(res, true);
}

HWTEST_F_L0(EcmaModuleTest, StoreModuleValue4)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("a");
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetLocalExportEntries(thread, localExportEntries);
    module->SetSharedType(SharedTypes::SHARED_MODULE);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValue1)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->GetModuleValue(thread, 0, true);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, ModuleExecution)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<TaggedArray> arr = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ImportEntry> importEntry = objectFactory->NewImportEntry(0, val, val, SharedTypes::UNSENDABLE_MODULE);
    module->SetImportEntries(thread, arr);
    module->AddImportEntry(thread, module, importEntry, 2, 3);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, AddAsyncParentModule)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    SourceTextModule::AddAsyncParentModule(thread, module1, module2);
    EXPECT_TRUE(!thread->HasPendingException());
    SourceTextModule::AddAsyncParentModule(thread, module1, module2);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, AsyncModuleExecutionFulfilled)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetStatus(ModuleStatus::ERRORED);
    module->SetException(thread, JSTaggedValue(2));
    SourceTextModule::AsyncModuleExecutionFulfilled(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    SourceTextModule::AsyncModuleExecutionRejected(thread, module, JSTaggedValue::Exception());
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GatherAvailableAncestors)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module2->SetPendingAsyncDependencies(1);
    module2->SetCycleRoot(thread, module2);
    SourceTextModule::AddAsyncParentModule(thread, module1, module2);
    SourceTextModule::AsyncParentCompletionSet execList1;
    module2->SetStatus(ModuleStatus::ERRORED);
    SourceTextModule::GatherAvailableAncestors(thread, module1, execList1);
    EXPECT_TRUE(!thread->HasPendingException());

    execList1.insert(module2);
    SourceTextModule::GatherAvailableAncestors(thread, module1, execList1);
    EXPECT_TRUE(!thread->HasPendingException());

    module2->SetStatus(ModuleStatus::EVALUATING_ASYNC);
    SourceTextModule::GatherAvailableAncestors(thread, module1, execList1);
    EXPECT_TRUE(!thread->HasPendingException());

    SourceTextModule::AsyncParentCompletionSet execList2;
    module2->SetStatus(ModuleStatus::EVALUATING_ASYNC);
    module2->SetAsyncEvaluatingOrdinal(3);
    SourceTextModule::GatherAvailableAncestors(thread, module1, execList2);
    EXPECT_TRUE(!thread->HasPendingException());

    SourceTextModule::AsyncParentCompletionSet execList3;
    module2->SetPendingAsyncDependencies(2);
    SourceTextModule::GatherAvailableAncestors(thread, module1, execList3);
    EXPECT_TRUE(!thread->HasPendingException());

    module2->SetHasTLA(true);
    SourceTextModule::GatherAvailableAncestors(thread, module1, execList3);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, ExecuteCjsModuleTest)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "modules.abc";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());

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
    std::shared_ptr<JSPandaFile> jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), baseFileName);

    CString recordName2 = "testModule";
    moduleManager->ExecuteCjsModule(thread, recordName2, jsPandaFile.get());
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, ExecuteCjsModuleTest2)
{
    const char *fileName = "__JSPandaFileExecutorTest1.abc";
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
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral *method = new MethodLiteral(methodId[0]);
    method->Initialize(pf.get());
    pf->SetMethodLiteralToMap(method);
    pfManager->AddJSPandaFile(pf);
    CString recordName = "testModule";
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->ExecuteCjsModule(thread, recordName, pf.get());
    EXPECT_TRUE(!thread->HasPendingException());
}


HWTEST_F_L0(EcmaModuleTest, GetLazyModuleValueOuter)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution =
        JSHandle<JSTaggedValue>::Cast(objectFactory->NewResolvedBindingRecord(module, val));
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetLazyModuleValueOuter(thread, 0, jsFunc);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetLazyModuleValueOuter2)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::CJS_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());

    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetLazyModuleValueOuter(thread, 0, jsFunc);
    EXPECT_TRUE(!thread->HasPendingException());

    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName);
    JSHandle<EcmaString> baseFileNameHdl = objectFactory->NewFromUtf8(baseFileName);
    JSHandle<ResolvedRecordIndexBinding> recordIndexBinding =
        objectFactory->NewSResolvedRecordIndexBindingRecord(recordNameHdl, baseFileNameHdl, 0);
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(recordIndexBinding);
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    ctor->SetModule(thread, module);
    jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetLazyModuleValueOuter(thread, 0, jsFunc);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetLazyModuleValueOuter3)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::CJS_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName);
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);

    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(nameBinding);
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetLazyModuleValueOuter(thread, 0, jsFunc);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueOuter1)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution =
        JSHandle<JSTaggedValue>::Cast(objectFactory->NewResolvedBindingRecord(module, val));
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetModuleValueOuter(thread, 0, jsFunc);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueOuter2)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::CJS_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    EXPECT_TRUE(!thread->HasPendingException());

    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetModuleValueOuter(thread, 0, jsFunc);

    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName);
    JSHandle<EcmaString> baseFileNameHdl = objectFactory->NewFromUtf8(baseFileName);
    JSHandle<ResolvedRecordIndexBinding> recordIndexBinding =
        objectFactory->NewSResolvedRecordIndexBindingRecord(recordNameHdl, baseFileNameHdl, 0);
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(recordIndexBinding);
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    ctor->SetModule(thread, module);
    jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetModuleValueOuter(thread, 0, jsFunc);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueOuter3)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::CJS_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName);
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);

    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(nameBinding);
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    JSHandle<JSFunction> ctor(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    ctor->SetModule(thread, module);
    JSTaggedValue jsFunc = ctor.GetTaggedValue();
    ModuleValueAccessor::GetModuleValueOuter(thread, 0, jsFunc);
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetSendableModuleValueImpl2) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::CJS_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName);
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(nameBinding);
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    MockModuleValueAccessor::GetSendableModuleValueOuterInternal<false>(thread, 0, module.GetTaggedValue());
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetLazySendableModuleValueImpl2) {
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleFilenameString(baseFileName);
    CString recordName = "cjs";
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::CJS_MODULE);
    module->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    JSHandle<EcmaString> recordNameHdl = objectFactory->NewFromUtf8(recordName);
    JSHandle<ResolvedRecordBinding> nameBinding =
        objectFactory->NewSResolvedRecordBindingRecord(recordNameHdl, val);
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(nameBinding);
    envRec->Set(thread, 0, resolution);
    module->SetEnvironment(thread, envRec);
    MockModuleValueAccessor::GetSendableModuleValueOuterInternal<true>(thread, 0, module.GetTaggedValue());
    EXPECT_TRUE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GenerateSendableFuncModule2)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<JSTaggedValue> moduleRecord = JSHandle<JSTaggedValue>::Cast(module);
    SendableClassModule::GenerateSendableFuncModule(thread, moduleRecord);
    JSHandle<SourceTextModule> currentModule = JSHandle<SourceTextModule>::Cast(moduleRecord);
    EXPECT_FALSE(SourceTextModule::IsModuleInSharedHeap(currentModule));
}

HWTEST_F_L0(EcmaModuleTest, CloneEnvForSModule1)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "a";
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<ImportEntry> importEntry =
        objectFactory->NewImportEntry(0, val, val, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module1, importEntry, 0, 1);
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolution = JSHandle<JSTaggedValue>::Cast(
        objectFactory->NewResolvedBindingRecord(module1, val));
    envRec->Set(thread, 0, resolution);
    module1->SetEnvironment(thread, envRec);
    module1->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<TaggedArray> elements = JSSharedModule::CloneEnvForSModule(thread, module1, envRec);
    EXPECT_TRUE(elements->GetLength() == 1U);
}

HWTEST_F_L0(EcmaModuleTest, TransferFromLocalToSharedModuleMapAndGetInsertedSModule) {
    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSSourceTextModule();
    CString recordName = "test";
    module1->SetEcmaModuleRecordNameString(recordName);
    module1->SetSharedType(SharedTypes::SHARED_MODULE);
    // success
    JSHandle<SourceTextModule> res =
        manager1->TransferFromLocalToSharedModuleMapAndGetInsertedSModule(thread, module1);
    EXPECT_EQ(res, module1);
    // insert fail
    JSHandle<SourceTextModule> module2 = objectFactory->NewSSourceTextModule();
    module2->SetEcmaModuleRecordNameString(recordName);
    module2->SetSharedType(SharedTypes::SHARED_MODULE);
    res = manager1->TransferFromLocalToSharedModuleMapAndGetInsertedSModule(thread, module2);
    EXPECT_EQ(res, module1);
}

HWTEST_F_L0(EcmaModuleTest, FindModuleMutexWithLock) {
    SharedModuleManager* manager1 = SharedModuleManager::GetInstance();
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module1 = objectFactory->NewSSourceTextModule();
    std::string baseFileNameStr = MODULE_ABC_PATH "module_unexecute.abc";
    CString baseFileName = baseFileNameStr.c_str();
    module1->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "module_unexecute";
    module1->SetEcmaModuleRecordNameString(recordName1);
    manager1->AddToResolvedModulesAndCreateSharedModuleMutex(thread, recordName1, module1.GetTaggedValue());
    manager1->FindModuleMutexWithLock(thread, module1);
    EXPECT_FALSE(thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, GetRecordName3)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module_base.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    int arkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();
    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(arkProperties | ArkProperties::ENABLE_ESM_TRACE);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module_base");
    EXPECT_TRUE(result);
}

HWTEST_F_L0(EcmaModuleTest, GetExportObjectIndex3)
{
    ecmascript::ThreadNativeScope nativeScope(thread);
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    ModuleLogger *moduleLogger = new ModuleLogger(thread->GetEcmaVM());
    thread->SetModuleLogger(moduleLogger);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    JSNApi::GetExportObject(instance, "module_test_module_test_B", "a");
    EXPECT_TRUE(result);
    thread->SetModuleLogger(nullptr);
    delete moduleLogger;
}

HWTEST_F_L0(EcmaModuleTest, GetModuleValueOuterInternal2)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    int arkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();
    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(arkProperties | ArkProperties::ENABLE_ESM_TRACE);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
}

HWTEST_F_L0(EcmaModuleTest, RemoveModule)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    thread->GetEcmaVM()->GetJSOptions().SetIsWorker(false);

    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    module->SetTypes(ModuleTypes::APP_MODULE);

    JSHandle<JSTaggedValue> res = moduleManager->TryGetImportedModule("module_test_module_test_C");
    EXPECT_NE(res, thread->GlobalConstants()->GetHandledUndefined());
    ModuleDeregister::RemoveModule(thread, module);

    res = moduleManager->TryGetImportedModule("module_test_module_test_C");
    EXPECT_EQ(res, thread->GlobalConstants()->GetHandledUndefined());
}

HWTEST_F_L0(EcmaModuleTest, RemoveModule2)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    thread->GetEcmaVM()->GetJSOptions().SetIsWorker(false);

    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    module->SetTypes(ModuleTypes::OHOS_MODULE);
    
    JSHandle<JSTaggedValue> res = moduleManager->TryGetImportedModule("module_test_module_test_C");
    EXPECT_NE(res, thread->GlobalConstants()->GetHandledUndefined());
    ModuleDeregister::RemoveModule(thread, module);

    res = moduleManager->TryGetImportedModule("module_test_module_test_C");
    EXPECT_EQ(res, thread->GlobalConstants()->GetHandledUndefined());
}

HWTEST_F_L0(EcmaModuleTest, IsEvaluatedModule)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);
    bool result = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    EXPECT_TRUE(result);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule("module_test_module_test_C");
    StateVisit stateVisit;
    bool res = SourceTextModule::IsEvaluatedModule(thread, stateVisit, module);
    EXPECT_TRUE(res);
}

HWTEST_F_L0(EcmaModuleTest, ResolveNativeStarExport2)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString recordName = "@ohos:hilog";
    module->SetEcmaModuleRecordNameString(recordName);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);

    int arkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();
    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(arkProperties | ArkProperties::ENABLE_ESM_TRACE);
    JSHandle<JSTaggedValue> res1 = SourceTextModule::ResolveNativeStarExport(thread, module, val);
    EXPECT_TRUE(res1->IsNull());
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModuleImpl2)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    ModuleLogger *moduleLogger = new ModuleLogger(vm);
    thread->SetModuleLogger(moduleLogger);

    // internal module
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSTaggedValue::SetProperty(thread, global, keyValue, thread->GlobalConstants()->GetHandledUndefined());
    module->SetEcmaModuleRecordNameString("@hms:xxxxx");
    int arkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();
    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(arkProperties | ArkProperties::ENABLE_ESM_TRACE);
    SourceTextModule::LoadNativeModuleImpl(vm, thread, module, ModuleTypes::INTERNAL_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
    thread->SetModuleLogger(nullptr);
    delete moduleLogger;
}

HWTEST_F_L0(EcmaModuleTest, LoadNativeModule3)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module->SetEcmaModuleRecordNameString("@app:bundleName/moduleName/lib*.so");
    module->SetLocalExportEntries(thread, localExportEntries);
    SourceTextModule::StoreModuleValue(thread, module, 0, val);
    module->SetTypes(ModuleTypes::NATIVE_MODULE);
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> keyValue = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("requireNapi"));
    JSHandle<JSFunction> current(objectFactory->NewJSFunction(globalEnv,
                                                              reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSFunction::SetFunctionExtraInfo(thread, current, reinterpret_cast<void *>(MockRequireNapiValue),
                                     nullptr, nullptr, 0);
    JSTaggedValue::SetProperty(thread, global, keyValue, JSHandle<JSTaggedValue>(current));
    int arkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();
    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(arkProperties | ArkProperties::ENABLE_ESM_TRACE);
    SourceTextModule::LoadNativeModule(thread, module, ModuleTypes::APP_MODULE);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, EvaluateForConcurrent)
{
    // Mock create sendable function
    auto objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();

    uint32_t methodOffset = 100;
    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(methodOffset));
    methodLiteral->SetIsShared(true);
    JSHandle<Method> method = objectFactory->NewSMethod(methodLiteral);

    module->SetStatus(ModuleStatus::INSTANTIATED);
    int res = SourceTextModule::EvaluateForConcurrent(thread, module, method);
    EXPECT_EQ(res, SourceTextModule::UNDEFINED_INDEX);
    
    module->SetStatus(ModuleStatus::EVALUATED);
    res = SourceTextModule::EvaluateForConcurrent(thread, module, method);
    EXPECT_EQ(res, SourceTextModule::UNDEFINED_INDEX);

    JSHandle<JSObject> error = objectFactory->GetJSError(
        base::ErrorType::RANGE_ERROR, "Stack overflow!", StackCheck::NO);
    thread->SetException(error.GetTaggedValue());
    res = SourceTextModule::EvaluateForConcurrent(thread, module, method);
    EXPECT_EQ(res, 0);
}

HWTEST_F_L0(EcmaModuleTest, ModuleDeclarationArrayEnvironmentSetup2)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    int arkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();
    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(arkProperties | ArkProperties::ENABLE_ESM_TRACE);
    SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());

    module->SetStatus(ModuleStatus::EVALUATED);
    SourceTextModule::ModuleDeclarationArrayEnvironmentSetup(thread, module);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, IsOhmUrl)
{
    CString ohmUrl = "@bundle:com.bundleName.test/moduleName/requestModuleName";
    bool res = ModulePathHelper::IsOhmUrl(ohmUrl);
    EXPECT_EQ(res, true);

    ohmUrl = "@package:pkg_modules@entry.@hw-agconnect.hmcore";
    res = ModulePathHelper::IsOhmUrl(ohmUrl);
    EXPECT_EQ(res, true);

    ohmUrl = "@normalized:N&&&har/Index&1.0.0";
    res = ModulePathHelper::IsOhmUrl(ohmUrl);
    EXPECT_EQ(res, true);

    ohmUrl = "ets/Test";
    res = ModulePathHelper::IsOhmUrl(ohmUrl);
    EXPECT_EQ(res, false);
}

HWTEST_F_L0(EcmaModuleTest, CheckAndGetRecordName)
{
    CString recordName = "";
    CString ohmUrl = "@bundle:com.bundleName.test/moduleName/requestModuleName";
    bool res = ModulePathHelper::CheckAndGetRecordName(thread, ohmUrl, recordName);
    EXPECT_EQ(res, true);

    ohmUrl = "@package:pkg_modules@entry.@hw-agconnect.hmcore";
    res = ModulePathHelper::CheckAndGetRecordName(thread, ohmUrl, recordName);
    EXPECT_EQ(res, true);

    ohmUrl = "@normalized:N&&&har/Index&1.0.0";
    res = ModulePathHelper::CheckAndGetRecordName(thread, ohmUrl, recordName);
    EXPECT_EQ(res, true);

    ohmUrl = "ets/Test";
    res = ModulePathHelper::CheckAndGetRecordName(thread, ohmUrl, recordName);
    EXPECT_EQ(res, false);
}

HWTEST_F_L0(EcmaModuleTest, ValidateAbcPath)
{
    CString baseFileName = "/data/storage/el1/bundle/com.bundleName.test/moduleName/moduleName/ets/modules.abc";
    bool res = ModulePathHelper::ValidateAbcPath(baseFileName, ValidateFilePath::ETS_MODULES);
    EXPECT_EQ(res, true);

    baseFileName = "";
    res = ModulePathHelper::ValidateAbcPath(baseFileName, ValidateFilePath::ETS_MODULES);
    EXPECT_EQ(res, false);

    baseFileName = "/data/storage/el1/bundle1/com.bundleName.test";
    res = ModulePathHelper::ValidateAbcPath(baseFileName, ValidateFilePath::ETS_MODULES);
    EXPECT_EQ(res, false);

    baseFileName = "/data/storage/el1/bundle/";
    res = ModulePathHelper::ValidateAbcPath(baseFileName, ValidateFilePath::ETS_MODULES);
    EXPECT_EQ(res, false);

    baseFileName = "/data/storage/el1/bundle/com.bundleName.test/moduleName/moduleName/ets/modules.ab";
    res = ModulePathHelper::ValidateAbcPath(baseFileName, ValidateFilePath::ETS_MODULES);
    EXPECT_EQ(res, false);

    baseFileName = "/data/storage/el1/bundle/com.bundleName.test/moduleName/moduleName/ets/modules.ab";
    res = ModulePathHelper::ValidateAbcPath(baseFileName, ValidateFilePath::ABC);
    EXPECT_EQ(res, false);
}


HWTEST_F_L0(EcmaModuleTest, ParseCrossModuleFile)
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
    CString requestPath = "moduleName/src/main/a/b/c";
    ModulePathHelper::ParseCrossModuleFile(pf.get(), requestPath);
    EXPECT_EQ(requestPath, "moduleName/src/main/a/b/c");

    requestPath="moduleName/src";
    ModulePathHelper::ParseCrossModuleFile(pf.get(), requestPath);
    EXPECT_EQ(requestPath, "moduleName/src");

    requestPath="moduleName/src";
    CString recordInfo1 = "moduleName";
    CString fieldInfo1 = "/src";
    pf->InsertNpmEntries(recordInfo1, fieldInfo1);
    ModulePathHelper::ParseCrossModuleFile(pf.get(), requestPath);
    EXPECT_EQ(requestPath, "/src");

    requestPath="moduleName/src/main/a/b/c";
    CString recordInfo2 = "moduleName";
    CString fieldInfo2 = "/src/main/a/b/c";
    pf->InsertNpmEntries(recordInfo2, fieldInfo2);
    ModulePathHelper::ParseCrossModuleFile(pf.get(), requestPath);
    EXPECT_EQ(requestPath, "/a/b/c");
}

HWTEST_F_L0(EcmaModuleTest, CheckAndThrowModuleError)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString baseFileName = "modules.abc";
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    JSHandle<SourceTextModule> module2 = objectFactory->NewSourceTextModule();
    module1->SetEcmaModuleFilenameString(baseFileName);
    module2->SetEcmaModuleFilenameString(baseFileName);
    CString recordName1 = "a";
    CString recordName2 = "b";
    module1->SetEcmaModuleRecordNameString(recordName1);
    module2->SetEcmaModuleRecordNameString(recordName2);
    module1->SetSharedType(SharedTypes::SHARED_MODULE);
    module1->SetStatus(ModuleStatus::EVALUATED);
    module2->SetStatus(ModuleStatus::EVALUATED);
    ModuleManager *moduleManager = thread->GetModuleManager();
    moduleManager->AddResolveImportedModule(recordName1, module1.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordName2, module2.GetTaggedValue());
    JSHandle<SourceTextModule> moduleRecord1 = moduleManager->HostGetImportedModule(recordName1);
    moduleRecord1->CheckAndThrowModuleError(thread);
    EXPECT_TRUE(!thread->HasPendingException());
    JSHandle<SourceTextModule> moduleRecord2 = moduleManager->HostGetImportedModule(recordName2);
    moduleRecord2->CheckAndThrowModuleError(thread);
    EXPECT_TRUE(!thread->HasPendingException());
}

HWTEST_F_L0(EcmaModuleTest, ModuleStatusOrder)
{
    EXPECT_EQ(static_cast<int>(ModuleStatus::UNINSTANTIATED), 0x00);
    EXPECT_EQ(static_cast<int>(ModuleStatus::PREINSTANTIATING), 0x01);
    EXPECT_EQ(static_cast<int>(ModuleStatus::INSTANTIATING), 0x02);
    EXPECT_EQ(static_cast<int>(ModuleStatus::INSTANTIATED), 0x03);
    EXPECT_EQ(static_cast<int>(ModuleStatus::EVALUATING), 0x04);
    EXPECT_EQ(static_cast<int>(ModuleStatus::EVALUATING_ASYNC), 0x05);
    EXPECT_EQ(static_cast<int>(ModuleStatus::EVALUATED), 0x06);
    EXPECT_EQ(static_cast<int>(ModuleStatus::ERRORED), 0x07);
}

HWTEST_F_L0(EcmaModuleTest, FindOhpmEntryPoint)
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

    //test requestName is empty string
    CString ohpmPath = "pkg_modules/0";
    CString requestName = "";
    CString result = "";
    CString entryPoint = ModulePathHelper::FindOhpmEntryPoint(pf.get(), ohpmPath, requestName);
    EXPECT_EQ(entryPoint, result);
}

HWTEST_F_L0(EcmaModuleTest, ResolveOhmUrlStartWithBundle)
{
    std::string ohmUrl = "@bundle:com.bundleName.test/moduleName/requestModuleName";
    auto result = ModulePathHelper::ResolveOhmUrl(ohmUrl);
    EXPECT_EQ(result.first, "requestModuleName");
    EXPECT_EQ(result.second, "com.bundleName.test/moduleName");
}

HWTEST_F_L0(EcmaModuleTest, ResolveOhmUrlStartWithNormalized)
{
    std::string ohmUrl = "@normalized:N&hsp&com.example.application&hsp/src/main/page/Test&1.0.0";
    auto result = ModulePathHelper::ResolveOhmUrl(ohmUrl);
    EXPECT_EQ(result.first, "hsp/src/main/page/Test");
    EXPECT_EQ(result.second, "com.example.application/hsp");
}

HWTEST_F_L0(EcmaModuleTest, GetResolvedModulesSize)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    EXPECT_EQ(moduleManager->GetResolvedModulesSize(), 0);

    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString recordName = "test";
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    EXPECT_EQ(moduleManager->GetResolvedModulesSize(), 1);
}

HWTEST_F_L0(EcmaModuleTest, AddNormalSerializeModule)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();

    CString recordName = "@ohos:hilog";
    JSHandle<TaggedArray> serializerArray = objectFactory->NewTaggedArray(1);
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();

    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    moduleManager->AddNormalSerializeModule(thread, serializerArray, 0);
    EXPECT_EQ(serializerArray->Get(thread, 0), module.GetTaggedValue());
}

HWTEST_F_L0(EcmaModuleTest, RestoreMutableFields)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSTaggedValue undefinedValue = thread->GlobalConstants()->GetUndefined();
    SourceTextModule::MutableFields fields;

    fields.TopLevelCapability = JSTaggedValue(1);
    fields.NameDictionary = JSTaggedValue(2);
    fields.CycleRoot = JSTaggedValue(3);
    fields.AsyncParentModules = JSTaggedValue(4);
    fields.SendableEnv = JSTaggedValue(5);
    fields.Exception = JSTaggedValue(6);
    fields.Namespace = JSTaggedValue(7);

    SourceTextModule::RestoreMutableFields(thread, module, fields);

    EXPECT_EQ(module->GetTopLevelCapability(thread), fields.TopLevelCapability);
    EXPECT_EQ(module->GetNameDictionary(thread), fields.NameDictionary);
    EXPECT_EQ(module->GetCycleRoot(thread), fields.CycleRoot);
    EXPECT_EQ(module->GetAsyncParentModules(thread), fields.AsyncParentModules);
    EXPECT_EQ(module->GetSendableEnv(thread), fields.SendableEnv);
    EXPECT_EQ(module->GetException(thread), fields.Exception);
    EXPECT_EQ(module->GetNamespace(thread), fields.Namespace);

    SourceTextModule::StoreAndResetMutableFields(thread, module, fields);

    EXPECT_EQ(module->GetTopLevelCapability(thread), undefinedValue);
    EXPECT_EQ(module->GetNameDictionary(thread), undefinedValue);
    EXPECT_EQ(module->GetCycleRoot(thread), undefinedValue);
    EXPECT_EQ(module->GetAsyncParentModules(thread), undefinedValue);
    EXPECT_EQ(module->GetSendableEnv(thread), undefinedValue);
    EXPECT_EQ(module->GetException(thread), undefinedValue);
    EXPECT_EQ(module->GetNamespace(thread), undefinedValue);
}

HWTEST_F_L0(EcmaModuleTest, UpdateSharedModule)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> curmodule = objectFactory->NewSourceTextModule();
    JSHandle<SourceTextModule> resolvedModule = objectFactory->NewSSourceTextModule();
    resolvedModule->SetEcmaModuleRecordNameString("updateSharedModule");
    resolvedModule->SetSharedType(SharedTypes::SHARED_MODULE);
    JSHandle<TaggedArray> envRec = objectFactory->NewTaggedArray(1);
    JSHandle<JSTaggedValue> resolvedBinding = JSHandle<JSTaggedValue>::Cast(
        objectFactory->NewResolvedIndexBindingRecord(resolvedModule, 0));
    envRec->Set(thread, 0, resolvedBinding);
    curmodule->SetEnvironment(thread, envRec);
    JSTaggedValue res = MockModuleValueAccessor::GetModuleValueFromIndexBinding<false>(
        thread, curmodule, resolvedBinding.GetTaggedValue(), false);
    EXPECT_TRUE(res.IsHole());

    JSHandle<SourceTextModule> resolvedModule2 = objectFactory->NewSSourceTextModule();
    CString recordName = "updateSharedModule";
    resolvedModule2->SetEcmaModuleRecordNameString(recordName);
    resolvedModule2->SetSharedType(SharedTypes::SHARED_MODULE);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewSLocalExportEntry(JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromASCII("exportName")),
                                      JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromASCII("localName")), 1);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewSTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    resolvedModule2->SetLocalExportEntries(thread, localExportEntries);
    JSHandle<JSTaggedValue> val(objectFactory->NewFromUtf8("exportVal"));

    SharedModuleManager::GetInstance()->
        TransferFromLocalToSharedModuleMapAndGetInsertedSModule(thread, resolvedModule2);
    SourceTextModule::StoreModuleValue(thread, resolvedModule2, 0, val);
    res = MockModuleValueAccessor::GetModuleValueFromIndexBinding<false>(
        thread, curmodule, resolvedBinding.GetTaggedValue(), false);
    EXPECT_EQ(res, val.GetTaggedValue());
}

HWTEST_F_L0(EcmaModuleTest, DeregisterModuleList)
{
    EcmaVM *vm = thread->GetEcmaVM();
    vm->PushToDeregisterModuleList("@ohos:hilog");
    vm->PushToDeregisterModuleList("com.bundleName.test/moduleName/requestModuleName");
    vm->PushToDeregisterModuleList("&hsp&com.example.application&hsp/src/main/page/Test&1.0.0");
    EXPECT_EQ(vm->ContainInDeregisterModuleList("@ohos:hilog"), true);
    EXPECT_EQ(vm->ContainInDeregisterModuleList("com.bundleName.test/moduleName/requestModuleName"), true);
    EXPECT_EQ(vm->ContainInDeregisterModuleList("com.bundleName.test/moduleName/requestModuleName1"), false);

    vm->RemoveFromDeregisterModuleList("@ohos:hilog");
    vm->RemoveFromDeregisterModuleList("com.bundleName.test/moduleName/requestModuleName");
    EXPECT_EQ(vm->ContainInDeregisterModuleList("@ohos:hilog"), false);
    EXPECT_EQ(vm->ContainInDeregisterModuleList("com.bundleName.test/moduleName/requestModuleName"), false);
}

HWTEST_F(EcmaModuleTest, Deregister, TestSize.Level0)
{
    CString baseFileName = MODULE_ABC_PATH "deregister_test.abc";
    CString recordName = "entry";
    CString recordNameA = "A";

    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteFromFile(thread, baseFileName, recordName);
    EXPECT_TRUE(result);
    JSHandle<SourceTextModule> moduleA =
        thread->GetModuleManager()->GetImportedModule(recordNameA);
    JSHandle<JSTaggedValue> recordNameARecord = JSHandle<JSTaggedValue>::Cast(moduleA);
    JSTaggedValue val = ModuleValueAccessor::GetModuleValueInner(thread, 0, recordNameARecord);
    EXPECT_EQ(val, JSTaggedValue(20));
    uint32_t normalModuleSize = thread->GetModuleManager()->GetResolvedModulesSize();
    EXPECT_EQ(normalModuleSize, 3);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 0);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    builtins::BuiltinsArkTools::ForceFullGC(ecmaRuntimeCallInfo);
    normalModuleSize = thread->GetModuleManager()->GetResolvedModulesSize();
    EXPECT_EQ(normalModuleSize, 1);
}
}  // namespace panda::test
