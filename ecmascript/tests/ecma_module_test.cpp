/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/linked_hash_table.h"


using namespace panda::ecmascript;

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
        objFactory->NewLocalExportEntry(exportNameHandle, localNameHandle);
    JSHandle<SourceTextModule> module = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, module, localExportEntry, 0, 1);

    JSHandle<JSTaggedValue> storeKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSHandle<JSTaggedValue> valueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value));
    module->StoreModuleValue(thread, storeKey, valueHandle);

    JSHandle<JSTaggedValue> loadKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName));
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
    JSHandle<LocalExportEntry> localExportEntry =
        objFactory->NewLocalExportEntry(exportNameHandle, exportLocalNameHandle);
    JSHandle<SourceTextModule> moduleExport = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, moduleExport, localExportEntry, 0, 1);
    // store module value
    JSHandle<JSTaggedValue> exportValueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportValue));
    moduleExport->StoreModuleValue(thread, exportLocalNameHandle, exportValueHandle);

    JSTaggedValue importDefaultValue =
        moduleExport->GetModuleValue(thread, exportNameHandle.GetTaggedValue(), false);
    EXPECT_EQ(exportValueHandle.GetTaggedValue(), importDefaultValue);
}
}  // namespace panda::test
