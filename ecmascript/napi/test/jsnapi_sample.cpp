/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstddef>
#include <ctime>
#include <sys/time.h>

#include "gtest/gtest.h"

#include "assembler/assembly-parser.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::pandasm;
using namespace panda::panda_file;

namespace panda::test {
class JSNApiSampleTest : public testing::Test {
public:
    void SetUp() override
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        vm_ = JSNApi::CreateJSVM(option);
        thread_ = vm_->GetJSThread();
        vm_->SetEnableForceGC(true);
    }

    void TearDown() override
    {
        vm_->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(vm_);
    }

protected:
    JSThread *thread_ = nullptr;
    EcmaVM *vm_ = nullptr;
};


/* demo4 无参无返回值的函数的调用。 ts：
 * function FuncTest(): void {
 * console.log("func test log ...");
 * }
 * FuncTest();
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo4_function_test_1)
{
    GTEST_LOG_(INFO) << "sample_demo4_function_test_1 =======================================";
    LocalScope scope(vm_);

    Local<FunctionRef> FuncTest = FunctionRef::New(vm_, [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
        EcmaVM *vm = runtimeInfo->GetVM();
        LocalScope scope(vm);
        GTEST_LOG_(INFO) << "func test log ...";
        return JSValueRef::Undefined(vm);
    });
    FuncTest->Call(vm_, JSValueRef::Undefined(vm_), nullptr, 0);
    GTEST_LOG_(INFO) << "sample_demo4_function_test_1 ==========================================";
}

/* demo4 有参有返回值的函数的调用。 ts：
 * function Add(x: number, y: number): number {
 * return x + y;
 * }
 * let sum = Add(12, 34);
 * console.log(sum);
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo4_function_test_2)
{
    GTEST_LOG_(INFO) << "sample_demo4_function_test_2 =======================================";
    LocalScope scope(vm_);
    Local<FunctionRef> Add = FunctionRef::New(vm_, [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
        EcmaVM *vm = runtimeInfo->GetVM();
        LocalScope scope(vm);
        // 参数的个数。
        uint32_t argsCount = runtimeInfo->GetArgsNumber();
        // 遍历参数。
        for (uint32_t i = 0; i < argsCount; ++i) {
            Local<JSValueRef> arg = runtimeInfo->GetCallArgRef(i);
            GTEST_LOG_(INFO) << "func test arg " << i << "   " << arg->Int32Value(vm);
        }
        // 获取前两个参数。
        Local<JSValueRef> jsArg0 = runtimeInfo->GetCallArgRef(0);
        Local<JSValueRef> jsArg1 = runtimeInfo->GetCallArgRef(1);
        int arg0 = jsArg0->Int32Value(vm);
        int arg1 = jsArg1->Int32Value(vm);
        int sum = arg0 + arg1;
        GTEST_LOG_(INFO) << "func test sum " << sum;
        // 参数返回值
        return NumberRef::New(vm, sum);
    });
    int argv0 = 12; // random number
    int argv1 = 34; // random number
    Local<JSValueRef> *argv = new Local<JSValueRef>[2];
    argv[0] = NumberRef::New(vm_, argv0);
    argv[1] = NumberRef::New(vm_, argv1);
    Local<JSValueRef> jsSum = Add->Call(vm_, JSValueRef::Undefined(vm_), argv, 2);
    int sum = jsSum->Int32Value(vm_);
    GTEST_LOG_(INFO) << "func test call sum " << sum;
    GTEST_LOG_(INFO) << "sample_demo4_function_test_2 ==========================================";
}

Local<JSValueRef> AddFunc(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();
    LocalScope scope(vm);
    // 参数的个数。
    uint32_t argsCount = runtimeInfo->GetArgsNumber();
    // 遍历参数。
    for (uint32_t i = 0; i < argsCount; ++i) {
        Local<JSValueRef> arg = runtimeInfo->GetCallArgRef(i);
        GTEST_LOG_(INFO) << "func test arg " << i << "   " << arg->Int32Value(vm);
    }
    // 获取前两个参数。
    Local<JSValueRef> jsArg0 = runtimeInfo->GetCallArgRef(0);
    Local<JSValueRef> jsArg1 = runtimeInfo->GetCallArgRef(1);
    int arg0 = jsArg0->Int32Value(vm);
    int arg1 = jsArg1->Int32Value(vm);
    int sum = arg0 + arg1;
    // 参数返回值
    return NumberRef::New(vm, sum);
}

Local<JSValueRef> AddProxyFunc(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();
    LocalScope scope(vm);
    // 参数的个数。
    uint32_t argsCount = runtimeInfo->GetArgsNumber();
    // 函数调用的时候的传参个数，如果不能等于这个值说明函数调用有问题。
    uint32_t defaultArgsCount = 3;
    if (argsCount != defaultArgsCount) {
        return NumberRef::New(vm, 0);
    }
    // 函数
    int index = 0; // 获取参数的索引。
    Local<JSValueRef> jsArg0 = runtimeInfo->GetCallArgRef(index);
    // 获取前两个参数。
    index++;
    Local<JSValueRef> jsArg1 = runtimeInfo->GetCallArgRef(index);
    index++;
    Local<JSValueRef> jsArg2 = runtimeInfo->GetCallArgRef(index);
    Local<FunctionRef> addFunc = jsArg0;
    const int addFuncArgCount = 2; // 内部调用的函数的参数个数。
    Local<JSValueRef> argv[addFuncArgCount] = { jsArg1, jsArg2 };
    Local<JSValueRef> jsSum = addFunc->Call(vm, JSValueRef::Undefined(vm), argv, addFuncArgCount);
    int sum = jsSum->Int32Value(vm);
    int multiple = 2; // 代理的功能，原函数的2倍返回。
    sum *= multiple;
    // 参数返回值
    return NumberRef::New(vm, sum);
}

/* demo4 参数是函数的调用。 ts：
 * function Add(x: number, y: number): number {
 * return x + y;
 * }
 * function AddProxy(add: (x: number, y: number) => number, x: number, y: number) {
 * let sum = add(x, y);
 * return sum * 2
 * }
 * let sum1 = Add(12, 34);
 * let sum2 = AddProxy(Add,12, 34);
 * console.log(sum1);
 * console.log(sum2);
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo4_function_test_3)
{
    GTEST_LOG_(INFO) << "sample_demo4_function_test_3 =======================================";
    LocalScope scope(vm_);
    Local<FunctionRef> Add = FunctionRef::New(vm_, AddFunc);
    Local<FunctionRef> AddProxy = FunctionRef::New(vm_, AddProxyFunc);
    int num1 = 12; // random number
    int num2 = 34; // random number
    Local<JSValueRef> addArgv[2];
    addArgv[0] = NumberRef::New(vm_, num1);
    addArgv[1] = NumberRef::New(vm_, num2);
    Local<JSValueRef> addProxyArgv[3];
    addProxyArgv[0] = Add;
    addProxyArgv[1] = NumberRef::New(vm_, num1);
    addProxyArgv[2] = NumberRef::New(vm_, num2);
    Local<JSValueRef> jsSum1 = Add->Call(vm_, JSValueRef::Undefined(vm_), addArgv, 2);
    Local<JSValueRef> jsSum2 = AddProxy->Call(vm_, JSValueRef::Undefined(vm_), addProxyArgv, 3);
    int sum1 = jsSum1->Int32Value(vm_);
    int sum2 = jsSum2->Int32Value(vm_);
    GTEST_LOG_(INFO) << "func test call Add , sum = " << sum1;
    GTEST_LOG_(INFO) << "func test call AddProxy , sum = " << sum2;
    GTEST_LOG_(INFO) << "sample_demo4_function_test_3 ==========================================";
}

/* demo5 类的静态函数和非静态函数的测试，静态变量变量 和 非静态变量的测试。 ts:
 * class Greeter {
 * // 静态变量。
 * static position:string = "door";
 * // 私有静态变量。
 * private static standardGreetingStr:string = "Hello, there";
 *
 * // 私有非静态变量。
 * private privateGreeting: string;
 * // 非静态变量。
 * greeting: string;
 *
 * // 构造函数。
 * constructor(greet: string) {
 * this.greeting = greet;
 * }
 *
 * // 非静态函数。
 * SetPrivateGreeting(priGreeting: string):void
 * {
 * this.privateGreeting = priGreeting;
 * }
 *
 * // 非静态函数调用。
 * Greet(): string {
 * if (this.privateGreeting) {
 * return "Hello, " + this.privateGreeting;
 * }else if (this.greeting) {
 * return "Hello, " + this.greeting;
 * }
 * else {
 * return Greeter.standardGreetingStr;
 * }
 * }
 *
 * // 静态函数调用。
 * static StandardGreeting(): string {
 * return Greeter.standardGreetingStr;
 * }
 *
 * // 静态函数调用。
 * static StandardPosition(): string {
 * return Greeter.position;
 * }
 *
 * }
 *
 * let greeter1: Greeter = new Greeter("everyone");
 *
 * // 非静态函数调用
 * console.log(greeter1.Greet());
 *
 * greeter1.SetPrivateGreeting("vip");
 * console.log(greeter1.Greet());
 *
 * greeter1.SetPrivateGreeting("");
 * console.log(greeter1.Greet());
 *
 * // 修改变量
 * greeter1.greeting = "";
 * console.log(greeter1.Greet());
 *
 * // 静态函数调用。
 * console.log(Greeter.StandardGreeting());
 * console.log(Greeter.StandardPosition());
 *
 * // 修改静态变量。
 * Greeter.position = "house";
 * console.log(Greeter.StandardPosition());
 */
class Tag {
public:
    explicit Tag(const std::string name) : name_(name)
    {
        GTEST_LOG_(INFO) << "tag construction , name is " << name_;
    }
    ~Tag()
    {
        GTEST_LOG_(INFO) << "tag deconstruction , name is " << name_;
    }

private:
    std::string name_;
};

class Greeter {
private:
    // 新建ClassFunction
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm);

    // 添加静态变量。
    static void AddStaticVariable(EcmaVM *vm, Local<FunctionRef> &claFunc);
    // 添加静态函数。
    static void AddStaticFunction(EcmaVM *vm, Local<FunctionRef> &claFunc);

    // 添加非静态变量。
    static void AddVariable(EcmaVM *vm, Local<ObjectRef> &proto);
    // 添加非静态函数。
    static void AddFunction(EcmaVM *vm, Local<ObjectRef> &proto);

public:
    // 获取ClassFunction
    static Local<FunctionRef> GetClassFunction(EcmaVM *vm);
    static Local<Greeter> New(EcmaVM *vm, Local<StringRef> greet);

    // 非静态函数调用。
    static void SetPrivateGreeting(EcmaVM *vm, Local<Greeter> thisRef, Local<StringRef> priGreeting);
    static Local<StringRef> Greet(EcmaVM *vm, Local<Greeter> thisRef);

    // 静态函数的调用。
    static Local<StringRef> StandardGreeting(EcmaVM *vm);
    static Local<StringRef> StandardPosition(EcmaVM *vm);

private:
    static Local<SymbolRef> standardGreetingStrKey;
    static Local<SymbolRef> privateGreetingKey;

    // 类名
    const static std::string CLASS_NAME;
};

Local<SymbolRef> Greeter::standardGreetingStrKey;
Local<SymbolRef> Greeter::privateGreetingKey;

const std::string Greeter::CLASS_NAME = "GreeterClass";

Local<FunctionRef> Greeter::NewClassFunction(EcmaVM *vm)
{
    // 初始化私有静态变量的key。
    Greeter::standardGreetingStrKey = SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "standardGreetingStr"));
    Greeter::privateGreetingKey = SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "privateGreeting"));

    Tag *tag = new Tag("ClassFunctionTag");
    Local<FunctionRef> claFunc = FunctionRef::NewClassFunction(
        vm,
        // 构造函数调用
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
            // 获取参数。
            Local<JSValueRef> greet = runtimeInfo->GetCallArgRef(0);
            // ts: this.greeting = greet;
            thisRef->Set(vm, StringRef::NewFromUtf8(vm, "greeting"), greet);
            return thisRef;
        },
        [](void *nativePointer, void *data) {
            GTEST_LOG_(INFO) << "NewClassFunction, nativePointer is " << nativePointer;
            Tag *t = static_cast<Tag *>(data);
            delete t;
        },
        tag);
    // static 添加 到  claFunc。
    // 添加静态变量。
    AddStaticVariable(vm, claFunc);
    // 添加静态函数
    AddStaticFunction(vm, claFunc);
    Local<JSValueRef> jsProto = claFunc->GetFunctionPrototype(vm);
    Local<ObjectRef> proto = jsProto->ToObject(vm);
    // 非static 添加到 proto。
    // 添加非静态变量
    AddVariable(vm, proto);
    // 添加非静态函数
    AddFunction(vm, proto);
    // 设置类名。
    claFunc->SetName(vm, StringRef::NewFromUtf8(vm, Greeter::CLASS_NAME.c_str()));
    return claFunc;
}

Local<FunctionRef> Greeter::GetClassFunction(EcmaVM *vm)
{
    Local<ObjectRef> globalObj = JSNApi::GetGlobalObject(vm);
    Local<JSValueRef> jsClaFunc = globalObj->Get(vm, StringRef::NewFromUtf8(vm, Greeter::CLASS_NAME.c_str()));
    if (jsClaFunc->IsFunction()) {
        return jsClaFunc;
    }
    Local<FunctionRef> claFunc = Greeter::NewClassFunction(vm);
    // 添加到全局。
    globalObj->Set(vm, claFunc->GetName(vm), claFunc);
    return claFunc;
}

// 添加静态变量。
void Greeter::AddStaticVariable(EcmaVM *vm, Local<FunctionRef> &claFunc)
{
    // 静态变量。
    // static position:string = "door";
    claFunc->Set(vm, StringRef::NewFromUtf8(vm, "position"), StringRef::NewFromUtf8(vm, "door"));
    // 私有静态变量。
    // private static standardGreetingStr:string = "Hello, there";
    claFunc->Set(vm, Greeter::standardGreetingStrKey, StringRef::NewFromUtf8(vm, "Hello, there"));
}

// 添加静态函数。
void Greeter::AddStaticFunction(EcmaVM *vm, Local<FunctionRef> &claFunc)
{
    // 静态函数调用。
    claFunc->Set(vm, StringRef::NewFromUtf8(vm, "StandardGreeting"),
        FunctionRef::New(vm,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            Local<FunctionRef> claFunc = Greeter::GetClassFunction(vm);
            Local<JSValueRef> jsStandardGreetingStr = claFunc->Get(vm, Greeter::standardGreetingStrKey);
            return jsStandardGreetingStr;
        }));
    // 静态函数调用。
    claFunc->Set(vm, StringRef::NewFromUtf8(vm, "StandardPosition"),
        FunctionRef::New(vm,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            Local<FunctionRef> claFunc = Greeter::GetClassFunction(vm);
            Local<JSValueRef> jsPosition = claFunc->Get(vm, StringRef::NewFromUtf8(vm, "position"));
            return jsPosition;
        }));
}

// 添加非静态变量。
void Greeter::AddVariable(EcmaVM *vm, Local<ObjectRef> &proto)
{
    // 私有非静态变量。
    proto->Set(vm, Greeter::privateGreetingKey, JSValueRef::Undefined(vm));
    // 非静态变量。
    proto->Set(vm, StringRef::NewFromUtf8(vm, "greeting"), JSValueRef::Undefined(vm));
}

// 添加非静态函数。
void Greeter::AddFunction(EcmaVM *vm, Local<ObjectRef> &proto)
{
    // 非静态函数。
    proto->Set(vm, StringRef::NewFromUtf8(vm, "SetPrivateGreeting"),
        FunctionRef::New(vm,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
            // 获取参数。
            Local<JSValueRef> priGreeting = runtimeInfo->GetCallArgRef(0);
            thisRef->Set(vm, Greeter::privateGreetingKey, priGreeting);
            return JSValueRef::Undefined(vm);
        }));
    // 非静态函数。
    proto->Set(vm, StringRef::NewFromUtf8(vm, "Greet"),
        FunctionRef::New(vm,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // 获取类
            Local<FunctionRef> claFunc = Greeter::GetClassFunction(vm);
            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
            Local<JSValueRef> jsPrivateGreeting = thisRef->Get(vm, Greeter::privateGreetingKey);
            Local<JSValueRef> jsGreeting = thisRef->Get(vm, StringRef::NewFromUtf8(vm, "greeting"));
            Local<JSValueRef> jsStandardGreetingStr = claFunc->Get(vm, Greeter::standardGreetingStrKey);
            std::string ret;
            if (jsPrivateGreeting->IsString()) {
                ret.append("Hello, ").append(jsPrivateGreeting->ToString(vm)->ToString());
            } else if (jsGreeting->IsString()) {
                ret.append("Hello, ").append(jsGreeting->ToString(vm)->ToString());
            } else {
                ret.append(jsStandardGreetingStr->ToString(vm)->ToString());
            }
            return StringRef::NewFromUtf8(vm, ret.c_str(), ret.size());
        }));
}

Local<Greeter> Greeter::New(EcmaVM *vm, Local<StringRef> greet)
{
    // 获取类函数。
    Local<FunctionRef> claFunc = Greeter::GetClassFunction(vm);
    // 定义参数。
    Local<JSValueRef> argv[1] = {greet};
    // 构造函数，构造对象。
    Local<JSValueRef> jsObj = claFunc->Constructor(vm, argv, 1);
    Local<ObjectRef> obj = jsObj->ToObject(vm);
    return obj;
}

/* // 非静态函数调用。 ts:
 * SetPrivateGreeting(priGreeting: string):void
 */
void Greeter::SetPrivateGreeting(EcmaVM *vm, Local<Greeter> thisRef, Local<StringRef> priGreeting)
{
    Local<ObjectRef> objRef = thisRef;
    Local<FunctionRef> func = objRef->Get(vm, StringRef::NewFromUtf8(vm, "SetPrivateGreeting"));
    Local<JSValueRef> argv [1] = {priGreeting};
    func->Call(vm, objRef, argv, 1);
}

/* // 非静态函数调用。 ts:
 * Greet(): string
 */
Local<StringRef> Greeter::Greet(EcmaVM *vm, Local<Greeter> thisRef)
{
    Local<ObjectRef> objRef = thisRef;
    Local<FunctionRef> func = objRef->Get(vm, StringRef::NewFromUtf8(vm, "Greet"));
    return func->Call(vm, objRef, nullptr, 0);
}

/* // 静态函数的调用。 ts:
 * static StandardGreeting(): string
 */
Local<StringRef> Greeter::StandardGreeting(EcmaVM *vm)
{
    // 获取类函数。
    Local<FunctionRef> claFunc = Greeter::GetClassFunction(vm);
    // 获取函数
    Local<FunctionRef> func = claFunc->Get(vm, StringRef::NewFromUtf8(vm, "StandardGreeting"));
    // 调用函数
    return func->Call(vm, JSValueRef::Undefined(vm), nullptr, 0);
}

/* // 静态函数的调用。ts:
 * static StandardPosition(): string
 */
Local<StringRef> Greeter::StandardPosition(EcmaVM *vm)
{
    // 获取类函数。
    Local<FunctionRef> claFunc = Greeter::GetClassFunction(vm);
    // 获取函数
    Local<FunctionRef> func = claFunc->Get(vm, StringRef::NewFromUtf8(vm, "StandardPosition"));
    // 调用函数
    return func->Call(vm, JSValueRef::Undefined(vm), nullptr, 0);
}

/* 类的调用测试： ts:
 * let greeter1: Greeter = new Greeter("everyone");
 *
 * // 非静态函数调用
 * console.log(greeter1.Greet());
 *
 * greeter1.SetPrivateGreeting("vip");
 * console.log(greeter1.Greet());
 *
 * greeter1.SetPrivateGreeting("");
 * console.log(greeter1.Greet());
 *
 * // 修改变量
 * greeter1.greeting = "";
 * console.log(greeter1.Greet());
 *
 * // 静态函数调用。
 * console.log(Greeter.StandardGreeting());
 * console.log(Greeter.StandardPosition());
 *
 * // 修改静态变量。
 * Greeter.position = "house";
 * console.log(Greeter.StandardPosition());
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo5_class_test)
{
    GTEST_LOG_(INFO) << "sample_demo5_class_test =======================================";
    LocalScope scope(vm_);
    Local<Greeter> greeter1 = Greeter::New(vm_, StringRef::NewFromUtf8(vm_, "everyone"));
    // 非静态函数调用
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString();
    Greeter::SetPrivateGreeting(vm_, greeter1, StringRef::NewFromUtf8(vm_, "vip"));
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString();
    Greeter::SetPrivateGreeting(vm_, greeter1, JSValueRef::Undefined(vm_));
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString();
    // 修改变量
    Local<ObjectRef> objRef1 = greeter1;
    objRef1->Set(vm_, StringRef::NewFromUtf8(vm_, "greeting"), JSValueRef::Undefined(vm_));
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString();

    // 静态函数调用。
    GTEST_LOG_(INFO) << Greeter::StandardGreeting(vm_)->ToString();
    GTEST_LOG_(INFO) << Greeter::StandardPosition(vm_)->ToString();

    // 修改静态变量。
    Local<FunctionRef> classFunc = Greeter::GetClassFunction(vm_);
    classFunc->Set(vm_, StringRef::NewFromUtf8(vm_, "position"), StringRef::NewFromUtf8(vm_, "house"));

    GTEST_LOG_(INFO) << Greeter::StandardPosition(vm_)->ToString();
    GTEST_LOG_(INFO) << "sample_demo5_class_test =======================================";
}

// JSValueRef转为字符串输出。
std::string jsValue2String(EcmaVM *vm, Local<JSValueRef> &jsVal)
{
    if (jsVal->IsString()) {
        return "type string, val : " + jsVal->ToString(vm)->ToString();
    } else if (jsVal->IsNumber()) {
        return "type number, val : " + std::to_string(jsVal->Int32Value(vm));
    } else if (jsVal->IsBoolean()) {
        return "type bool, val : " + std::to_string(jsVal->BooleaValue());
    } else if (jsVal->IsSymbol()) {
        Local<SymbolRef> symbol = jsVal;
        return "type symbol, val : " + symbol->GetDescription(vm)->ToString();
    } else {
        return "type other : " + jsVal->ToString(vm)->ToString();
    }
}

void MapSetValue(EcmaVM *vm, Local<MapRef> &map, Local<JSValueRef> symbolKey)
{
    map->Set(vm, StringRef::NewFromUtf8(vm, "key1"), StringRef::NewFromUtf8(vm, "val1"));
    int num2 = 222; // random number
    map->Set(vm, StringRef::NewFromUtf8(vm, "key2"), NumberRef::New(vm, num2));
    map->Set(vm, StringRef::NewFromUtf8(vm, "key3"), BooleanRef::New(vm, true));
    map->Set(vm, StringRef::NewFromUtf8(vm, "key4"), SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "val4")));
    int num5 = 55; // random number
    map->Set(vm, IntegerRef::New(vm, num5), StringRef::NewFromUtf8(vm, "val5"));
    int num61 = 66;  // random number
    int num62 = 666; // random number
    map->Set(vm, IntegerRef::New(vm, num61), IntegerRef::New(vm, num62));
    map->Set(vm, BooleanRef::New(vm, true), StringRef::NewFromUtf8(vm, "val7"));
    map->Set(vm, symbolKey, StringRef::NewFromUtf8(vm, "val8"));
}

void MapGetValue(EcmaVM *vm, Local<MapRef> &map, Local<JSValueRef> symbolKey)
{
    Local<JSValueRef> val1 = map->Get(vm, StringRef::NewFromUtf8(vm, "key1"));
    bool val1IsString = val1->IsString();
    GTEST_LOG_(INFO) << "key1 : IsString:" << val1IsString << "    val:" << val1->ToString(vm)->ToString();

    Local<JSValueRef> val2 = map->Get(vm, StringRef::NewFromUtf8(vm, "key2"));
    bool val2IsNumber = val2->IsNumber();
    GTEST_LOG_(INFO) << "key2 : IsNumber:" << val2IsNumber << "    val:" << val2->Int32Value(vm);

    Local<JSValueRef> val3 = map->Get(vm, StringRef::NewFromUtf8(vm, "key3"));
    bool val3IsBoolean = val3->IsBoolean();
    GTEST_LOG_(INFO) << "key3 : IsBoolean:" << val3IsBoolean << "    val:" << val3->BooleaValue();

    Local<JSValueRef> val4 = map->Get(vm, StringRef::NewFromUtf8(vm, "key4"));
    bool val4IsSymbol = val4->IsSymbol();
    Local<SymbolRef> val4Symbol = val4;
    GTEST_LOG_(INFO) << "key4 : IsSymbol:" << val4IsSymbol << "    val:" << val4Symbol->GetDescription(vm)->ToString();

    int num5 = 55; // random number
    Local<JSValueRef> val5 = map->Get(vm, IntegerRef::New(vm, num5));
    GTEST_LOG_(INFO) << "55 : " << val5->ToString(vm)->ToString();

    int num6 = 66; // random number
    Local<JSValueRef> val6 = map->Get(vm, IntegerRef::New(vm, num6));
    GTEST_LOG_(INFO) << "66 : " << val6->Int32Value(vm);
    Local<JSValueRef> val7 = map->Get(vm, BooleanRef::New(vm, true));
    GTEST_LOG_(INFO) << "true : " << val7->ToString(vm)->ToString();

    Local<JSValueRef> val8 = map->Get(vm, symbolKey);
    GTEST_LOG_(INFO) << "SymbolRef(key8)  : " << val8->ToString(vm)->ToString();

    Local<JSValueRef> val82 = map->Get(vm, SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "key8")));
    GTEST_LOG_(INFO) << "new SymbolRef(key8) is Undefined : " << val82->IsUndefined();

    int32_t size = map->GetSize();
    GTEST_LOG_(INFO) << "size : " << size;
    int32_t totalElement = map->GetTotalElements();
    GTEST_LOG_(INFO) << "total element : " << totalElement;

    for (int i = 0; i < size; ++i) {
        Local<JSValueRef> jsKey = map->GetKey(vm, i);
        Local<JSValueRef> jsVal = map->GetValue(vm, i);
        GTEST_LOG_(INFO) << "for map index : " << i << "    key : " << jsValue2String(vm, jsKey) << "    val : " <<
            jsValue2String(vm, jsVal);
    }
}

void MapIteratorGetValue(EcmaVM *vm, Local<MapRef> &map)
{
    Local<MapIteratorRef> mapIter = MapIteratorRef::New(vm, map);
    ecmascript::EcmaRuntimeCallInfo *mapIterInfo = mapIter->GetEcmaRuntimeCallInfo(vm);

    Local<StringRef> kind = mapIter->GetKind(vm);
    GTEST_LOG_(INFO) << "Map Iterator kind : " << kind->ToString();

    for (Local<ArrayRef> array = MapIteratorRef::Next(vm, mapIterInfo); array->IsArray(vm);
        array = MapIteratorRef::Next(vm, mapIterInfo)) {
        int index = mapIter->GetIndex() - 1;
        Local<JSValueRef> jsKey = ArrayRef::GetValueAt(vm, array, 0);
        Local<JSValueRef> jsVal = ArrayRef::GetValueAt(vm, array, 1);
        GTEST_LOG_(INFO) << "for map iterator index : " << index << "    key : " << jsValue2String(vm, jsKey) <<
            "    val : " << jsValue2String(vm, jsVal);
    }
}

/* demo9 Map,MapIterator 的操作。 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo9_map_test_1_MapRef_MapIteratorRef)
{
    GTEST_LOG_(INFO) << "sample_demo9_map_test_1_MapRef_MapIteratorRef =======================================";
    LocalScope scope(vm_);

    Local<MapRef> map = MapRef::New(vm_);
    Local<JSValueRef> symbolKey = SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "key8"));
    MapSetValue(vm_, map, symbolKey);
    MapGetValue(vm_, map, symbolKey);
    MapIteratorGetValue(vm_, map);
    GTEST_LOG_(INFO) << "sample_demo9_map_test_1_MapRef_MapIteratorRef =======================================";
}

/* demo9 WeakMap 的操作。 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo9_map_test_2_WeakMapref)
{
    GTEST_LOG_(INFO) << "sample_demo9_map_test_2_WeakMapref =======================================";
    LocalScope scope(vm_);

    Local<WeakMapRef> weakMap = WeakMapRef::New(vm_);
    weakMap->Set(vm_, StringRef::NewFromUtf8(vm_, "key1"), StringRef::NewFromUtf8(vm_, "val1"));
    int num2 = 222; // random number
    weakMap->Set(vm_, StringRef::NewFromUtf8(vm_, "key2"), NumberRef::New(vm_, num2));
    weakMap->Set(vm_, StringRef::NewFromUtf8(vm_, "key3"), BooleanRef::New(vm_, true));
    weakMap->Set(vm_, StringRef::NewFromUtf8(vm_, "key4"), SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "val4")));
    weakMap->Set(vm_, IntegerRef::New(vm_, 55), StringRef::NewFromUtf8(vm_, "val5"));
    int num6 = 666; // random number
    weakMap->Set(vm_, IntegerRef::New(vm_, 66), IntegerRef::New(vm_, num6));
    weakMap->Set(vm_, BooleanRef::New(vm_, true), StringRef::NewFromUtf8(vm_, "val7"));
    Local<JSValueRef> key8 = SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "key8"));
    weakMap->Set(vm_, key8, StringRef::NewFromUtf8(vm_, "val8"));

    int size = weakMap->GetSize();
    GTEST_LOG_(INFO) << "size : " << size;
    int32_t totalElements = weakMap->GetTotalElements();
    GTEST_LOG_(INFO) << "total elements : " << totalElements;

    for (int i = 0; i < size; ++i) {
        Local<JSValueRef> jsKey = weakMap->GetKey(vm_, i);
        Local<JSValueRef> jsVal = weakMap->GetValue(vm_, i);
        GTEST_LOG_(INFO) << "for WeakMap index : " << i << "    key : " << jsValue2String(vm_, jsKey) << "    val : " <<
            jsValue2String(vm_, jsVal);
    }

    bool hasKey2 = weakMap->Has(StringRef::NewFromUtf8(vm_, "key2"));
    GTEST_LOG_(INFO) << "Has Key2 : " << hasKey2;
    bool hasKey222 = weakMap->Has(StringRef::NewFromUtf8(vm_, "key222"));
    GTEST_LOG_(INFO) << "Has key222 : " << hasKey222;

    GTEST_LOG_(INFO) << "sample_demo9_map_test_2_WeakMapref =======================================";
}


// json对象获取值。
void JsonObjGetValue(EcmaVM *vm, Local<ObjectRef> obj)
{
    Local<JSValueRef> jsVal1 = obj->Get(vm, StringRef::NewFromUtf8(vm, "str1"));
    bool jsVal1IsString = jsVal1->IsString();
    Local<StringRef> val1 = jsVal1->ToString(vm);
    GTEST_LOG_(INFO) << "str1 : is string : " << jsVal1IsString << "  value : " << val1->ToString();
    Local<JSValueRef> jsVal2 = obj->Get(vm, StringRef::NewFromUtf8(vm, "str2"));
    bool jsVal2IsNumber = jsVal2->IsNumber();
    int val2 = jsVal2->Int32Value(vm);
    GTEST_LOG_(INFO) << "str2 : is number : " << jsVal2IsNumber << "  value : " << val2;
    Local<JSValueRef> jsVal3 = obj->Get(vm, StringRef::NewFromUtf8(vm, "333"));
    int val3 = jsVal3->Int32Value(vm);
    GTEST_LOG_(INFO) << "333 : " << val3;
    Local<JSValueRef> jsVal4 = obj->Get(vm, StringRef::NewFromUtf8(vm, "444"));
    Local<StringRef> val4 = jsVal4->ToString(vm);
    GTEST_LOG_(INFO) << "str4 : " << val4->ToString();

    Local<JSValueRef> jsVal8 = obj->Get(vm, StringRef::NewFromUtf8(vm, "b8"));
    bool jsVal8IsBool = jsVal8->IsBoolean();
    Local<BooleanRef> val8Ref = jsVal8;
    bool val8 = val8Ref->Value();
    GTEST_LOG_(INFO) << "b8  is bool : " << jsVal8IsBool << "    val : " << val8;
}

// json对象获取数组。
void JsonObjGetArray(EcmaVM *vm, Local<ObjectRef> obj)
{
    Local<JSValueRef> jsVal5 = obj->Get(vm, StringRef::NewFromUtf8(vm, "arr5"));
    Local<ObjectRef> val5Obj = jsVal5;
    int length = val5Obj->Get(vm, StringRef::NewFromUtf8(vm, "length"))->Int32Value(vm);
    GTEST_LOG_(INFO) << "arr5 length : " << length;
    for (int i = 0; i < length; ++i) {
        Local<JSValueRef> val5Item = val5Obj->Get(vm, NumberRef::New(vm, i));
        GTEST_LOG_(INFO) << "arr5 : " << i << "    " << val5Item->Int32Value(vm);
    }
    Local<ArrayRef> val5Arr = jsVal5;
    uint32_t length2 = val5Arr->Length(vm);
    GTEST_LOG_(INFO) << "arr5 length2 : " << length2;
    for (uint32_t i = 0; i < length2; ++i) {
        Local<JSValueRef> val5Item = ArrayRef::GetValueAt(vm, val5Arr, i);
        GTEST_LOG_(INFO) << "arr5 : " << i << "    " << val5Item->Int32Value(vm);
    }
    Local<JSValueRef> jsVal6 = obj->Get(vm, StringRef::NewFromUtf8(vm, "arr6"));
    Local<ObjectRef> val6Obj = jsVal6;
    int length3 = val6Obj->Get(vm, StringRef::NewFromUtf8(vm, "length"))->Int32Value(vm);
    GTEST_LOG_(INFO) << "arr6 length : " << length3;
    for (int i = 0; i < length3; ++i) {
        Local<JSValueRef> val6Item = val6Obj->Get(vm, NumberRef::New(vm, i));
        GTEST_LOG_(INFO) << "arr6 : " << i << "    " << val6Item->ToString(vm)->ToString();
    }
}

// json对象获取对象。
void JsonObjGetObject(EcmaVM *vm, Local<ObjectRef> obj)
{
    Local<JSValueRef> jsVal7 = obj->Get(vm, StringRef::NewFromUtf8(vm, "data7"));
    Local<ObjectRef> val7Obj = jsVal7->ToObject(vm);
    Local<ArrayRef> val7ObjKeys = val7Obj->GetOwnPropertyNames(vm);
    for (uint32_t i = 0; i < val7ObjKeys->Length(vm); ++i) {
        Local<JSValueRef> itemKey = ArrayRef::GetValueAt(vm, val7ObjKeys, i);
        Local<JSValueRef> itemVal = val7Obj->Get(vm, itemKey);
        GTEST_LOG_(INFO) << "data7 :   item index:" << i << "    Key:" << itemKey->ToString(vm)->ToString() <<
            "    val:" << itemVal->ToString(vm)->ToString();
    }
}

/* demo11 json 测试，json字符串 转 obj。 注意key不能是纯数字。 json:
 * {
 * "str1": "val1",
 * "str2": 222,
 * "333": 456,
 * "444": "val4",
 * "arr5": [
 * 51,
 * 52,
 * 53,
 * 54,
 * 55
 * ],
 * "arr6": [
 * "str61",
 * "str62",
 * "str63",
 * "str64"
 * ],
 * "data7": {
 * "key71": "val71",
 * "key72": "val72"
 * },
 * "b8": true
 * }
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_1_parse_object)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_1_parse_object =======================================";
    LocalScope scope(vm_);
    std::string jsonStr =
        "{\"str1\":\"val1\",\"str2\":222,\"333\":456,\"444\":\"val4\",\"arr5\":[51,52,53,54,55],\"arr6\":[\"str61\","
        "\"str62\",\"str63\",\"str64\"],\"data7\":{\"key71\":\"val71\",\"key72\":\"val72\"},\"b8\":true}";
    Local<JSValueRef> jsObj = JSON::Parse(vm_, StringRef::NewFromUtf8(vm_, jsonStr.c_str()));
    Local<ObjectRef> obj = jsObj->ToObject(vm_);

    JsonObjGetValue(vm_, obj);
    JsonObjGetArray(vm_, obj);
    JsonObjGetObject(vm_, obj);

    GTEST_LOG_(INFO) << "sample_demo11_json_test_1_parse_object =======================================";
}

/* demo11 json 测试，obj转json字符串。 json:
 * {
 * "key1": "val1",
 * "key2": 123,
 * "333": "val3",
 * "arr4": [
 * "val40",
 * "val41",
 * "val42",
 * "val43"
 * ],
 * "arr5": [
 * 50,
 * 51,
 * 52,
 * 53
 * ],
 * "b6": true,
 * "obj7": {
 * "key71": "val71",
 * "key72": "val72",
 * "key73": "val73"
 * }
 * }
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_2_stringify_object)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_2_stringify_object =======================================";
    LocalScope scope(vm_);

    Local<ObjectRef> obj = ObjectRef::New(vm_);
    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "key1"), StringRef::NewFromUtf8(vm_, "val1"));
    int num2 = 123; // random number
    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "key2"), NumberRef::New(vm_, num2));
    int num3 = 333; // random number
    obj->Set(vm_, NumberRef::New(vm_, num3), StringRef::NewFromUtf8(vm_, "val3"));

    Local<ArrayRef> arr4 = ArrayRef::New(vm_);
    ArrayRef::SetValueAt(vm_, arr4, 0, StringRef::NewFromUtf8(vm_, "val40"));
    ArrayRef::SetValueAt(vm_, arr4, 1, StringRef::NewFromUtf8(vm_, "val41"));
    ArrayRef::SetValueAt(vm_, arr4, 2, StringRef::NewFromUtf8(vm_, "val42"));
    ArrayRef::SetValueAt(vm_, arr4, 3, StringRef::NewFromUtf8(vm_, "val43"));
    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "arr4"), arr4);

    Local<ArrayRef> arr5 = ArrayRef::New(vm_);
    int num50 = 50; // random number
    int num51 = 51; // random number
    int num52 = 52; // random number
    int num53 = 53; // random number
    ArrayRef::SetValueAt(vm_, arr5, 0, IntegerRef::New(vm_, num50));
    ArrayRef::SetValueAt(vm_, arr5, 1, IntegerRef::New(vm_, num51));
    ArrayRef::SetValueAt(vm_, arr5, 2, IntegerRef::New(vm_, num52));
    ArrayRef::SetValueAt(vm_, arr5, 3, IntegerRef::New(vm_, num53));
    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "arr5"), arr5);

    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "b6"), BooleanRef::New(vm_, true));

    Local<ObjectRef> obj7 = ObjectRef::New(vm_);
    obj7->Set(vm_, StringRef::NewFromUtf8(vm_, "key71"), StringRef::NewFromUtf8(vm_, "val71"));
    obj7->Set(vm_, StringRef::NewFromUtf8(vm_, "key72"), StringRef::NewFromUtf8(vm_, "val72"));
    obj7->Set(vm_, StringRef::NewFromUtf8(vm_, "key73"), StringRef::NewFromUtf8(vm_, "val73"));

    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "obj7"), obj7);

    Local<JSValueRef> jsStr = JSON::Stringify(vm_, obj);
    GTEST_LOG_(INFO) << "jsStr is String : " << jsStr->IsString();
    Local<StringRef> strRef = jsStr->ToString(vm_);
    std::string str = strRef->ToString();
    GTEST_LOG_(INFO) << "json : " << str;

    GTEST_LOG_(INFO) << "sample_demo11_json_test_2_stringify_object =======================================";
}

/* demo11 json 测试，json字符串转为数组。 json:
 * [51,52,53,54,55]
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_3_parse_array1)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array1 =======================================";
    LocalScope scope(vm_);

    // 随机的一个 json 数字 数组。  [51,52,53,54,55]
    Local<StringRef> arrStr1 = StringRef::NewFromUtf8(vm_, "[51,52,53,54,55]");
    Local<JSValueRef> jsArr1 = JSON::Parse(vm_, arrStr1);
    bool jsArr1IsArr = jsArr1->IsArray(vm_);
    GTEST_LOG_(INFO) << "jsArr1 is array : " << jsArr1IsArr;
    Local<ArrayRef> arr1 = jsArr1;
    uint32_t arr1Length = arr1->Length(vm_);
    GTEST_LOG_(INFO) << "arr1 length : " << arr1Length;
    for (uint32_t i = 0; i < arr1Length; ++i) {
        Local<JSValueRef> arr1Item = ArrayRef::GetValueAt(vm_, arr1, i);
        if (arr1Item->IsNumber()) {
            int num = arr1Item->Int32Value(vm_);
            GTEST_LOG_(INFO) << "arr1 index : " << i << "  value : " << num;
        } else {
            GTEST_LOG_(INFO) << "arr1 index : " << i << "  not number !";
        }
    }
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array1 =======================================";
}

/* demo11 json 测试，json字符串转为数组。 json:
 * ["a1","a2","a3","a4","a5","a6"]
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_3_parse_array2)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array2 =======================================";
    LocalScope scope(vm_);

    // 随机的一个json字符串数组。 ["a1","a2","a3","a4","a5","a6"]
    Local<StringRef> arrStr2 = StringRef::NewFromUtf8(vm_, "[\"a1\",\"a2\",\"a3\",\"a4\",\"a5\",\"a6\"]");
    Local<JSValueRef> jsArr2 = JSON::Parse(vm_, arrStr2);
    bool jsArr2IsArr = jsArr2->IsArray(vm_);
    GTEST_LOG_(INFO) << "jsArr2 is array : " << jsArr2IsArr;
    Local<ArrayRef> arr2 = jsArr2;
    uint32_t arr2Length = arr2->Length(vm_);
    GTEST_LOG_(INFO) << "arr2 length : " << arr2Length;
    for (uint32_t i = 0; i < arr2Length; ++i) {
        Local<JSValueRef> arr2Item = ArrayRef::GetValueAt(vm_, arr2, i);
        std::string str = arr2Item->ToString(vm_)->ToString();
        GTEST_LOG_(INFO) << "arr2 index : " << i << "  value : " << str;
    }

    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array2 =======================================";
}

/* demo11 json 测试，json字符串转为数组。 json:
 * [true,true,false,false,true]
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_3_parse_array3)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array3 =======================================";
    LocalScope scope(vm_);
    Local<StringRef> arrStr3 = StringRef::NewFromUtf8(vm_, "[true,true,false,false,true]");
    Local<JSValueRef> jsArr3 = JSON::Parse(vm_, arrStr3);
    bool jsArr3IsArr = jsArr3->IsArray(vm_);
    GTEST_LOG_(INFO) << "jsArr3 is array : " << jsArr3IsArr;
    Local<ArrayRef> arr3 = jsArr3;
    uint32_t arr3Length = arr3->Length(vm_);
    GTEST_LOG_(INFO) << "arr3 length : " << arr3Length;
    for (uint32_t i = 0; i < arr3Length; ++i) {
        Local<JSValueRef> arr3Item = ArrayRef::GetValueAt(vm_, arr3, i);
        int b = arr3Item->BooleaValue();
        GTEST_LOG_(INFO) << "arr3 index : " << i << "  value : " << b;
    }
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array3 =======================================";
}

/* demo11 json 测试，json字符串转为数组。 json:
 * [
 * {
 * "key1": "val11",
 * "key2": "val12"
 * },
 * {
 * "key1": "val21",
 * "key2": "val22"
 * },
 * {
 * "key1": "val31",
 * "key2": "val32"
 * }
 * ]
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_3_parse_array4)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array4 =======================================";
    LocalScope scope(vm_);

    // json 对象数组。
    // json: [{"key1": "val11","key2": "val12"},{"key1": "val21","key2": "val22"},{"key1": "val31","key2": "val32"}]
    Local<StringRef> arrStr4 =
        StringRef::NewFromUtf8(vm_, "[{\"key1\":\"val11\",\"key2\":\"val12\"},{\"key1\":\"val21\",\"key2\":\"val22\"},{"
        "\"key1\":\"val31\",\"key2\":\"val32\"}]");
    Local<JSValueRef> jsArr4 = JSON::Parse(vm_, arrStr4);
    bool jsArr4IsArr = jsArr4->IsArray(vm_);
    GTEST_LOG_(INFO) << "jsArr4 is array : " << jsArr4IsArr;
    Local<ArrayRef> arr4 = jsArr4;
    uint32_t arr4Length = arr4->Length(vm_);
    GTEST_LOG_(INFO) << "arr4 length : " << arr4Length;
    for (uint32_t i = 0; i < arr4Length; ++i) {
        Local<JSValueRef> jsArr4Item = ArrayRef::GetValueAt(vm_, arr4, i);
        Local<ObjectRef> obj = jsArr4Item->ToObject(vm_);

        Local<JSValueRef> objVal1 = obj->Get(vm_, StringRef::NewFromUtf8(vm_, "key1"));
        Local<JSValueRef> objVal2 = obj->Get(vm_, StringRef::NewFromUtf8(vm_, "key2"));

        GTEST_LOG_(INFO) << "arr4 index : " << i << "  key1 : " << objVal1->ToString(vm_)->ToString();
        GTEST_LOG_(INFO) << "arr4 index : " << i << "  key2 : " << objVal2->ToString(vm_)->ToString();
    }

    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array4 =======================================";
}

/* demo11 json 测试，json字符串转为数组。 json:
 * ["val1",222,true,{"key1": "val1","key2": "val2"}]
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_3_parse_array5)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array5 =======================================";
    LocalScope scope(vm_);

    // json 数组： ["val1",222,true,{"key1": "val1","key2": "val2"}]
    Local<StringRef> arrStr5 = StringRef::NewFromUtf8(vm_, "[\"val1\",222,true,{\"key1\":\"val1\",\"key2\":\"val1\"}]");
    Local<JSValueRef> jsArr5 = JSON::Parse(vm_, arrStr5);
    bool jsArr5IsArr = jsArr5->IsArray(vm_);
    GTEST_LOG_(INFO) << "jsArr5 is array : " << jsArr5IsArr;
    Local<ArrayRef> arr5 = jsArr5;
    uint32_t arr5Length = arr5->Length(vm_);
    GTEST_LOG_(INFO) << "arr5 length : " << arr5Length;
    for (uint32_t i = 0; i < arr5Length; ++i) {
        Local<JSValueRef> arr5Item = ArrayRef::GetValueAt(vm_, arr5, i);
        if (arr5Item->IsString()) {
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  value : " << arr5Item->ToString(vm_)->ToString();
        } else if (arr5Item->IsNumber()) {
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  value : " << arr5Item->Int32Value(vm_);
        } else if (arr5Item->IsBoolean()) {
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  value : " << arr5Item->ToBoolean(vm_)->Value();
        } else if (arr5Item->IsObject()) {
            Local<ObjectRef> obj = arr5Item->ToObject(vm_);
            Local<ObjectRef> val1 = obj->Get(vm_, StringRef::NewFromUtf8(vm_, "key1"));
            Local<ObjectRef> val2 = obj->Get(vm_, StringRef::NewFromUtf8(vm_, "key2"));
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  key1 : " << val1->ToString(vm_)->ToString();
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  key2 : " << val2->ToString(vm_)->ToString();
        } else {
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  not type !";
        }
    }
    GTEST_LOG_(INFO) << "sample_demo11_json_test_3_parse_array5 =======================================";
}

/* demo11 json 测试，数组转json字符串。 json:
 * ["val0","val1","val2","val3"]
 * [
 * {
 * "key1": "val11",
 * "key2": "val12",
 * "key3": "val13"
 * },
 * {
 * "key1": "val21",
 * "key2": "val22",
 * "key3": "val23"
 * }
 * ]
 * [
 * "val1",
 * 222,
 * true,
 * {
 * "key1": "val1",
 * "key2": "val2"
 * }
 * ]
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_4_stringify_array1)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_4_stringify_array1 =======================================";
    LocalScope scope(vm_);
    Local<ArrayRef> arr = ArrayRef::New(vm_);
    ArrayRef::SetValueAt(vm_, arr, 0, StringRef::NewFromUtf8(vm_, "val0"));
    ArrayRef::SetValueAt(vm_, arr, 1, StringRef::NewFromUtf8(vm_, "val1"));
    ArrayRef::SetValueAt(vm_, arr, 2, StringRef::NewFromUtf8(vm_, "val2"));
    ArrayRef::SetValueAt(vm_, arr, 3, StringRef::NewFromUtf8(vm_, "val3"));
    Local<JSValueRef> json1 = JSON::Stringify(vm_, arr);
    GTEST_LOG_(INFO) << " js arr 1 json : " << json1->ToString(vm_)->ToString();
    GTEST_LOG_(INFO) << "sample_demo11_json_test_4_stringify_array1 =======================================";
}

HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_4_stringify_array2)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_4_stringify_array2 =======================================";
    Local<ObjectRef> obj1 = ObjectRef::New(vm_);
    obj1->Set(vm_, StringRef::NewFromUtf8(vm_, "key1"), StringRef::NewFromUtf8(vm_, "val11"));
    obj1->Set(vm_, StringRef::NewFromUtf8(vm_, "key2"), StringRef::NewFromUtf8(vm_, "val12"));
    obj1->Set(vm_, StringRef::NewFromUtf8(vm_, "key3"), StringRef::NewFromUtf8(vm_, "val13"));
    Local<ObjectRef> obj2 = ObjectRef::New(vm_);
    obj2->Set(vm_, StringRef::NewFromUtf8(vm_, "key1"), StringRef::NewFromUtf8(vm_, "val21"));
    obj2->Set(vm_, StringRef::NewFromUtf8(vm_, "key2"), StringRef::NewFromUtf8(vm_, "val22"));
    obj2->Set(vm_, StringRef::NewFromUtf8(vm_, "key3"), StringRef::NewFromUtf8(vm_, "val23"));
    Local<ArrayRef> arr = ArrayRef::New(vm_);
    ArrayRef::SetValueAt(vm_, arr, 0, obj1);
    ArrayRef::SetValueAt(vm_, arr, 1, obj2);
    Local<JSValueRef> json2 = JSON::Stringify(vm_, arr);
    GTEST_LOG_(INFO) << " js arr 2 json : " << json2->ToString(vm_)->ToString();
    GTEST_LOG_(INFO) << "sample_demo11_json_test_4_stringify_array2 =======================================";
}

HWTEST_F_L0(JSNApiSampleTest, sample_demo11_json_test_4_stringify_array3)
{
    GTEST_LOG_(INFO) << "sample_demo11_json_test_4_stringify_array3 =======================================";
    Local<ObjectRef> obj = ObjectRef::New(vm_);
    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "key1"), StringRef::NewFromUtf8(vm_, "val1"));
    obj->Set(vm_, StringRef::NewFromUtf8(vm_, "key2"), StringRef::NewFromUtf8(vm_, "val2"));
    Local<ArrayRef> arr = ArrayRef::New(vm_);
    ArrayRef::SetValueAt(vm_, arr, 0, StringRef::NewFromUtf8(vm_, "val1"));
    int num2 = 222; // random number
    ArrayRef::SetValueAt(vm_, arr, 1, NumberRef::New(vm_, num2));
    ArrayRef::SetValueAt(vm_, arr, 2, BooleanRef::New(vm_, true));
    ArrayRef::SetValueAt(vm_, arr, 3, obj);
    Local<JSValueRef> json3 = JSON::Stringify(vm_, arr);
    GTEST_LOG_(INFO) << " js arr 3 json : " << json3->ToString(vm_)->ToString();
    GTEST_LOG_(INFO) << "sample_demo11_json_test_4_stringify_array3 =======================================";
}
}