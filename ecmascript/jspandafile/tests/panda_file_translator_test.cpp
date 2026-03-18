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
#include "class_data_accessor-inl.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "gtest/gtest.h"
#include "libpandabase/utils/utf.h"
#define private public
#define protected public
#include "ecmascript/jspandafile/program_object.h"
#undef protected
#undef private
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class PandaFileTranslatorTest : public testing::Test {
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
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
protected:
    std::shared_ptr<JSPandaFile> CreateJSPandaFile(const char *source, const CString filename)
    {
        Parser parser;
        const std::string fn = "SRC.pa";
        auto res = parser.Parse(source, fn);
        EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), filename);
        return pf;
    }
};

HWTEST_F_L0(PandaFileTranslatorTest, GenerateProgram)
{
    Parser parser;
    auto vm = thread->GetEcmaVM();
    const char *filename = "__PandaFileTranslatorTest1.pa";
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const char *data = R"(
        .function any func_main_0(any a0, any a1, any a2) {}
        .function void func() {}
    )";
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral *method1 = new MethodLiteral(methodId[0]);
    MethodLiteral *method2 = new MethodLiteral(methodId[1]);
    pf->SetMethodLiteralToMap(method1);
    pf->SetMethodLiteralToMap(method2);
    pfManager->AddJSPandaFile(pf);

    JSHandle<ecmascript::Program> program1 = pfManager->GenerateProgram(vm, pf.get(), std::string_view("func"));
    JSHandle<JSFunction> mainFunc1(thread, program1->GetMainFunction(thread));
    JSHandle<JSTaggedValue> funcName1 = JSFunction::GetFunctionName(thread, JSHandle<JSFunctionBase>(mainFunc1));
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName1)).ToCString(thread).c_str(), "func");

    pf->UpdateMainMethodIndex(methodId[1].GetOffset());
    JSHandle<ecmascript::Program> program2 = pfManager->GenerateProgram(vm, pf.get(), JSPandaFile::ENTRY_FUNCTION_NAME);
    JSHandle<JSFunction> mainFunc2(thread, program2->GetMainFunction(thread));
    JSHandle<JSTaggedValue> funcName2 = JSFunction::GetFunctionName(thread, JSHandle<JSFunctionBase>(mainFunc2));
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName2)).ToCString(thread).c_str(), "func_main_0");
}

HWTEST_F_L0(PandaFileTranslatorTest, TranslateClasses)
{
    Parser parser;
    const char *filename = "__PandaFileTranslatorTest2.pa";
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const char *data = R"(
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    pfManager->AddJSPandaFile(pf);
    EXPECT_TRUE(pf->FindMethodLiteral(methodId[0].GetOffset()) == nullptr);

    const char *methodName = MethodLiteral::GetMethodName(pf.get(), methodId[0]);
    PandaFileTranslator::TranslateClasses(thread, pf.get(), CString(methodName));
    EXPECT_TRUE(pf->FindMethodLiteral(methodId[0].GetOffset()) != nullptr);
    EXPECT_EQ(pf->FindMethodLiteral(methodId[0].GetOffset())->GetFunctionKind(),
                                    ecmascript::FunctionKind::NONE_FUNCTION);
}

HWTEST_F_L0(PandaFileTranslatorTest, AllocateConstPool)
{
    Parser parser;
    const char *filename = "__PandaFileTranslatorTest2.pa";
    const char *data = R"(
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    JSHandle<ConstantPool> constpool = PandaFileTranslator::AllocateConstPoolForTest(thread->GetEcmaVM(), pf.get());
    EXPECT_EQ(constpool->GetSharedConstpoolId(), JSTaggedValue(ConstantPool::CONSTPOOL_TYPE_FLAG));
}

HWTEST_F_L0(PandaFileTranslatorTest, MergeObjectLiteralHClassCache_AotHCInfoNotTaggedArray)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);
    pool->SetAotHClassInfo(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> constpool(pool);

    JSTaggedValue aotHCInfoBefore = pool->GetAotHClassInfo(thread);
    EXPECT_EQ(aotHCInfoBefore, JSTaggedValue::Undefined());

    ConstantPool::MergeObjectLiteralHClassCache(thread->GetEcmaVM(), constpool);

    JSTaggedValue aotHCInfoAfter = pool->GetAotHClassInfo(thread);
    EXPECT_EQ(aotHCInfoAfter, JSTaggedValue::Undefined());
}

HWTEST_F_L0(PandaFileTranslatorTest, MergeObjectLiteralHClassCache_AotHCInfoArrayLengthZero)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);
    JSHandle<TaggedArray> emptyArray = factory->NewTaggedArray(0);
    pool->SetAotHClassInfo(thread, emptyArray.GetTaggedValue());
    JSHandle<JSTaggedValue> constpool(pool);

    JSTaggedValue aotHCInfoBefore = pool->GetAotHClassInfo(thread);
    EXPECT_TRUE(aotHCInfoBefore.IsTaggedArray());
    EXPECT_EQ(TaggedArray::Cast(aotHCInfoBefore.GetTaggedObject())->GetLength(), 0U);

    ConstantPool::MergeObjectLiteralHClassCache(thread->GetEcmaVM(), constpool);

    JSTaggedValue aotHCInfoAfter = pool->GetAotHClassInfo(thread);
    EXPECT_TRUE(aotHCInfoAfter.IsTaggedArray());
    EXPECT_EQ(TaggedArray::Cast(aotHCInfoAfter.GetTaggedObject())->GetLength(), 0U);
}

HWTEST_F_L0(PandaFileTranslatorTest, MergeObjectLiteralHClassCache_LastNotTaggedArray)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);
    JSHandle<TaggedArray> aotHCInfoArray = factory->NewTaggedArray(2);
    aotHCInfoArray->Set(thread, 0, JSTaggedValue::Undefined());
    aotHCInfoArray->Set(thread, 1, JSTaggedValue::Undefined());
    pool->SetAotHClassInfo(thread, aotHCInfoArray.GetTaggedValue());
    JSHandle<JSTaggedValue> constpool(pool);

    JSTaggedValue lastBefore = aotHCInfoArray->Get(thread, 1);
    EXPECT_FALSE(lastBefore.IsTaggedArray());

    ConstantPool::MergeObjectLiteralHClassCache(thread->GetEcmaVM(), constpool);

    JSTaggedValue lastAfter = aotHCInfoArray->Get(thread, 1);
    EXPECT_FALSE(lastAfter.IsTaggedArray());
}

HWTEST_F_L0(PandaFileTranslatorTest, MergeObjectLiteralHClassCache_CurCachedIsHole)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);

    JSHandle<TaggedArray> snapshotCachedArray = factory->NewTaggedArray(2);
    snapshotCachedArray->Set(thread, 0, JSTaggedValue::Undefined());
    snapshotCachedArray->Set(thread, 1, JSTaggedValue::Undefined());

    JSHandle<TaggedArray> aotHCInfoArray = factory->NewTaggedArray(1);
    aotHCInfoArray->Set(thread, 0, snapshotCachedArray.GetTaggedValue());

    pool->SetAotHClassInfo(thread, aotHCInfoArray.GetTaggedValue());

    JSHandle<JSTaggedValue> constpool(pool);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    EXPECT_TRUE(env->GetObjectLiteralHClassCache().GetTaggedValue().IsHole());

    ConstantPool::MergeObjectLiteralHClassCache(thread->GetEcmaVM(), constpool);

    JSTaggedValue newCache = env->GetObjectLiteralHClassCache().GetTaggedValue();
    EXPECT_FALSE(newCache.IsHole());
    EXPECT_EQ(newCache, snapshotCachedArray.GetTaggedValue());
}

HWTEST_F_L0(PandaFileTranslatorTest, MergeObjectLiteralHClassCache_CurValueIsJSHClassAndAOT)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSHClass> hclass1 = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT,
        env->GetFunctionPrototype());
    hclass1->SetAOT(true);

    JSHandle<JSHClass> hclass2 = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT,
        env->GetFunctionPrototype());

    JSHandle<JSHClass> hclass3 = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT,
        env->GetFunctionPrototype());

    JSHandle<TaggedArray> curCachedArray = factory->NewTaggedArray(3);
    curCachedArray->Set(thread, 0, JSTaggedValue::Undefined());
    curCachedArray->Set(thread, 1, hclass1.GetTaggedValue());
    curCachedArray->Set(thread, 2, JSTaggedValue::Undefined());

    env->SetObjectLiteralHClassCache(thread, curCachedArray.GetTaggedValue());

    JSHandle<TaggedArray> snapshotCachedArray = factory->NewTaggedArray(3);
    snapshotCachedArray->Set(thread, 0, hclass2.GetTaggedValue());
    snapshotCachedArray->Set(thread, 1, hclass3.GetTaggedValue());
    snapshotCachedArray->Set(thread, 2, JSTaggedValue::Undefined());

    JSHandle<TaggedArray> aotHCInfoArray = factory->NewTaggedArray(1);
    aotHCInfoArray->Set(thread, 0, snapshotCachedArray.GetTaggedValue());

    pool->SetAotHClassInfo(thread, aotHCInfoArray.GetTaggedValue());

    JSHandle<JSTaggedValue> constpool(pool);

    ConstantPool::MergeObjectLiteralHClassCache(thread->GetEcmaVM(), constpool);

    JSTaggedValue curCachedAfter0 = curCachedArray->Get(thread, 0);
    EXPECT_EQ(curCachedAfter0, hclass2.GetTaggedValue());

    JSTaggedValue curCachedAfter1 = curCachedArray->Get(thread, 1);
    EXPECT_EQ(curCachedAfter1, hclass1.GetTaggedValue());

    JSTaggedValue curCachedAfter2 = curCachedArray->Get(thread, 2);
    EXPECT_EQ(curCachedAfter2, JSTaggedValue::Undefined());
}

HWTEST_F_L0(PandaFileTranslatorTest, GetMethodFromCache_IsLoadingAOTMethodInfo)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);

    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    pool->SetJSPandaFile(pf.get());

    JSHandle<AOTLiteralInfo> aotLiteralInfo = factory->NewAOTLiteralInfo(2);
    aotLiteralInfo->Set(thread, 0, JSTaggedValue(1));
    aotLiteralInfo->Set(thread, 1, JSTaggedValue(2));

    pool->SetObjectToCache(thread, 0, aotLiteralInfo.GetTaggedValue());

    pf->SetAOTFileInfoIndex(0);

    JSHandle<JSTaggedValue> constpool(pool);
    JSTaggedValue result = ConstantPool::GetMethodFromCache(constpool.GetTaggedValue(), 0, thread);

    EXPECT_TRUE(result.IsUndefined());
}

HWTEST_F_L0(PandaFileTranslatorTest, GetMethodFromCache_NotLoadingAOTMethodInfo)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    pool->SetJSPandaFile(pf.get());

    JSHandle<JSFunction> function = factory->NewJSFunction(env);

    pool->SetObjectToCache(thread, 0, function.GetTaggedValue());

    JSHandle<JSTaggedValue> constpool(pool);
    JSTaggedValue result = ConstantPool::GetMethodFromCache(constpool.GetTaggedValue(), 0, thread);

    EXPECT_EQ(result, function.GetTaggedValue());
}

HWTEST_F_L0(PandaFileTranslatorTest, GetIhcFromAOTLiteralInfo_IsAOTLiteralInfo)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> function = factory->NewJSFunction(env);

    JSHandle<AOTLiteralInfo> aotLiteralInfo = factory->NewAOTLiteralInfo(2);
    aotLiteralInfo->SetIhc(thread, function.GetTaggedValue());

    pool->Set(thread, 0, aotLiteralInfo.GetTaggedValue());

    JSHandle<JSTaggedValue> constpool(pool);
    JSTaggedValue result = ConstantPool::GetIhcFromAOTLiteralInfo(thread, constpool.GetTaggedValue(), 0);

    EXPECT_EQ(result, function.GetTaggedValue());
}

HWTEST_F_L0(PandaFileTranslatorTest, GetIhcFromAOTLiteralInfo_NotAOTLiteralInfo)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = {factory->NewConstantPool(10)};

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> function = factory->NewJSFunction(env);

    pool->Set(thread, 0, function.GetTaggedValue());

    JSHandle<JSTaggedValue> constpool(pool);
    JSTaggedValue result = ConstantPool::GetIhcFromAOTLiteralInfo(thread, constpool.GetTaggedValue(), 0);

    EXPECT_TRUE(result.IsUndefined());
}

HWTEST_F_L0(PandaFileTranslatorTest, GetIhcFromAOTLiteralInfo_NotHeapObject)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> pool = factory->NewConstantPool(10);

    pool->Set(thread, 0, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> constpool(pool);
    JSTaggedValue result = ConstantPool::GetIhcFromAOTLiteralInfo(thread, constpool.GetTaggedValue(), 0);

    EXPECT_TRUE(result.IsUndefined());
}
HWTEST_F_L0(PandaFileTranslatorTest, UpdateConstpoolWhenDeserialAI_SkipSpecialIndices)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> aiCP = factory->NewConstantPool(10);
    JSHandle<ConstantPool> sharedCP = factory->NewConstantPool(10);
    JSHandle<ConstantPool> unsharedCP = factory->NewConstantPool(10);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> function = factory->NewJSFunction(env);

    for (uint32_t i = 0; i < 10; i++) {
        aiCP->SetObjectToCache(thread, i, function.GetTaggedValue());
    }

    ConstantPool::UpdateConstpoolWhenDeserialAI(thread->GetEcmaVM(), aiCP, sharedCP, unsharedCP);
}

HWTEST_F_L0(PandaFileTranslatorTest, UpdateConstpoolWhenDeserialAI_ProcessNormalIndex)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> aiCP = factory->NewConstantPool(10);
    JSHandle<ConstantPool> sharedCP = factory->NewConstantPool(10);
    JSHandle<ConstantPool> unsharedCP = factory->NewConstantPool(10);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> function = factory->NewJSFunction(env);

    JSHandle<AOTLiteralInfo> aotLiteralInfo = factory->NewAOTLiteralInfo(2);
    aotLiteralInfo->SetIhc(thread, function.GetTaggedValue());

    aiCP->SetObjectToCache(thread, 0, aotLiteralInfo.GetTaggedValue());

    ConstantPool::UpdateConstpoolWhenDeserialAI(thread->GetEcmaVM(), aiCP, sharedCP, unsharedCP);

    JSTaggedValue unsharedVal = unsharedCP->GetObjectFromCache(thread, 0);
    EXPECT_EQ(unsharedVal, aotLiteralInfo.GetTaggedValue());
}

HWTEST_F_L0(PandaFileTranslatorTest, UpdateConstpoolWhenDeserialAI_IsAOTLiteralInfo)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ConstantPool> aiCP = factory->NewConstantPool(10);
    JSHandle<ConstantPool> sharedCP = factory->NewConstantPool(10);
    JSHandle<ConstantPool> unsharedCP = factory->NewConstantPool(10);

    JSHandle<AOTLiteralInfo> aotLiteralInfo = factory->NewAOTLiteralInfo(2);
    aotLiteralInfo->Set(thread, 0, JSTaggedValue(1));
    aotLiteralInfo->Set(thread, 1, JSTaggedValue(2));

    aiCP->SetObjectToCache(thread, 0, aotLiteralInfo.GetTaggedValue());

    ConstantPool::UpdateConstpoolWhenDeserialAI(thread->GetEcmaVM(), aiCP, sharedCP, unsharedCP);

    JSTaggedValue unsharedVal = unsharedCP->GetObjectFromCache(thread, 0);
    EXPECT_EQ(unsharedVal, aotLiteralInfo.GetTaggedValue());
}

}  // namespace panda::test
