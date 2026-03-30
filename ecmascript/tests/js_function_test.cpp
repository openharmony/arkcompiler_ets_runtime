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

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "ecmascript/js_function.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

using namespace panda::ecmascript::base;

namespace panda::test {
class JSFunctionTest : public BaseTestWithScope<false> {
};

JSFunction *JSObjectCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(JSFunctionTest, Create)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> funHandle = thread->GetEcmaVM()->GetFactory()->NewJSFunction(env);
    EXPECT_TRUE(*funHandle != nullptr);
    EXPECT_EQ(funHandle->GetProtoOrHClass(thread), JSTaggedValue::Hole());

    JSHandle<LexicalEnv> lexicalEnv = thread->GetEcmaVM()->GetFactory()->NewLexicalEnv(0);
    funHandle->SetLexicalEnv(thread, lexicalEnv.GetTaggedValue());
    EXPECT_EQ(funHandle->GetLexicalEnv(thread), lexicalEnv.GetTaggedValue());
    EXPECT_TRUE(*lexicalEnv != nullptr);
}
HWTEST_F_L0(JSFunctionTest, MakeConstructor)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> func = thread->GetEcmaVM()->GetFactory()->NewJSFunction(env, static_cast<void *>(nullptr),
                                                                                 FunctionKind::BASE_CONSTRUCTOR);
    EXPECT_TRUE(*func != nullptr);
    JSHandle<JSTaggedValue> funcHandle(func);
    func->GetJSHClass()->SetExtensible(true);

    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSObject> obj = JSObject::ObjectCreate(thread, nullHandle);
    JSHandle<JSTaggedValue> objValue(obj);

    JSFunction::MakeConstructor(thread, func, objValue);

    JSHandle<JSTaggedValue> constructorKey(
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("constructor"));

    JSHandle<JSTaggedValue> protoKey(thread->GetEcmaVM()->GetFactory()->NewFromASCII("prototype"));
    JSTaggedValue proto = JSObject::GetProperty(thread, funcHandle, protoKey).GetValue().GetTaggedValue();
    JSTaggedValue constructor =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(obj), constructorKey).GetValue().GetTaggedValue();
    EXPECT_EQ(constructor, funcHandle.GetTaggedValue());
    EXPECT_EQ(proto, obj.GetTaggedValue());
    EXPECT_EQ(func->GetFunctionKind(thread), FunctionKind::BASE_CONSTRUCTOR);
}

HWTEST_F_L0(JSFunctionTest, OrdinaryHasInstance)
{
    JSHandle<JSTaggedValue> objFun(thread, JSObjectCreate(thread));

    JSHandle<JSObject> jsobject =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> obj(thread, jsobject.GetTaggedValue());
    EXPECT_TRUE(*jsobject != nullptr);

    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = globalEnv->GetObjectFunction();
    EXPECT_TRUE(ecmascript::JSFunction::OrdinaryHasInstance(thread, constructor, obj));
}

JSTaggedValue TestInvokeInternal(EcmaRuntimeCallInfo *argv)
{
    if (argv->GetArgsNumber() == 1 && argv->GetCallArg(0).GetTaggedValue() == JSTaggedValue(1)) {
        return BuiltinsBase::GetTaggedBoolean(true);
    } else {
        return BuiltinsBase::GetTaggedBoolean(false);
    }
}

HWTEST_F_L0(JSFunctionTest, Invoke)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> hclass(thread, JSObjectCreate(thread));
    JSHandle<JSTaggedValue> callee(
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    EXPECT_TRUE(*callee != nullptr);

    char keyArray[] = "invoked";
    JSHandle<JSTaggedValue> calleeKey(thread->GetEcmaVM()->GetFactory()->NewFromASCII(&keyArray[0]));
    JSHandle<JSFunction> calleeFunc =
        thread->GetEcmaVM()->GetFactory()->NewJSFunction(env, reinterpret_cast<void *>(TestInvokeInternal));
    calleeFunc->SetCallable(true);
    JSHandle<JSTaggedValue> calleeValue(calleeFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(callee), calleeKey, calleeValue);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, callee, undefined, 1);
    info->SetCallArg(JSTaggedValue(1));
    JSTaggedValue res = JSFunction::Invoke(info, calleeKey);

    JSTaggedValue ruler = BuiltinsBase::GetTaggedBoolean(true);
    EXPECT_EQ(res.GetRawData(), ruler.GetRawData());
}

HWTEST_F_L0(JSFunctionTest, SetSymbolFunctionName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> jsFunction = factory->NewJSFunction(env);
    JSHandle<JSSymbol> symbol = factory->NewPublicSymbolWithChar("name");
    JSHandle<EcmaString> name = factory->NewFromASCII("[name]");
    JSHandle<JSTaggedValue> prefix(thread, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(jsFunction), JSHandle<JSTaggedValue>(symbol), prefix);
    JSHandle<JSTaggedValue> functionName =
        JSFunctionBase::GetFunctionName(thread, JSHandle<JSFunctionBase>(jsFunction));
    EXPECT_TRUE(functionName->IsString());
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *(JSHandle<EcmaString>(functionName)), *name));
}

HWTEST_F_L0(JSFunctionTest, ReplaceJSFunctionForHook)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
    )";
    const CString fileName = "test.pa";
    // create MethodLiteral
    pandasm::Parser parser;
    auto res = parser.Parse(source, "SRC.pa");
    std::unique_ptr<const panda_file::File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    MethodLiteral *methodLiterals = pf->GetMethodLiterals();
    MethodLiteral& foo1MethodLiteral = methodLiterals[0];
    MethodLiteral& foo2MethodLiteral = methodLiterals[1];

    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();
    JSHandle<Method> foo1Method = factory->NewMethod(&foo1MethodLiteral);
    JSHandle<Method> foo2Method = factory->NewMethod(&foo2MethodLiteral);

    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("module");
    JSHandle<JSFunction> foo1Function = factory->NewJSFunction(globalEnv, foo1Method);
    foo1Function->SetLength(1);
    foo1Function->SetCallNapi(true);
    JSHandle<LexicalEnv> lexicalEnv = factory->NewLexicalEnv(1);
    foo1Function->SetLexicalEnv(thread, lexicalEnv);
    JSHandle<JSFunction> foo2Function = factory->NewJSFunction(globalEnv, foo2Method);
    foo2Function->SetLength(2);
    foo2Function->SetCallNapi(false);
    foo2Function->SetHomeObject(thread, module.GetTaggedValue());
    foo2Function->SetModule(thread, module.GetTaggedValue());

    JSFunction::ReplaceFunctionForHook(thread, foo1Function, foo2Function);
    EXPECT_EQ(foo1Function->GetMethod(thread), foo2Function->GetMethod(thread));
    EXPECT_EQ(foo1Function->GetCodeEntryOrNativePointer(), foo2Function->GetCodeEntryOrNativePointer());
    EXPECT_EQ(foo1Function->GetLength(), foo2Function->GetLength());
    EXPECT_EQ(foo1Function->GetBitField(), foo2Function->GetBitField());
    EXPECT_EQ(foo1Function->GetProtoOrHClass(thread), foo2Function->GetProtoOrHClass(thread));
    EXPECT_EQ(foo1Function->GetLexicalEnv(thread), foo2Function->GetLexicalEnv(thread));
    EXPECT_EQ(foo1Function->GetHomeObject(thread), foo2Function->GetHomeObject(thread));
    EXPECT_EQ(foo1Function->GetRawProfileTypeInfo(thread), foo2Function->GetRawProfileTypeInfo(thread));
    EXPECT_EQ(foo1Function->GetMachineCode(thread), foo2Function->GetMachineCode(thread));
    EXPECT_EQ(foo1Function->GetBaselineCode(thread), foo2Function->GetBaselineCode(thread));
    EXPECT_EQ(foo1Function->GetModule(thread), foo2Function->GetModule(thread));
#if !ENABLE_MEMORY_OPTIMIZATION
    EXPECT_EQ(foo1Function->GetWorkNodePointer(), foo2Function->GetWorkNodePointer());
#endif
}

HWTEST_F_L0(JSFunctionTest, ReplaceJSFunctionForHookMethodIsSame)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    // create MethodLiteral
    pandasm::Parser parser;
    auto res = parser.Parse(source, "SRC.pa");
    std::unique_ptr<const panda_file::File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    MethodLiteral *methodLiterals = pf->GetMethodLiterals();
    MethodLiteral& fooMethodLiteral = methodLiterals[0];

    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();
    JSHandle<Method> fooMethod = factory->NewMethod(&fooMethodLiteral);

    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("module");
    JSHandle<JSFunction> foo1Function = factory->NewJSFunction(globalEnv, fooMethod);
    foo1Function->SetLength(1);
    foo1Function->SetCallNapi(true);
    JSHandle<LexicalEnv> lexicalEnv = factory->NewLexicalEnv(1);
    foo1Function->SetLexicalEnv(thread, lexicalEnv);
    JSHandle<JSFunction> foo2Function = factory->NewJSFunction(globalEnv, fooMethod);
    foo2Function->SetLength(2);
    foo2Function->SetCallNapi(false);
    foo2Function->SetHomeObject(thread, module.GetTaggedValue());
    foo2Function->SetModule(thread, module.GetTaggedValue());

    JSFunction::ReplaceFunctionForHook(thread, foo1Function, foo2Function);
    // not replace when method is the same
    EXPECT_NE(foo1Function->GetLength(), foo2Function->GetLength());
    EXPECT_NE(foo1Function->GetBitField(), foo2Function->GetBitField());
    EXPECT_NE(foo1Function->GetLexicalEnv(thread), foo2Function->GetLexicalEnv(thread));
    EXPECT_NE(foo1Function->GetHomeObject(thread), foo2Function->GetHomeObject(thread));
    EXPECT_NE(foo1Function->GetModule(thread), foo2Function->GetModule(thread));
}

HWTEST_F_L0(JSFunctionTest, ReplaceJSFunctionForHookOldFuncMachineCodeIsNotUndefined)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
    )";
    const CString fileName = "test.pa";
    // create MethodLiteral
    pandasm::Parser parser;
    auto res = parser.Parse(source, "SRC.pa");
    std::unique_ptr<const panda_file::File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    MethodLiteral *methodLiterals = pf->GetMethodLiterals();
    MethodLiteral& foo1MethodLiteral = methodLiterals[0];
    MethodLiteral& foo2MethodLiteral = methodLiterals[1];

    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();
    JSHandle<Method> foo1Method = factory->NewMethod(&foo1MethodLiteral);
    JSHandle<Method> foo2Method = factory->NewMethod(&foo2MethodLiteral);

    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("module");
    JSHandle<JSFunction> foo1Function = factory->NewJSFunction(globalEnv, foo1Method);
    foo1Function->SetLength(1);
    foo1Function->SetCallNapi(true);
    JSHandle<LexicalEnv> lexicalEnv = factory->NewLexicalEnv(1);
    foo1Function->SetLexicalEnv(thread, lexicalEnv);
    MachineCodeDesc desc;
    JSHandle<JSTaggedValue> machineCode(thread, factory->NewMachineCodeObject(16, desc));
    foo1Function->SetMachineCode(thread, machineCode);
    JSHandle<JSFunction> foo2Function = factory->NewJSFunction(globalEnv, foo2Method);
    foo2Function->SetLength(2);
    foo2Function->SetCallNapi(false);
    foo2Function->SetHomeObject(thread, module.GetTaggedValue());
    foo2Function->SetModule(thread, module.GetTaggedValue());

    JSFunction::ReplaceFunctionForHook(thread, foo1Function, foo2Function);
    // not replace when method is the same
    EXPECT_NE(foo1Function->GetLength(), foo2Function->GetLength());
    EXPECT_NE(foo1Function->GetBitField(), foo2Function->GetBitField());
    EXPECT_NE(foo1Function->GetLexicalEnv(thread), foo2Function->GetLexicalEnv(thread));
    EXPECT_NE(foo1Function->GetHomeObject(thread), foo2Function->GetHomeObject(thread));
    EXPECT_NE(foo1Function->GetModule(thread), foo2Function->GetModule(thread));
}

HWTEST_F_L0(JSFunctionTest, ReplaceJSFunctionForHookNewFuncMachineCodeIsNotUndefined)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
    )";
    const CString fileName = "test.pa";
    // create MethodLiteral
    pandasm::Parser parser;
    auto res = parser.Parse(source, "SRC.pa");
    std::unique_ptr<const panda_file::File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    MethodLiteral *methodLiterals = pf->GetMethodLiterals();
    MethodLiteral& foo1MethodLiteral = methodLiterals[0];
    MethodLiteral& foo2MethodLiteral = methodLiterals[1];

    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();
    JSHandle<Method> foo1Method = factory->NewMethod(&foo1MethodLiteral);
    JSHandle<Method> foo2Method = factory->NewMethod(&foo2MethodLiteral);

    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("module");
    JSHandle<JSFunction> foo1Function = factory->NewJSFunction(globalEnv, foo1Method);
    foo1Function->SetLength(1);
    foo1Function->SetCallNapi(true);
    JSHandle<LexicalEnv> lexicalEnv = factory->NewLexicalEnv(1);
    foo1Function->SetLexicalEnv(thread, lexicalEnv);
    JSHandle<JSFunction> foo2Function = factory->NewJSFunction(globalEnv, foo2Method);
    foo2Function->SetLength(2);
    foo2Function->SetCallNapi(false);
    foo2Function->SetHomeObject(thread, module.GetTaggedValue());
    foo2Function->SetModule(thread, module.GetTaggedValue());
    MachineCodeDesc desc;
    JSHandle<JSTaggedValue> machineCode(thread, factory->NewMachineCodeObject(16, desc));
    foo2Function->SetMachineCode(thread, machineCode);

    JSFunction::ReplaceFunctionForHook(thread, foo1Function, foo2Function);
    // not replace when method is the same
    EXPECT_NE(foo1Function->GetLength(), foo2Function->GetLength());
    EXPECT_NE(foo1Function->GetBitField(), foo2Function->GetBitField());
    EXPECT_NE(foo1Function->GetLexicalEnv(thread), foo2Function->GetLexicalEnv(thread));
    EXPECT_NE(foo1Function->GetHomeObject(thread), foo2Function->GetHomeObject(thread));
    EXPECT_NE(foo1Function->GetModule(thread), foo2Function->GetModule(thread));
}

HWTEST_F_L0(JSFunctionTest, ReplaceJSApiFunctionForHook)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();

    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("module");
    JSHandle<JSFunction> foo1Function = factory->NewNormalJSApiFunction(globalEnv);
    foo1Function->SetLength(1);
    foo1Function->SetCallNapi(true);
    JSHandle<LexicalEnv> lexicalEnv = factory->NewLexicalEnv(1);
    foo1Function->SetLexicalEnv(thread, lexicalEnv);
    JSHandle<JSFunction> foo2Function = factory->NewNormalJSApiFunction(globalEnv);
    JSTaggedValue methodValue = thread->GlobalConstants()->GetGlobalConstantObject(
        static_cast<size_t>(ConstantIndex::NONE_FUNCTION_METHOD_INDEX));
    foo2Function->SetMethod(thread, methodValue);
    foo2Function->SetLength(2);
    foo2Function->SetCallNapi(false);
    foo2Function->SetHomeObject(thread, module.GetTaggedValue());

    JSFunction::ReplaceFunctionForHook(thread, foo1Function, foo2Function);

    EXPECT_EQ(foo1Function->GetMethod(thread), foo2Function->GetMethod(thread));
    EXPECT_EQ(foo1Function->GetCodeEntryOrNativePointer(), foo2Function->GetCodeEntryOrNativePointer());
    EXPECT_EQ(foo1Function->GetLength(), foo2Function->GetLength());
    EXPECT_EQ(foo1Function->GetBitField(), foo2Function->GetBitField());
    EXPECT_EQ(foo1Function->GetProtoOrHClass(thread), foo2Function->GetProtoOrHClass(thread));
    EXPECT_EQ(foo1Function->GetLexicalEnv(thread), foo2Function->GetLexicalEnv(thread));
    EXPECT_EQ(foo1Function->GetHomeObject(thread), foo2Function->GetHomeObject(thread));
}
}  // namespace panda::test
