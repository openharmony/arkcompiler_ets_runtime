/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class SharedObjectFactoryTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(SharedObjectFactoryTest, NewSModuleNamespaceTest001)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<ModuleNamespace> np = factory->NewSModuleNamespace();
    ModuleNamespace::PreventExtensions();
    JSHandle<JSTaggedValue> moduleNamespaceTag = JSHandle<JSTaggedValue>::Cast(np);
    Local<JSValueRef> moduleNamespace = JSNApiHelper::ToLocal<ModuleNamespace>(moduleNamespaceTag);
    ASSERT_EQ(moduleNamespace->IsModuleNamespaceObject(vm), true);
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSImportEntryTest001)
{
    ObjectFactory *factory = instance->GetFactory();
    size_t requestIndex = 1;
    JSHandle<EcmaString> importName = factory->NewFromASCII("importName");
    JSHandle<EcmaString> localName = factory->NewFromASCII("localName");

    JSHandle<ImportEntry> entry = factory->NewSImportEntry(requestIndex,
                                                           JSHandle<JSTaggedValue>::Cast(importName),
                                                           JSHandle<JSTaggedValue>::Cast(localName));
    ASSERT_EQ(entry->GetImportName(thread).GetRawData(), JSHandle<JSTaggedValue>::Cast(importName)->GetRawData());
    ASSERT_EQ(entry->GetLocalName(thread).GetRawData(), JSHandle<JSTaggedValue>::Cast(localName)->GetRawData());
    ASSERT_EQ(entry->GetModuleRequestIndex(), 1);
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSLocalExportEntryTest001)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<LocalExportEntry> entry =
        factory->NewSLocalExportEntry(JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("exportName")),
                                      JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("localName")), 1);
    ASSERT_EQ(entry->GetExportName(thread).GetRawData(),
              JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("exportName"))->GetRawData());
    ASSERT_EQ(entry->GetLocalName(thread).GetRawData(),
              JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("localName"))->GetRawData());
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSIndirectExportEntryTest001)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<IndirectExportEntry> entry = factory->NewSIndirectExportEntry(
        JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("exportName")), 0,
        JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("importName")));
    ASSERT_EQ(entry->GetExportName(thread).GetRawData(),
              JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("exportName"))->GetRawData());
    ASSERT_EQ(entry->GetImportName(thread).GetRawData(),
              JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("importName"))->GetRawData());
    ASSERT_EQ(entry->GetModuleRequestIndex(), 0);
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSStarExportEntryTest001)
{
    ObjectFactory *factory = instance->GetFactory();

    JSHandle<StarExportEntry> entry = factory->NewSStarExportEntry(0);
    ASSERT_EQ(entry->GetModuleRequestIndex(), 0);
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSResolvedIndexBindingRecordTest001)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<ResolvedIndexBinding> record = factory->NewSResolvedIndexBindingRecord();
    ASSERT_NE(record->GetModule(thread).GetRawData(), JSTaggedValue::Exception().GetRawData());
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSResolvedIndexBindingRecordTest002)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSTaggedValue> undefinedValue = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<SourceTextModule> ecmaModule(undefinedValue);
    JSHandle<ResolvedIndexBinding> record = factory->NewSResolvedIndexBindingRecord(ecmaModule, 2);
    ASSERT_NE(record->GetModule(thread).GetRawData(), JSTaggedValue::Exception().GetRawData());
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSResolvedBindingRecordTest001)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<ResolvedRecordBinding> record = factory->NewSResolvedRecordBindingRecord();
    JSHandle<JSTaggedValue> undefinedValue = thread->GlobalConstants()->GetHandledUndefined();
    ASSERT_EQ(record->GetBindingName(thread).GetRawData(), undefinedValue->GetRawData());
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSResolvedBindingRecordTest002)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<ResolvedRecordBinding> record = factory->NewSResolvedRecordBindingRecord(
        factory->NewFromASCII("moduleRecord"),
        JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("bindName")));
    ASSERT_EQ(record->GetBindingName(thread).GetRawData(),
              JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("bindName"))->GetRawData());
}

HWTEST_F_L0(SharedObjectFactoryTest, CopySArrayTest001)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(5);
    JSHandle<TaggedArray> result = factory->CopySArray(array, 5, 10);
    for (size_t i = 0; i < 5; i++)
    {
        ASSERT_EQ(array->Get(thread, i).GetRawData(), result->Get(thread, i).GetRawData());
    }
    for (size_t i = 5; i < 10; i++)
    {
        ASSERT_EQ(result->Get(thread, i).GetRawData(), JSTaggedValue::Hole().GetRawData());
    }
}

HWTEST_F_L0(SharedObjectFactoryTest, CopySArrayTest002)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(10);
    JSHandle<TaggedArray> result = factory->CopySArray(array, 10, 5);
    for (size_t i = 0; i < 5; i++)
    {
        ASSERT_EQ(array->Get(thread, i).GetRawData(), result->Get(thread, i).GetRawData());
    }
}

HWTEST_F_L0(SharedObjectFactoryTest, CopySArrayTest003)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(5);
    JSHandle<TaggedArray> result = factory->CopySArray(array, 5, 0);
    ASSERT_EQ(result->GetLength(), 0);
}

HWTEST_F_L0(SharedObjectFactoryTest, NewSClassInfoExtractorTest001)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<ClassInfoExtractor> info =
        factory->NewSClassInfoExtractor(JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("method")));
    ASSERT_EQ(info->GetConstructorMethod(thread).GetRawData(),
              JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("method"))->GetRawData());
}
} // namespace panda::test