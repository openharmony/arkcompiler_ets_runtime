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
        option.SetLogLevel(LOG_LEVEL::ERROR);
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

/* demo1 原始类型的使用。 */
HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_IntegerRef)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_IntegerRef_int=============================================================";
    int inputInt = 123; // int value = 123
    Local<IntegerRef> intObject = IntegerRef::New(vm_, inputInt);
    int intValue = intObject->Value();
    GTEST_LOG_(INFO) << "sample_primitive_IntegerRef_intValue : " << intValue;
    Local<PrimitiveRef> pintObject = intObject;
    GTEST_LOG_(INFO) << "sample_primitive_IntegerRef_pintObject_IsInt : " << pintObject->IsInt();
    GTEST_LOG_(INFO) << "sample_primitive_IntegerRef_uint============================================================";
    uint inputUint = 123; // uint value = 123
    Local<IntegerRef> uintObject = IntegerRef::NewFromUnsigned(vm_, inputUint);
    int uintValue = uintObject->Value();
    GTEST_LOG_(INFO) << "sample_primitive_uintValue : " << uintValue;
}

HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_NumberRef)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_double===========================================================";
    double inputDouble = 3.14; // double value = 3.14
    Local<NumberRef> doubleObject = NumberRef::New(vm_, inputDouble);
    double doubleValue = doubleObject->Value();
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_doubleValue : " << doubleValue;
    Local<PrimitiveRef> pdoubleObject = doubleObject;
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_pdoubleObject_IsNumber : " << pdoubleObject->IsNumber();
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_int32_t==========================================================";
    int32_t inputInt32t = 666; // int32-t value = 666
    Local<NumberRef> int32tObject = NumberRef::New(vm_, inputInt32t);
    double int32tValue = int32tObject->Value();
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_int32tValue : " << int32tValue;
    Local<PrimitiveRef> pint32tObject = int32tObject;
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_pint32tObject_Int32Value : " << pint32tObject->Int32Value(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_uint32_t=========================================================";
    uint32_t inputUint32t = 666; // uint32_t value = 666
    Local<NumberRef> uint32tObject = NumberRef::New(vm_, inputUint32t);
    double uint32tValue = uint32tObject->Value();
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_uint32tValue : " << uint32tValue;
    Local<PrimitiveRef> puint32tObject = uint32tObject;
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_puint32tObject_Uint32Value : " << puint32tObject->Uint32Value(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_int64_t==========================================================";
    int64_t inputInt64t = 666; // int64_t value = 666
    Local<NumberRef> int64tObject = NumberRef::New(vm_, inputInt64t);
    double int64tValue = int64tObject->Value();
    GTEST_LOG_(INFO) << "sample_primitive_NumberRef_int64tValue : " << int64tValue;
}

HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_BigIntRef_Uint64)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_uint64_t=========================================================";
    uint64_t inputUint64t = 999; // uint64_t value = 999
    Local<BigIntRef> uint64tObject = BigIntRef::New(vm_, inputUint64t);
    int64_t ucValue = 0;
    bool uLossless = true;
    uint64_t ucValue1 = 0;
    uint64tObject->BigIntToInt64(vm_, &ucValue, &uLossless);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_BigIntToInt64 : " << ucValue;
    uint64tObject->BigIntToUint64(vm_, &ucValue1, &uLossless);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_BigIntToUint64 : " << ucValue1;
    bool uSign = false;
    uint32_t uSize = 3;
    int uarraySize = 3;
    const uint64_t uWords[3] = {666, 777, 888};
    Local<JSValueRef> ubigWords = BigIntRef::CreateBigWords(vm_, uSign, uSize, uWords);
    for (int i = 0; i < uarraySize; i++) {
        GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_UwordsArrayindex"
                         << " [ " << i << " ] : " << uWords[i];
    }
    Local<BigIntRef> ubigWordsRef(ubigWords);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_UArraySize : " << ubigWordsRef->GetWordsArraySize(vm_);
    bool uresultSignBit = true;
    int unewarraySize = 3;
    uint64_t *uresultWords = new uint64_t[unewarraySize]();
    ubigWordsRef->GetWordsArray(vm_, &uresultSignBit, uSize, uresultWords);
    for (int i = 0; i < unewarraySize; i++) {
        GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_uresultWords"
                         << " [ " << i << " ] : " << uresultWords[i];
    }
}

HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_BigIntRef_Int64)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_int64_t==========================================================";
    int64_t inputInt64t = 111; // int64_t value = 111
    Local<BigIntRef> int64tObject = BigIntRef::New(vm_, inputInt64t);
    int64_t icValue = 0;
    bool iLossless = true;
    uint64_t icValue1 = 0;
    int64tObject->BigIntToInt64(vm_, &icValue, &iLossless);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_BigIntToInt64 : " << icValue;
    int64tObject->BigIntToUint64(vm_, &icValue1, &iLossless);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_BigIntToUint64 : " << icValue1;
    bool iSign = false;
    uint32_t iSize = 3;
    int iarraySize = 3;
    const uint64_t iWords[3] = {222, 333, 444};
    Local<JSValueRef> ibigWords = BigIntRef::CreateBigWords(vm_, iSign, iSize, iWords);
    for (int i = 0; i < iarraySize; i++) {
        GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_iWordsArrayindex"
                         << " [ " << i << " ] : " << iWords[i];
    }
    Local<BigIntRef> ibigWordsRef(ibigWords);
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_iarraySize : " << ibigWordsRef->GetWordsArraySize(vm_);
    bool iresultSignBit = true;
    int inewarraySize = 3;
    uint64_t *iresultWords = new uint64_t[inewarraySize]();
    ibigWordsRef->GetWordsArray(vm_, &iresultSignBit, iSize, iresultWords);
    for (int i = 0; i < inewarraySize; i++) {
        GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_iresultWords"
                         << " [ " << i << " ] : " << iresultWords[i];
    }
    Local<PrimitiveRef> pint64tObject = int64tObject;
    GTEST_LOG_(INFO) << "sample_primitive_BigIntRef_pint64tObject_IsBigInt : " << pint64tObject->IsBigInt(vm_);
}

HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_BooleanRef)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_BooleanRef_true============================================================";
    Local<BooleanRef> trueObject = BooleanRef::New(vm_, true);
    GTEST_LOG_(INFO) << "sample_primitive_BooleanRef_trueObject_Value : " << trueObject->Value();
    Local<PrimitiveRef> ptrueObject = trueObject;
    GTEST_LOG_(INFO) << "sample_primitive_BooleanRef_ptrueObject_IsBoolean : " << ptrueObject->IsBoolean();
    GTEST_LOG_(INFO) << "sample_primitive_BooleanRef_false===========================================================";
    Local<BooleanRef> falseObject = BooleanRef::New(vm_, false);
    GTEST_LOG_(INFO) << "sample_primitive_BooleanRef_falseObject_Value : " << falseObject->Value();
    Local<PrimitiveRef> pfalseObject = falseObject;
    GTEST_LOG_(INFO) << "sample_primitive_BooleanRef_pfalseObject_IsBoolean : " << pfalseObject->IsBoolean();
}

HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_StringRef_Char)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_char=============================================================";
    char utf8[12] = "Hello world";
    Local<JSValueRef> local = StringRef::NewFromUtf8(vm_, utf8);
    JSValueRef *jsValue = (*local);
    StringRef *charObject = StringRef::Cast(jsValue);
    EXPECT_EQ(charObject->Utf8Length(vm_), 12);
    char buffer[12];
    EXPECT_EQ(charObject->WriteUtf8(vm_, buffer, 12), 12);
    std::string res(buffer);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_charObject : " << res;
    std::string charObjectStr = charObject->ToString(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_charObject_ToString : " << charObjectStr;
    uint32_t charSize = charObject->Length(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_charObject_Length : " << charSize;
    char cs[16] = {0};
    uint32_t length = charObject->WriteLatin1(vm_, cs, 12);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_charObject_WriteLatin1 : " << length;
    Local<StringRef> napiWrapperString = charObject->GetNapiWrapperString(vm_);
    EXPECT_EQ(napiWrapperString->Utf8Length(vm_), 13);
    char buffer1[12];
    EXPECT_EQ(charObject->WriteUtf8(vm_, buffer1, 12), 12);
    std::string res1(buffer1);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_charObject_GetNapiWrapperString : " << res1;
    PrimitiveRef *pcharObject = charObject;
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_pcharObject_IsString : " << pcharObject->IsString(vm_);
}

HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_StringRef_Char16)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_char16_t=========================================================";
    const char16_t *utf16 = u"Hi,ABCDEF";
    Local<JSValueRef> local = StringRef::NewFromUtf16(vm_, utf16);
    JSValueRef *jsValue = (*local);
    StringRef *char16tObject = StringRef::Cast(jsValue);
    EXPECT_EQ(char16tObject->Utf8Length(vm_), 10);
    char16_t buffer2[10];
    EXPECT_EQ(char16tObject->WriteUtf16(vm_, buffer2, 9), 9);
    std::string res2(buffer2, buffer2 + sizeof(buffer2) / sizeof(char16_t));
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_char16tObject : " << res2 << std::endl;
    std::string char16tObjectStr = char16tObject->ToString(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_char16tObject_ToString : " << char16tObjectStr;
    uint32_t charSize = char16tObject->Length(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_char16tObject_Length : " << charSize;
    char cs1[10] = {0};
    uint32_t length = char16tObject->WriteLatin1(vm_, cs1, 10);
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_char16tObject_WriteLatin1 : " << length;
    Local<StringRef> napiWrapperString = char16tObject->GetNapiWrapperString(vm_);
    EXPECT_EQ(napiWrapperString->Utf8Length(vm_), 13);
    char16_t buffer3[10];
    EXPECT_EQ(char16tObject->WriteUtf16(vm_, buffer3, 9), 9);
    std::string res3(buffer3, buffer3 + sizeof(buffer3) / sizeof(char16_t));
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_char16tObject : " << res3;
    PrimitiveRef *pchar16tObject = char16tObject;
    GTEST_LOG_(INFO) << "sample_primitive_StringRef_pchar16tObject_IsString : " << pchar16tObject->IsString(vm_);
}

HWTEST_F_L0(JSNApiSampleTest, Sample_PrimitiveRef_SymbolRef)
{
    LocalScope scope(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_SymbolRef==================================================================";
    Local<SymbolRef> symbolObject = SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "description"));
    Local<StringRef> symbolDescription = symbolObject->GetDescription(vm_);
    GTEST_LOG_(INFO) << "sample_primitive_symbol_description : " << symbolDescription->ToString(vm_);
    Local<PrimitiveRef> psymbolObject = symbolObject;
    GTEST_LOG_(INFO) << "sample_primitive_SymbolRef_Psym_IsSymbol : " << psymbolObject->IsSymbol(vm_);
}

/* demo2 本地指针包装的使用。 */
class NativePointerTestClass {
public:
    NativePointerTestClass() : someProperty(0)
    {
        GTEST_LOG_(INFO) << "NativePointerTestClass Construction";
    }

    ~NativePointerTestClass()
    {
        GTEST_LOG_(INFO) << "NativePointerTestClass Destruction";
    }

    int GetsomeProperty() const
    {
        return someProperty;
    }

    void SetsomeProperty(int value)
    {
        someProperty = value;
    }

private:
    int someProperty;
};

HWTEST_F_L0(JSNApiSampleTest, sample_NativePointerRef)
{
    LocalScope scope(vm_);
    NativePointerTestClass *nativePointerTestClass = new NativePointerTestClass();
    GTEST_LOG_(INFO) << "sample_NativePointer adress: " << nativePointerTestClass;
    Local<NativePointerRef> nativePoint = NativePointerRef::New(
        vm_, nativePointerTestClass,
        [](void *env, void *value, void *hint) {
            GTEST_LOG_(INFO) << "NativePointerCb value : " << value;
            GTEST_LOG_(INFO) << "NativePointerCb hint  : " << hint;
            NativePointerTestClass *ptr = static_cast<NativePointerTestClass *>(value);
            delete ptr;
        },
        (void *)0x123);
    NativePointerTestClass *yourClassPtr = static_cast<NativePointerTestClass *>(nativePoint->Value());
    GTEST_LOG_(INFO) << "sample_NativePointer GetValue : " << yourClassPtr->GetsomeProperty();
    yourClassPtr->SetsomeProperty(99);
    GTEST_LOG_(INFO) << "sample_NativePointer SetValue : " << yourClassPtr->GetsomeProperty();
    GTEST_LOG_(INFO) << "sample_NativePointer Value : " << nativePoint->Value();
}

HWTEST_F_L0(JSNApiSampleTest, sample_NativePointerRef_String)
{
    LocalScope scope(vm_);
    void *vps = new std::string("123456");
    Local<NativePointerRef> nativePoint = NativePointerRef::New(vm_, vps);
    std::string *p = (std::string *)nativePoint->Value();
    GTEST_LOG_(INFO) << "sample_NativePointerRef_String Value : " << *p;
    std::string value = "78910";
    *p = value;
    GTEST_LOG_(INFO) << "sample_NativePointerRef_String ChangeValue : " << *((std::string *)nativePoint->Value());
}

HWTEST_F_L0(JSNApiSampleTest, sample_NativePointerRef_Double)
{
    LocalScope scope(vm_);
    void *vpd = new double(123.456);
    Local<NativePointerRef> nativePoint = NativePointerRef::New(vm_, vpd);
    double *p = (double *)nativePoint->Value();
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Double Value : " << *p;
    double value = 66.66;
    *p = value;
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Double ChangeValue: " << *((double *)nativePoint->Value());
}

HWTEST_F_L0(JSNApiSampleTest, sample_NativePointerRef_Char)
{
    LocalScope scope(vm_);
    void *vpc = new char('a');
    Local<NativePointerRef> nativePoint = NativePointerRef::New(vm_, vpc);
    char *p = (char *)nativePoint->Value();
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Char Value : " << *p;
    *p = 'A';
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Char ChangeValue: " << *((char *)nativePoint->Value());
}

HWTEST_F_L0(JSNApiSampleTest, sample_NativePointerRef_Long)
{
    LocalScope scope(vm_);
    void *vpl = new long(123456);
    Local<NativePointerRef> nativePoint = NativePointerRef::New(vm_, vpl);
    long *p = (long *)nativePoint->Value();
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Long Value : " << *p;
    long value = 2147483647;
    *p = value;
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Long ChangeValue: " << *((long *)nativePoint->Value());
}

HWTEST_F_L0(JSNApiSampleTest, sample_NativePointerRef_Int)
{
    LocalScope scope(vm_);
    void *vpi = new int(123);
    Local<NativePointerRef> nativePoint = NativePointerRef::New(vm_, vpi);
    int *p = (int *)nativePoint->Value();
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Int Value : " << *p;
    int value = 147483647;
    *p = value;
    GTEST_LOG_(INFO) << "sample_NativePointerRef_Int ChangeValue: " << *((int *)nativePoint->Value());
}

/* demo3 对object的操作 */
static std::vector<Local<SymbolRef>> GetSymbolRef(EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "GetSymbolRef";
    static std::vector<Local<SymbolRef>> value = { SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "symbolKey1")),
                                                   SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "symbolKey2")),
                                                   SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "symbolKey3")) };
    return value;
}

Local<JSValueRef> Setter1(JsiRuntimeCallInfo *info)
{
    GTEST_LOG_(INFO) << "Setter is running";
    int cnt = 0; // 0 = first element
    EscapeLocalScope scope(info->GetVM());
    Local<JSValueRef> arg = info->GetCallArgRef(0);
    Local<JSValueRef> value = info->GetThisRef();
    Local<ObjectRef> obj = value->ToObject(info->GetVM());
    if (value->IsNumber()) {
        double num = value->ToNumber(info->GetVM())->Value();
        int max = 100; // 100 = maxValue
        int min = 0;   // 0 = minValue
        if (num < min || num > max) {
            return JSValueRef::Undefined(info->GetVM());
        }
    }
    obj->Set(info->GetVM(), GetSymbolRef(info->GetVM())[cnt], arg);
    return JSValueRef::Undefined(info->GetVM());
}

Local<JSValueRef> Setter2(JsiRuntimeCallInfo *info)
{
    GTEST_LOG_(INFO) << "Setter is running";
    int cnt = 1; // 1 = second element
    EscapeLocalScope scope(info->GetVM());
    Local<JSValueRef> arg = info->GetCallArgRef(0);
    Local<JSValueRef> value = info->GetThisRef();
    Local<ObjectRef> obj = value->ToObject(info->GetVM());
    if (!arg->IsString(info->GetVM())) {
        return JSValueRef::Undefined(info->GetVM());
    }
    obj->Set(info->GetVM(), GetSymbolRef(info->GetVM())[cnt], arg);
    return JSValueRef::Undefined(info->GetVM());
}

Local<JSValueRef> Setter3(JsiRuntimeCallInfo *info)
{
    GTEST_LOG_(INFO) << "Setter is running";
    int cnt = 2; // 2 = third element
    EscapeLocalScope scope(info->GetVM());
    Local<JSValueRef> arg = info->GetCallArgRef(0);
    Local<JSValueRef> value = info->GetThisRef();
    Local<ObjectRef> obj = value->ToObject(info->GetVM());
    if (!arg->IsSymbol(info->GetVM())) {
        return JSValueRef::Undefined(info->GetVM());
    }
    obj->Set(info->GetVM(), GetSymbolRef(info->GetVM())[cnt], arg);
    return JSValueRef::Undefined(info->GetVM());
}

Local<JSValueRef> Getter1(JsiRuntimeCallInfo *info)
{
    GTEST_LOG_(INFO) << "Getter is running";
    int cnt = 0; // 0 = first element
    Local<JSValueRef> value = info->GetThisRef();
    Local<ObjectRef> obj = value->ToObject(info->GetVM());
    Local<JSValueRef> temp = obj->Get(info->GetVM(), GetSymbolRef(info->GetVM())[cnt]);
    return temp->ToString(info->GetVM());
}

Local<JSValueRef> Getter2(JsiRuntimeCallInfo *info)
{
    GTEST_LOG_(INFO) << "Getter is running";
    int cnt = 1; // 1 = second element
    Local<JSValueRef> value = info->GetThisRef();
    Local<ObjectRef> obj = value->ToObject(info->GetVM());
    Local<JSValueRef> temp = obj->Get(info->GetVM(), GetSymbolRef(info->GetVM())[cnt]);
    return temp->ToString(info->GetVM());
}

Local<JSValueRef> Getter3(JsiRuntimeCallInfo *info)
{
    GTEST_LOG_(INFO) << "Getter is running";
    int cnt = 2; // 2 = third element
    Local<JSValueRef> value = info->GetThisRef();
    Local<ObjectRef> obj = value->ToObject(info->GetVM());
    Local<JSValueRef> temp = obj->Get(info->GetVM(), GetSymbolRef(info->GetVM())[cnt]);
    if (!temp->IsSymbol(info->GetVM())) {
        JSValueRef::Undefined(info->GetVM());
    }
    Local<SymbolRef> str = temp;
    return str->GetDescription(info->GetVM());
}

void ObjectRefSet(Local<ObjectRef> object, EcmaVM *vm, Local<SymbolRef> symbol)
{
    GTEST_LOG_(INFO) << "ObjectRefSet";
    int cnt = 1; // 1 = key
    bool b = object->Set(vm, cnt, StringRef::NewFromUtf8(vm, "TestValue1"));
    ASSERT_TRUE(b);
    b = object->Set(vm, StringRef::NewFromUtf8(vm, "Test2"), StringRef::NewFromUtf8(vm, "TestValue2"));
    ASSERT_TRUE(b);
    b = object->Set(vm, StringRef::NewFromUtf8(vm, "Test3"), StringRef::NewFromUtf8(vm, "TestValue3"));
    ASSERT_TRUE(b);
    b = object->Set(vm, symbol, StringRef::NewFromUtf8(vm, "symbolValue"));
    ASSERT_TRUE(b);

    Local<FunctionRef> getter1 = FunctionRef::New(vm, Getter1);
    Local<FunctionRef> setter1 = FunctionRef::New(vm, Setter1);

    Local<FunctionRef> getter2 = FunctionRef::New(vm, Getter2);
    Local<FunctionRef> setter2 = FunctionRef::New(vm, Setter2);

    Local<FunctionRef> getter3 = FunctionRef::New(vm, Getter3);
    Local<FunctionRef> setter3 = FunctionRef::New(vm, Setter3);

    PropertyAttribute attribute1(StringRef::NewFromUtf8(vm, "AttributeValue1"), true, true, false);
    PropertyAttribute attribute2(StringRef::NewFromUtf8(vm, "AttributeValue2"), false, true, true);
    PropertyAttribute attribute3(StringRef::NewFromUtf8(vm, "AttributeValue3"), true, false, true);
    attribute1.SetGetter(getter1);
    attribute1.SetSetter(setter1);
    b = object->DefineProperty(vm, StringRef::NewFromUtf8(vm, "AttributeKey1"), attribute1);
    ASSERT_TRUE(b);
    b = object->DefineProperty(vm, StringRef::NewFromUtf8(vm, "Test2"), attribute2);
    ASSERT_TRUE(b);
    b = object->SetAccessorProperty(vm, StringRef::NewFromUtf8(vm, "Accessor1"), getter1, setter1);
    ASSERT_TRUE(b);
    b = object->SetAccessorProperty(vm, StringRef::NewFromUtf8(vm, "Test3"), getter2, setter2);
    ASSERT_TRUE(b);
    b = object->DefineProperty(vm, StringRef::NewFromUtf8(vm, "AttributeKey3"), attribute3);
    ASSERT_TRUE(b);
    b = object->SetAccessorProperty(vm, StringRef::NewFromUtf8(vm, "AttributeKey3"), getter3, setter3, attribute3);
    ASSERT_TRUE(b);
}

void GetProperty(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "GetProperty";
    Local<ArrayRef> names = object->GetOwnPropertyNames(vm);
    int cnt = names->Length(vm);
    GTEST_LOG_(INFO) << "GetOwnPropertyNames cnt: " << cnt;
    for (int i = 0; i < cnt; i++) {
        Local<JSValueRef> value = ArrayRef::GetValueAt(vm, names, i);
        if (value->IsSymbol(vm)) {
            Local<SymbolRef> symbol = value;
            GTEST_LOG_(INFO) << "PropertyNames: " << symbol->GetDescription(vm)->ToString(vm);
        } else {
            GTEST_LOG_(INFO) << "PropertyNames: " << value->ToString(vm)->ToString(vm);
        }
    }
}

void Get(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "Get";
    int cnt = 1; // 1 = key
    Local<JSValueRef> value = object->Get(vm, cnt);
    if (value->IsString(vm)) {
        GTEST_LOG_(INFO) << "Key:1  Value:" << value->ToString(vm)->ToString(vm);
    }
    value = object->Get(vm, StringRef::NewFromUtf8(vm, "Test2"));
    if (value->IsString(vm)) {
        GTEST_LOG_(INFO) << "Key:Test2  Value:" << value->ToString(vm)->ToString(vm);
    }
    value = object->Get(vm, StringRef::NewFromUtf8(vm, "AttributeKey1"));
    if (value->IsString(vm)) {
        GTEST_LOG_(INFO) << "Key:AttributeKey1  Value:" << value->ToString(vm)->ToString(vm);
    }
    int num = 10; // 10 = randomness
    object->Set(vm, StringRef::NewFromUtf8(vm, "Accessor1"), NumberRef::New(vm, num));
    object->Set(vm, StringRef::NewFromUtf8(vm, "Test3"), StringRef::NewFromUtf8(vm, "Test3Value"));
    object->Set(vm, StringRef::NewFromUtf8(vm, "AttributeKey3"),
        SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "AttributeKey3Value")));
    Local<StringRef> str = object->Get(vm, StringRef::NewFromUtf8(vm, "AttributeKey3"));
    if (str->IsString(vm)) {
        GTEST_LOG_(INFO) << "Key:AttributeKey3  Value:" << str->ToString(vm);
    }
    str = object->Get(vm, StringRef::NewFromUtf8(vm, "Accessor1"));
    if (str->IsString(vm)) {
        GTEST_LOG_(INFO) << "Key:Accessor1  Value:" << str->ToString(vm);
    }
    str = object->Get(vm, StringRef::NewFromUtf8(vm, "Test3"));
    if (str->IsString(vm)) {
        GTEST_LOG_(INFO) << "Key:Test3  Value:" << str->ToString(vm);
    }
}

void SetValueAgain(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "SetValueAgain";
    GTEST_LOG_(INFO) << "SetValueAgain";
    int cnt = 1; // 1 = key
    bool b = object->Set(vm, cnt, StringRef::NewFromUtf8(vm, "TestValueAgain1"));
    ASSERT_TRUE(b);
    b = object->Set(vm, StringRef::NewFromUtf8(vm, "Test3"), StringRef::NewFromUtf8(vm, "TestValueAgain3"));
    ASSERT_TRUE(b);
    b = object->Set(vm, StringRef::NewFromUtf8(vm, "AttributeKey1"),
        StringRef::NewFromUtf8(vm, "AttributeValueAgain1"));
    ASSERT_TRUE(b);
    b = object->Set(vm, StringRef::NewFromUtf8(vm, "AttributeKey3"),
        StringRef::NewFromUtf8(vm, "AttributeValueAgain3"));
    ASSERT_TRUE(b);
    b = object->Set(vm, StringRef::NewFromUtf8(vm, "Accessor1"), StringRef::NewFromUtf8(vm, "AttributeValueAgain2"));
    ASSERT_TRUE(b);
    GTEST_LOG_(INFO) << "GetValueAgain";
    Get(object, vm);
    GetProperty(object, vm);
}

void GetOwnEnumerablePropertyNames(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "GetOwnEnumerablePropertyNames";
    Local<ArrayRef> names = object->GetOwnEnumerablePropertyNames(vm);
    int cnt = names->Length(vm);
    GTEST_LOG_(INFO) << "GetOwnEnumerablePropertyNames cnt: " << cnt;
    for (int i = 0; i < cnt; i++) {
        Local<JSValueRef> value = ArrayRef::GetValueAt(vm, names, i);
        if (value->IsSymbol(vm)) {
            Local<SymbolRef> symbol = value;
            GTEST_LOG_(INFO) << "PropertyNames: " << symbol->GetDescription(vm)->ToString(vm);
        } else {
            GTEST_LOG_(INFO) << "PropertyNames: " << value->ToString(vm)->ToString(vm);
        }
    }
}

void PrintAllProperty(Local<ObjectRef> object, EcmaVM *vm, int flag)
{
    Local<ArrayRef> names = object->GetAllPropertyNames(vm, flag);
    int cnt = names->Length(vm);
    switch (flag) {
        case 0: // 0 = NATIVE_DEFAULT
            GTEST_LOG_(INFO) << "GetOwnPropertyNames NATIVE_DEFAULT: " << cnt;
            break;
        case 1: // 1 = NATIVE_WRITABLE
            GTEST_LOG_(INFO) << "GetOwnPropertyNames NATIVE_WRITABLE: " << cnt;
            break;
        case 2: // 2 = NATIVE_ENUMERABLE
            GTEST_LOG_(INFO) << "GetOwnPropertyNames NATIVE_ENUMERABLE: " << cnt;
            break;
        case 3: // 3 = NATIVE_CONFIGURABLE
            GTEST_LOG_(INFO) << "GetOwnPropertyNames NATIVE_CONFIGURABLE: " << cnt;
            break;
        default:
            break;
    }
    for (int i = 0; i < cnt; i++) {
        Local<JSValueRef> value = ArrayRef::GetValueAt(vm, names, i);
        if (value->IsSymbol(vm)) {
            Local<SymbolRef> symbol = value;
            GTEST_LOG_(INFO) << "PropertyNames: " << symbol->GetDescription(vm)->ToString(vm);
        } else {
            GTEST_LOG_(INFO) << "PropertyNames: " << value->ToString(vm)->ToString(vm);
        }
    }
}

void GetAllPropertyNames(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "GetAllPropertyNames";
    for (int i = 0; i < 4; i++) { // 4 show Go through all the properties
        PrintAllProperty(object, vm, i);
    }
}

void HasAndDelete(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "HasAndDelete";
    bool b = object->Has(vm, 1); // 1 = key
    ASSERT_TRUE(b);
    b = object->Has(vm, StringRef::NewFromUtf8(vm, "Test2"));
    ASSERT_TRUE(b);
    b = object->Delete(vm, StringRef::NewFromUtf8(vm, "Test2"));
    ASSERT_TRUE(b);
    b = object->Has(vm, StringRef::NewFromUtf8(vm, "Test2"));
    ASSERT_FALSE(b);
}

void FreezeAndSeal(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "FreezeAndSeal";
    object->Seal(vm);
    int num1 = 1; // 1 = key
    int num2 = 2; // 2 = key
    bool b = object->Set(vm, num1, StringRef::NewFromUtf8(vm, "Seal1"));
    ASSERT_TRUE(b);
    b = object->Delete(vm, num1);
    ASSERT_FALSE(b);
    b = object->Set(vm, num2, StringRef::NewFromUtf8(vm, "2"));
    ASSERT_FALSE(b);
    object->Freeze(vm);
    PropertyAttribute attribute(StringRef::NewFromUtf8(vm, "FreezeValue"), true, true, false);
    b = object->DefineProperty(vm, StringRef::NewFromUtf8(vm, "Freeze"), attribute);
    ASSERT_FALSE(b);
    b = object->Has(vm, num2);
    ASSERT_FALSE(b);
    b = object->Has(vm, StringRef::NewFromUtf8(vm, "Freeze"));
    ASSERT_FALSE(b);
    b = object->Delete(vm, num2);
    ASSERT_FALSE(b);
    b = object->Delete(vm, StringRef::NewFromUtf8(vm, "Freeze"));
    ASSERT_FALSE(b);
}

void GetOwnProperty(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "GetOwnProperty";
    PropertyAttribute value1;
    bool b = object->GetOwnProperty(vm, NumberRef::New(vm, 1), value1);
    ASSERT_TRUE(b);
    PropertyAttribute value2;
    b = object->GetOwnProperty(vm, StringRef::NewFromUtf8(vm, "AttributeKey1"), value2);
    ASSERT_TRUE(b);
    ASSERT_EQ(true, value2.IsWritable());
    ASSERT_EQ(true, value2.IsEnumerable());
    ASSERT_EQ(false, value2.IsConfigurable());
    ASSERT_EQ(true, value2.HasWritable());
    ASSERT_EQ(true, value2.HasConfigurable());
    ASSERT_EQ(true, value2.HasEnumerable());
    Local<JSValueRef> value = value2.GetValue(vm);
    ASSERT_EQ("AttributeValue1", value->ToString(vm)->ToString(vm));
}

class A {
public:
    void Test() const
    {
        GTEST_LOG_(INFO) << "Class A Test()";
    }
};

void NativePointer(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "NativePointer";
    int cnt = 10; // 10 = accommodate quantity
    object->SetNativePointerFieldCount(vm, cnt);
    ASSERT_EQ(cnt, object->GetNativePointerFieldCount(vm));
    A *a = new A();
    int cnt2 = 1;  // 11 = random Numbers
    int cnt3 = 11; // 1 = random Numbers
    object->SetNativePointerField(vm, cnt2, static_cast<void *>(a), nullptr, nullptr);
    object->SetNativePointerField(vm, cnt3, static_cast<void *>(a), nullptr, nullptr);
    A *value1 = static_cast<A *>(object->GetNativePointerField(vm, cnt2));
    A *value2 = static_cast<A *>(object->GetNativePointerField(vm, cnt3));
    if (value1 == nullptr) {
        GTEST_LOG_(INFO) << "SetNativePointerField is Error";
    } else {
        value1->Test();
    }
    if (value2 == nullptr) {
        GTEST_LOG_(INFO) << "SetNativePointerField is OK";
    }
    Local<NativePointerRef> native = NativePointerRef::New(vm, static_cast<void *>(a));
    bool b = object->ConvertToNativeBindingObject(vm, native);
    ASSERT_TRUE(b);
}

void SetPrototype(Local<ObjectRef> object, EcmaVM *vm)
{
    GTEST_LOG_(INFO) << "SetPrototype";
    int cnt = 111; // 111 = random Numbers
    Local<NumberRef> obj = NumberRef::New(vm, cnt);
    object->SetPrototype(vm, obj);
    Local<JSValueRef> type = object->GetPrototype(vm);
    if (type->IsUndefined() || type->IsNull()) {
        return;
    }
    GTEST_LOG_(INFO) << "Prototype: " << type->Typeof(vm)->ToString(vm);
}

HWTEST_F_L0(JSNApiSampleTest, Sample_Demo3_ObjectRef)
{
    GTEST_LOG_(INFO) << "SetPrototype";
    LocalScope scope(vm_);
    GetSymbolRef(vm_);
    Local<SymbolRef> symbol = SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "Symbol"));
    Local<ObjectRef> object = ObjectRef::New(vm_);
    SetPrototype(object, vm_);
    ObjectRefSet(object, vm_, symbol);
    GetProperty(object, vm_);
    GetOwnProperty(object, vm_);
    GetAllPropertyNames(object, vm_);
    GetOwnEnumerablePropertyNames(object, vm_);
    Get(object, vm_);
    NativePointer(object, vm_);
    SetValueAgain(object, vm_);
    HasAndDelete(object, vm_);
    FreezeAndSeal(object, vm_);
}

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
        [](void *env, void *nativePointer, void *data) {
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
    if (jsClaFunc->IsFunction(vm)) {
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
            if (jsPrivateGreeting->IsString(vm)) {
                ret.append("Hello, ").append(jsPrivateGreeting->ToString(vm)->ToString(vm));
            } else if (jsGreeting->IsString(vm)) {
                ret.append("Hello, ").append(jsGreeting->ToString(vm)->ToString(vm));
            } else {
                ret.append(jsStandardGreetingStr->ToString(vm)->ToString(vm));
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
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString(vm_);
    Greeter::SetPrivateGreeting(vm_, greeter1, StringRef::NewFromUtf8(vm_, "vip"));
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString(vm_);
    Greeter::SetPrivateGreeting(vm_, greeter1, JSValueRef::Undefined(vm_));
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString(vm_);
    // 修改变量
    Local<ObjectRef> objRef1 = greeter1;
    objRef1->Set(vm_, StringRef::NewFromUtf8(vm_, "greeting"), JSValueRef::Undefined(vm_));
    GTEST_LOG_(INFO) << Greeter::Greet(vm_, greeter1)->ToString(vm_);

    // 静态函数调用。
    GTEST_LOG_(INFO) << Greeter::StandardGreeting(vm_)->ToString(vm_);
    GTEST_LOG_(INFO) << Greeter::StandardPosition(vm_)->ToString(vm_);

    // 修改静态变量。
    Local<FunctionRef> classFunc = Greeter::GetClassFunction(vm_);
    classFunc->Set(vm_, StringRef::NewFromUtf8(vm_, "position"), StringRef::NewFromUtf8(vm_, "house"));

    GTEST_LOG_(INFO) << Greeter::StandardPosition(vm_)->ToString(vm_);
    GTEST_LOG_(INFO) << "sample_demo5_class_test =======================================";
}

/* demo6 多态 ts：
 * // 基类
 * class Derive {
 * baseNum: number = 1
 * constructor(num: number){
 * this.baseNum = num
 * }
 * Compute(): number {
 * return this.baseNum
 * }
 * }
 * // 子类1
 * class DeriveDouble extends Derive {
 * constructor(num: number){
 * super(num);
 * }
 * Compute() : number {
 * return this.baseNum * 2
 * }
 * }
 * // 子类2
 * class DerivedTriple extends Derive {
 * constructor(num: number){
 * super(num);
 * }
 * Compute() : number {
 * return this.baseNum * 3
 * }
 * }
 *
 * // 测试：
 * let d1: Derive;
 * let d2: Derive;
 * let d3: Derive;
 * d1 = new Derive(5);//新建基类。
 * d2 = new DeriveDouble(5);//新建子类。
 * d3 = new DerivedTriple(5);//新建子类。
 * let i1:number = d1.Compute();
 * let i2:number = d2.Compute();
 * let i3:number = d3.Compute();
 * console.log(i1);
 * console.log(i2);
 * console.log(i3);
 */

/* 基类 ts:
 * class Derive {
 * baseNum: number = 1
 * constructor(num: number){
 * this.baseNum = num
 * }
 * Compute(): number {
 * return this.baseNum
 * }
 * }
 */
class Derive {
private:
    // 新建ClassFunction
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm);

public:
    // 获取ClassFunction
    static Local<FunctionRef> GetClassFunction(EcmaVM *vm);
    static Local<Derive> New(EcmaVM *vm, Local<NumberRef> num);

    static Local<NumberRef> Compute(EcmaVM *vm, Local<Derive> thisRef);

private:
    const static std::string CLASS_NAME;
};

const std::string Derive::CLASS_NAME = "DeriveClass";

Local<FunctionRef> Derive::NewClassFunction(EcmaVM *vm)
{
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
            Local<JSValueRef> num = runtimeInfo->GetCallArgRef(0);
            // this.baseNum = num
            thisRef->Set(vm, StringRef::NewFromUtf8(vm, "baseNum"), num);
            return thisRef;
        },
        nullptr, nullptr);
    Local<JSValueRef> jsProto = claFunc->GetFunctionPrototype(vm);
    Local<ObjectRef> proto = jsProto->ToObject(vm);
    // 添加非静态变量
    proto->Set(vm, StringRef::NewFromUtf8(vm, "baseNum"), NumberRef::New(vm, 1));
    // 添加非静态函数
    proto->Set(vm, StringRef::NewFromUtf8(vm, "Compute"),
        FunctionRef::New(vm,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);

            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);

            Local<JSValueRef> jsNum = thisRef->Get(vm, StringRef::NewFromUtf8(vm, "baseNum"));
            int num = jsNum->Int32Value(vm);
            return NumberRef::New(vm, num);
        }));
    // 设置类名。
    claFunc->SetName(vm, StringRef::NewFromUtf8(vm, Derive::CLASS_NAME.c_str()));
    return claFunc;
}

Local<FunctionRef> Derive::GetClassFunction(EcmaVM *vm)
{
    Local<ObjectRef> globalObj = JSNApi::GetGlobalObject(vm);
    Local<JSValueRef> jsClaFunc = globalObj->Get(vm, StringRef::NewFromUtf8(vm, Derive::CLASS_NAME.c_str()));
    if (jsClaFunc->IsFunction(vm)) {
        return jsClaFunc;
    }
    Local<FunctionRef> claFunc = Derive::NewClassFunction(vm);
    // 添加到全局。
    globalObj->Set(vm, claFunc->GetName(vm), claFunc);
    return claFunc;
}

Local<Derive> Derive::New(EcmaVM *vm, Local<NumberRef> num)
{
    // 获取类函数。
    Local<FunctionRef> claFunc = Derive::GetClassFunction(vm);
    // 定义参数。
    Local<JSValueRef> argv[1] = {num};
    // 构造函数，构造对象。
    Local<JSValueRef> jsObj = claFunc->Constructor(vm, argv, 1);
    Local<ObjectRef> obj = jsObj->ToObject(vm);
    return obj;
}

Local<NumberRef> Derive::Compute(EcmaVM *vm, Local<Derive> thisRef)
{
    Local<ObjectRef> objRef = thisRef;
    Local<FunctionRef> func = objRef->Get(vm, StringRef::NewFromUtf8(vm, "Compute"));
    return func->Call(vm, objRef, nullptr, 0);
}

/* 子类1 ts:
 * class DeriveDouble extends Base {
 * constructor(num: number){
 * super(num);
 * }
 * Compute() : number {
 * return this.baseNum * 2
 * }
 * }
 */
class DeriveDouble : public Derive {
private:
    DeriveDouble() = default;
    // 新建ClassFunction
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm);

public:
    // 获取ClassFunction
    static Local<FunctionRef> GetClassFunction(EcmaVM *vm);
    static Local<DeriveDouble> New(EcmaVM *vm, Local<NumberRef> num);
    ~DeriveDouble() = default;

public:
    // 设置类名
    const static std::string CLASS_NAME;
};

const std::string DeriveDouble::CLASS_NAME = "DeriveDoubleClass";

Local<FunctionRef> DeriveDouble::NewClassFunction(EcmaVM *vm)
{
    Local<FunctionRef> claFunc = FunctionRef::NewClassFunction(
        vm,
        // 构造函数调用
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // 获取参数。
            Local<JSValueRef> num = runtimeInfo->GetCallArgRef(0);
            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
            // 修改父类。
            thisRef->Set(vm, StringRef::NewFromUtf8(vm, "baseNum"), num);
            return thisRef;
        },
        nullptr, nullptr);
    // 设置类名。
    claFunc->SetName(vm, StringRef::NewFromUtf8(vm, DeriveDouble::CLASS_NAME.c_str()));

    // 添加非静态函数
    Local<ObjectRef> proto = claFunc->GetFunctionPrototype(vm)->ToObject(vm);
    proto->Set(vm, StringRef::NewFromUtf8(vm, "Compute"),
        FunctionRef::New(vm,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
            Local<JSValueRef> jsNum = thisRef->Get(vm, StringRef::NewFromUtf8(vm, "baseNum"));
            int num = jsNum->Int32Value(vm);
            int multiple = 2; // 函数功能，2倍返回。
            num *= multiple;
            return NumberRef::New(vm, num);
        }));
    // 父类。
    Local<FunctionRef> claFuncBase = Derive::GetClassFunction(vm);
    // 继承。
    claFunc->Inherit(vm, claFuncBase);
    return claFunc;
}

Local<FunctionRef> DeriveDouble::GetClassFunction(EcmaVM *vm)
{
    Local<ObjectRef> globalObj = JSNApi::GetGlobalObject(vm);
    Local<JSValueRef> jsClaFunc = globalObj->Get(vm, StringRef::NewFromUtf8(vm, DeriveDouble::CLASS_NAME.c_str()));
    if (jsClaFunc->IsFunction(vm)) {
        return jsClaFunc;
    }
    Local<FunctionRef> claFunc = DeriveDouble::NewClassFunction(vm);
    globalObj->Set(vm, claFunc->GetName(vm), claFunc);
    return claFunc;
}

Local<DeriveDouble> DeriveDouble::New(EcmaVM *vm, Local<NumberRef> num)
{
    // 获取类函数。
    Local<FunctionRef> claFunc = DeriveDouble::GetClassFunction(vm);
    // 定义参数。
    Local<JSValueRef> argv[1] = {num};
    // 构造函数，构造对象。
    Local<JSValueRef> jsObj = claFunc->Constructor(vm, argv, 1);
    Local<ObjectRef> obj = jsObj->ToObject(vm);
    return obj;
}

/* 子类2 ts:
 * class DerivedTriple extends Derive {
 * constructor(num: number){
 * super(num);
 * }
 * Compute() : number {
 * return this.baseNum * 3
 * }
 * }
 */
class DerivedTriple : public Derive {
private:
    DerivedTriple() = default;
    // 新建ClassFunction
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm);

public:
    // 获取ClassFunction
    static Local<FunctionRef> GetClassFunction(EcmaVM *vm);
    static Local<DerivedTriple> New(EcmaVM *vm, Local<NumberRef> num);
    ~DerivedTriple() = default;

public:
    // 设置类名
    const static std::string CLASS_NAME;
};

const std::string DerivedTriple::CLASS_NAME = "DerivedTripleClass";

Local<FunctionRef> DerivedTriple::NewClassFunction(EcmaVM *vm)
{
    Local<FunctionRef> claFunc = FunctionRef::NewClassFunction(
        vm,
        // 构造函数调用
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // 获取参数。
            Local<JSValueRef> num = runtimeInfo->GetCallArgRef(0);
            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
            // 修改父类。
            thisRef->Set(vm, StringRef::NewFromUtf8(vm, "baseNum"), num);
            return thisRef;
        },
        nullptr, nullptr);
    // 设置类名。
    claFunc->SetName(vm, StringRef::NewFromUtf8(vm, DerivedTriple::CLASS_NAME.c_str()));

    // 添加非静态函数
    Local<ObjectRef> proto = claFunc->GetFunctionPrototype(vm)->ToObject(vm);
    proto->Set(vm, StringRef::NewFromUtf8(vm, "Compute"),
        FunctionRef::New(vm,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // 获取this
            Local<JSValueRef> jsThisRef = runtimeInfo->GetThisRef();
            Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
            Local<JSValueRef> jsNum = thisRef->Get(vm, StringRef::NewFromUtf8(vm, "baseNum"));
            int num = jsNum->Int32Value(vm);
            int multiple = 3; // 函数功能，3倍返回。
            num *= multiple;
            return NumberRef::New(vm, num);
        }));
    // 父类。
    Local<FunctionRef> claFuncBase = Derive::GetClassFunction(vm);
    // 继承。
    claFunc->Inherit(vm, claFuncBase);
    return claFunc;
}

Local<FunctionRef> DerivedTriple::GetClassFunction(EcmaVM *vm)
{
    Local<ObjectRef> globalObj = JSNApi::GetGlobalObject(vm);
    Local<JSValueRef> jsClaFunc = globalObj->Get(vm, StringRef::NewFromUtf8(vm, DerivedTriple::CLASS_NAME.c_str()));
    if (jsClaFunc->IsFunction(vm)) {
        return jsClaFunc;
    }
    Local<FunctionRef> claFunc = DerivedTriple::NewClassFunction(vm);
    globalObj->Set(vm, claFunc->GetName(vm), claFunc);
    return claFunc;
}

Local<DerivedTriple> DerivedTriple::New(EcmaVM *vm, Local<NumberRef> num)
{
    // 获取类函数。
    Local<FunctionRef> claFunc = DerivedTriple::GetClassFunction(vm);
    // 定义参数。
    Local<JSValueRef> argv[1] = {num};
    // 构造函数，构造对象。
    Local<JSValueRef> jsObj = claFunc->Constructor(vm, argv, 1);
    Local<ObjectRef> obj = jsObj->ToObject(vm);
    return obj;
}

/* 测试。ts:
 * let d1: Derive;
 * let d2: Derive;
 * let d3: Derive;
 * d1 = new Derive(5);//新建基类。
 * d2 = new DeriveDouble(5);//新建子类。
 * d3 = new DerivedTriple(5);//新建子类。
 * let i1:number = d1.Compute();
 * let i2:number = d2.Compute();
 * let i3:number = d3.Compute();
 * console.log(i1);
 * console.log(i2);
 * console.log(i3);
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo6_polymorphic_test)
{
    GTEST_LOG_(INFO) << "sample_demo6_polymorphic_test =======================================";
    LocalScope scope(vm_);

    int num = 5;

    Local<Derive> d1 = Derive::New(vm_, NumberRef::New(vm_, num));
    Local<Derive> d2 = DeriveDouble::New(vm_, NumberRef::New(vm_, num));
    Local<Derive> d3 = DerivedTriple::New(vm_, NumberRef::New(vm_, num));

    Local<NumberRef> i1 = Derive::Compute(vm_, d1);
    Local<NumberRef> i2 = Derive::Compute(vm_, d2);
    Local<NumberRef> i3 = Derive::Compute(vm_, d3);

    GTEST_LOG_(INFO) << "i1 = " << i1->Int32Value(vm_);
    GTEST_LOG_(INFO) << "i2 = " << i2->Int32Value(vm_);
    GTEST_LOG_(INFO) << "i3 = " << i3->Int32Value(vm_);

    GTEST_LOG_(INFO) << "sample_demo6_polymorphic_test =======================================";
}

/* demo7 数组的使用 */
HWTEST_F_L0(JSNApiSampleTest, sample_ArrayRef_Int)
{
    LocalScope scope(vm_);
    uint32_t length = 5; // array length = 5
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
    uint32_t arrayLength = arrayObject->Length(vm_);
    uint32_t index = 1; // array index = 0
    for (int i = 0; i < (int)arrayLength; i++) {
        Local<IntegerRef> intValue = IntegerRef::New(vm_, i * 10);
        ArrayRef::SetValueAt(vm_, arrayObject, i, intValue);
        GTEST_LOG_(INFO) << "sample_setIntValue_index_" << i << ": " << intValue->Value();
    }
    Local<IntegerRef> resultInt = ArrayRef::GetValueAt(vm_, arrayObject, index);
    GTEST_LOG_(INFO) << "sample_getIntValue_index_1: " << resultInt->Value();
    int inputInt = 99; // int data = 99
    Local<IntegerRef> intValue = IntegerRef::New(vm_, inputInt);
    bool setResult = ArrayRef::SetValueAt(vm_, arrayObject, arrayLength, intValue);
    GTEST_LOG_(INFO) << "sample_setIntValue_index_5: " << intValue->Value() << " setResult: " << setResult;
    int changedInt = 66; // change data = 66
    Local<IntegerRef> changedIntValue = IntegerRef::New(vm_, changedInt);
    ArrayRef::SetValueAt(vm_, arrayObject, index, changedIntValue);
    GTEST_LOG_(INFO) << "sample_changedIntValue_index_1: " << changedIntValue->Value();
    Local<IntegerRef> resultChangedInt = ArrayRef::GetValueAt(vm_, arrayObject, index);
    GTEST_LOG_(INFO) << "sample_getChangedIntValue_index_1: " << resultChangedInt->Value();
    for (int i = 0; i < (int)arrayLength; i++) {
        Local<IntegerRef> printInt = ArrayRef::GetValueAt(vm_, arrayObject, i);
        GTEST_LOG_(INFO) << "sample_printIntValue_index_" << i << ": " << printInt->Value();
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_ArrayRef_Bool)
{
    LocalScope scope(vm_);
    uint32_t length = 5; // array length = 5
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
    uint32_t arrayLength = arrayObject->Length(vm_);
    bool inputBool = true; // bool data = true
    uint32_t index = 0;    // array index = 0
    Local<BooleanRef> boolValue = BooleanRef::New(vm_, inputBool);
    for (int i = 0; i < (int)arrayLength; i++) {
        ArrayRef::SetValueAt(vm_, arrayObject, i, boolValue);
        GTEST_LOG_(INFO) << "sample_setBoolValue_index_" << i << ": " << boolValue->Value();
    }
    Local<BooleanRef> resultBool = ArrayRef::GetValueAt(vm_, arrayObject, index);
    GTEST_LOG_(INFO) << "sample_getBoolValue_index_0: " << resultBool->Value();
    bool setResult = ArrayRef::SetValueAt(vm_, arrayObject, arrayLength, boolValue);
    GTEST_LOG_(INFO) << "sample_setBoolValue_index_5: " << boolValue->Value() << " setResult: " << setResult;
    bool changedBool = false; // change data = false
    Local<BooleanRef> changedBoolValue = BooleanRef::New(vm_, changedBool);
    ArrayRef::SetValueAt(vm_, arrayObject, index, changedBoolValue);
    GTEST_LOG_(INFO) << "sample_changedBoolValue_index_0: " << changedBoolValue->Value();
    Local<BooleanRef> resultChangedBool = ArrayRef::GetValueAt(vm_, arrayObject, index);
    GTEST_LOG_(INFO) << "sample_getChangedBoolValue_index_0: " << resultChangedBool->Value();
    for (int i = 0; i < (int)arrayLength; i++) {
        Local<BooleanRef> printBool = ArrayRef::GetValueAt(vm_, arrayObject, i);
        GTEST_LOG_(INFO) << "sample_printBoolValue_index_" << i << ": " << printBool->Value();
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_ArrayRef_Number)
{
    LocalScope scope(vm_);
    uint32_t length = 5; // array length = 5
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
    uint32_t arrayLength = arrayObject->Length(vm_);
    uint32_t inputNumber = 40; // number data = 40
    uint32_t index = 0;        // array index = 0
    Local<NumberRef> numberValue = NumberRef::New(vm_, inputNumber);
    for (int i = 0; i < (int)arrayLength; i++) {
        ArrayRef::SetValueAt(vm_, arrayObject, i, numberValue);
        GTEST_LOG_(INFO) << "sample_setNumberValue_index_" << i << ": " << numberValue->Value();
    }
    Local<NumberRef> resultNumber = ArrayRef::GetValueAt(vm_, arrayObject, index);
    GTEST_LOG_(INFO) << "sample_getNumberValue_index_0: " << resultNumber->Value();
    bool setResult = ArrayRef::SetValueAt(vm_, arrayObject, arrayLength, numberValue);
    GTEST_LOG_(INFO) << "sample_setNumberValue_index_5: " << numberValue->Value() << " setResult: " << setResult;
    uint32_t changedNumber = 50; // change data = 50
    Local<NumberRef> changedNumberValue = NumberRef::New(vm_, changedNumber);
    ArrayRef::SetValueAt(vm_, arrayObject, index, changedNumberValue);
    GTEST_LOG_(INFO) << "sample_changedNumberValue_index_0: " << changedNumberValue->Value();
    Local<NumberRef> resultChangedNumber = ArrayRef::GetValueAt(vm_, arrayObject, index);
    GTEST_LOG_(INFO) << "sample_getChangedNumberValue_index_0: " << resultChangedNumber->Value();
    for (int i = 0; i < (int)arrayLength; i++) {
        Local<NumberRef> printNumber = ArrayRef::GetValueAt(vm_, arrayObject, i);
        GTEST_LOG_(INFO) << "sample_printNumberValue_index_" << i << ": " << printNumber->Value();
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_ArrayRef_String)
{
    LocalScope scope(vm_);
    uint32_t length = 5; // array length = 5
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
    uint32_t arrayLength = arrayObject->Length(vm_);
    std::string inputString = "Sample Test";
    uint32_t index = 0; // array index = 0
    Local<StringRef> stringValue = StringRef::NewFromUtf8(vm_, inputString.c_str());
    for (int i = 0; i < (int)arrayLength; i++) {
        ArrayRef::SetValueAt(vm_, arrayObject, i, stringValue);
        char setBuffer[20];
        stringValue->WriteUtf8(vm_, setBuffer, inputString.length());
        std::string result(setBuffer);
        GTEST_LOG_(INFO) << "sample_setStringValue_index_" << i << ": " << result;
        memset_s(setBuffer, sizeof(setBuffer), 0, sizeof(setBuffer));
    }
    Local<StringRef> resultString = ArrayRef::GetValueAt(vm_, arrayObject, index);
    char getBuffer[20];
    resultString->WriteUtf8(vm_, getBuffer, inputString.length());
    std::string getResult(getBuffer);
    GTEST_LOG_(INFO) << "sample_getStringValue_index_0: " << getResult;
    bool setResult = ArrayRef::SetValueAt(vm_, arrayObject, arrayLength, stringValue);
    GTEST_LOG_(INFO) << "sample_setStringValue_index_5"
                     << " setResult: " << setResult;
    std::string changedString = "Change Test";
    Local<StringRef> changedStringValue = StringRef::NewFromUtf8(vm_, changedString.c_str());
    ArrayRef::SetValueAt(vm_, arrayObject, index, changedStringValue);
    Local<StringRef> resultChangedString = ArrayRef::GetValueAt(vm_, arrayObject, index);
    char changedBuffer[20];
    resultChangedString->WriteUtf8(vm_, changedBuffer, changedString.length());
    std::string changedResult(changedBuffer);
    GTEST_LOG_(INFO) << "sample_getChangedStringValue_index_0: " << changedResult;
    for (int i = 0; i < (int)arrayLength; i++) {
        Local<StringRef> printString = ArrayRef::GetValueAt(vm_, arrayObject, i);
        char printBuffer[20];
        printString->WriteUtf8(vm_, printBuffer, inputString.length());
        std::string printResult(printBuffer);
        GTEST_LOG_(INFO) << "sample_printStringValue_index_" << i << ": " << printResult;
        memset_s(printBuffer, sizeof(printBuffer), 0, sizeof(printBuffer));
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Int8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15; // arraybuffer length = 15
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    int8_t *ptr = (int8_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength; i++) {
        *ptr = int8_t(i + 10);
        ptr++;
    }
    int32_t byteOffset = 5;      // byte offset = 5
    int32_t int8ArrayLength = 6; // array length = 6
    Local<Int8ArrayRef> typedArray = Int8ArrayRef::New(vm_, arrayBuffer, byteOffset, int8ArrayLength);
    GTEST_LOG_(INFO) << "sample_Int8Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Int8Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Int8Array_arrayLength : " << typedArray->ArrayLength(vm_);
    int8_t *result = (int8_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < int8ArrayLength; i++) {
        int value = int8_t(*result);
        GTEST_LOG_(INFO) << "sample_Int8Array_getInt8ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Uint8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15; // arraybuffer length = 15
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    uint8_t *ptr = (uint8_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength; i++) {
        *ptr = uint8_t(i + 10);
        ptr++;
    }
    int32_t byteOffset = 5;       // byte offset = 5
    int32_t Uint8ArrayLength = 6; // array length = 6
    Local<Uint8ArrayRef> typedArray = Uint8ArrayRef::New(vm_, arrayBuffer, byteOffset, Uint8ArrayLength);
    GTEST_LOG_(INFO) << "sample_Uint8Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Uint8Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Uint8Array_arrayLength : " << typedArray->ArrayLength(vm_);
    uint8_t *result = (uint8_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < Uint8ArrayLength; i++) {
        int value = uint8_t(*result);
        GTEST_LOG_(INFO) << "sample_Uint8Array_getUint8ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Uint8ClampedArray)
{
    LocalScope scope(vm_);
    const int32_t length = 15; // arraybuffer length = 15
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    uint8_t *ptr = (uint8_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength; i++) {
        *ptr = uint8_t(i + 10);
        ptr++;
    }
    int32_t byteOffset = 5;     // byte offset = 5
    int32_t uint8ArrLength = 6; // array length = 6
    Local<Uint8ClampedArrayRef> typedArray = Uint8ClampedArrayRef::New(vm_, arrayBuffer, byteOffset, uint8ArrLength);
    GTEST_LOG_(INFO) << "sample_Uint8ClampedArray_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Uint8ClampedArray_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Uint8ClampedArray_arrayLength : " << typedArray->ArrayLength(vm_);
    uint8_t *result = (uint8_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < uint8ArrLength; i++) {
        int value = uint8_t(*result);
        GTEST_LOG_(INFO) << "sample_Uint8ClampedArray_getUint8ClampedArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Int16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30; // arraybuffer length = 30
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    int16_t *ptr = (int16_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 2; i++) {
        *ptr = int16_t(i + 10);
        ptr++;
    }
    int32_t byteOffset = 4;       // byte offset = 4
    int32_t int16ArrayLength = 6; // array length = 6
    Local<Int16ArrayRef> typedArray = Int16ArrayRef::New(vm_, arrayBuffer, byteOffset, int16ArrayLength);
    GTEST_LOG_(INFO) << "sample_Int16Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Int16Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Int16Array_arrayLength : " << typedArray->ArrayLength(vm_);
    int16_t *result = (int16_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < int16ArrayLength; i++) {
        int value = int16_t(*result);
        GTEST_LOG_(INFO) << "sample_Int16Array_getInt16ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Uint16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30; // arraybuffer length = 30
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    uint16_t *ptr = (uint16_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 2; i++) {
        *ptr = uint16_t(i + 10);
        ptr++;
    }
    int32_t byteOffset = 4;        // byte offset = 4
    int32_t uint16ArrayLength = 6; // array length = 6
    Local<Uint16ArrayRef> typedArray = Uint16ArrayRef::New(vm_, arrayBuffer, byteOffset, uint16ArrayLength);
    GTEST_LOG_(INFO) << "sample_Uint16Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Uint16Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Uint16Array_arrayLength : " << typedArray->ArrayLength(vm_);
    uint16_t *result = (uint16_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < uint16ArrayLength; i++) {
        int value = uint16_t(*result);
        GTEST_LOG_(INFO) << "sample_Uint16Array_getUint16ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Int32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 32; // arraybuffer length = 32
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    int32_t *ptr = (int32_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 4; i++) {
        *ptr = int32_t(i + 10);
        ptr++;
    }
    int32_t byteOffset = 4;       // byte offset = 4
    int32_t int32ArrayLength = 6; // array length = 6
    Local<Int32ArrayRef> typedArray = Int32ArrayRef::New(vm_, arrayBuffer, byteOffset, int32ArrayLength);
    GTEST_LOG_(INFO) << "sample_Int32Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Int32Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Int32Array_arrayLength : " << typedArray->ArrayLength(vm_);
    int32_t *result = (int32_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < int32ArrayLength; i++) {
        int value = int32_t(*result);
        GTEST_LOG_(INFO) << "sample_Int32Array_getInt32ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Uint32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 32; // arraybuffer length = 32
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    uint32_t *ptr = (uint32_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 4; i++) {
        *ptr = uint32_t(i + 10);
        ptr++;
    }
    int32_t byteOffset = 4;        // byte offset = 4
    int32_t uint32ArrayLength = 6; // array length = 6
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm_, arrayBuffer, byteOffset, uint32ArrayLength);
    GTEST_LOG_(INFO) << "sample_Uint32Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Uint32Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Uint32Array_arrayLength : " << typedArray->ArrayLength(vm_);
    uint32_t *result = (uint32_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < uint32ArrayLength; i++) {
        int value = uint32_t(*result);
        GTEST_LOG_(INFO) << "sample_Uint32Array_getUint32ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Float32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 32; // arraybuffer length = 32
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    float *ptr = (float *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 4; i++) {
        *ptr = float(i + 10);
        ptr++;
    }
    int32_t byteOffset = 4;         // byte offset = 4
    int32_t float32ArrayLength = 6; // array length = 6
    Local<Float32ArrayRef> typedArray = Float32ArrayRef::New(vm_, arrayBuffer, byteOffset, float32ArrayLength);
    GTEST_LOG_(INFO) << "sample_Float32Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Float32Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Float32Array_arrayLength : " << typedArray->ArrayLength(vm_);
    float *result = (float *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < float32ArrayLength; i++) {
        int value = float(*result);
        GTEST_LOG_(INFO) << "sample_Float32Array_getFloat32ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_Float64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 64; // arraybuffer length = 64
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    double *ptr = (double *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 8; i++) {
        *ptr = double(i + 10);
        ptr++;
    }
    int32_t byteOffset = 8;         // byte offset = 8
    int32_t float64ArrayLength = 6; // array length = 6
    Local<Float64ArrayRef> typedArray = Float64ArrayRef::New(vm_, arrayBuffer, byteOffset, float64ArrayLength);
    GTEST_LOG_(INFO) << "sample_Float64Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Float64Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_Float64Array_arrayLength : " << typedArray->ArrayLength(vm_);
    double *result = (double *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < float64ArrayLength; i++) {
        int value = double(*result);
        GTEST_LOG_(INFO) << "sample_Float64Array_getFloat64ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_BigInt64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 64; // arraybuffer length = 64
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    int64_t *ptr = (int64_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 8; i++) {
        *ptr = int64_t(i * 100);
        ptr++;
    }
    int32_t byteOffset = 8;          // byte offset = 8
    int32_t bigInt64ArrayLength = 6; // array length = 6
    Local<BigInt64ArrayRef> typedArray = BigInt64ArrayRef::New(vm_, arrayBuffer, byteOffset, bigInt64ArrayLength);
    GTEST_LOG_(INFO) << "sample_BigInt64Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_BigInt64Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_BigInt64Array_arrayLength : " << typedArray->ArrayLength(vm_);
    int64_t *result = (int64_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < bigInt64ArrayLength; i++) {
        int value = int64_t(*result);
        GTEST_LOG_(INFO) << "sample_BigInt64Array_getBigInt64ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_TypedArrayRef_BigUint64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 64; // arraybuffer length = 64
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    uint64_t *ptr = (uint64_t *)arrayBuffer->GetBuffer(vm_);
    for (int i = 0; i < arrayLength / 8; i++) {
        *ptr = int64_t(i * 100);
        ptr++;
    }
    int32_t byteOffset = 8;           // byte offset = 8
    int32_t bigUint64ArrayLength = 6; // array length = 6
    Local<BigUint64ArrayRef> typedArray = BigUint64ArrayRef::New(vm_, arrayBuffer, byteOffset, bigUint64ArrayLength);
    GTEST_LOG_(INFO) << "sample_BigUint64Array_byteLength : " << typedArray->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_BigUint64Array_byteOffset : " << typedArray->ByteOffset(vm_);
    GTEST_LOG_(INFO) << "sample_BigUint64Array_arrayLength : " << typedArray->ArrayLength(vm_);
    uint64_t *result = (uint64_t *)typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_);
    for (int i = 0; i < bigUint64ArrayLength; i++) {
        int value = uint64_t(*result);
        GTEST_LOG_(INFO) << "sample_BigUint64Array_getBigUint64ArrayValue : " << value;
        result++;
    }
}

HWTEST_F_L0(JSNApiSampleTest, sample_DataViewRef)
{
    LocalScope scope(vm_);
    const int32_t length = 15; // arraybuffer length = 15
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t byteOffset = 5;     // byte offset = 5
    int32_t dataViewLength = 7; // dataview length = 7
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, byteOffset, dataViewLength);
    GTEST_LOG_(INFO) << "sample_DataView_byteLength : " << dataView->ByteLength();
    GTEST_LOG_(INFO) << "sample_DataView_byteOffset : " << dataView->ByteOffset();
    GTEST_LOG_(INFO) << "sample_DataView_getArrayBuffer : " << dataView->GetArrayBuffer(vm_)->GetBuffer(vm_);
}

HWTEST_F_L0(JSNApiSampleTest, sample_ArrayBuffer_New_Detach)
{
    LocalScope scope(vm_);
    const int32_t length = 32; // arraybuffer length = 32
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    int32_t arrayLength = arrayBuffer->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_ArrayBuffer_byteLength : " << arrayLength;
    GTEST_LOG_(INFO) << "sample_ArrayBuffer_getArrayBuffer : " << arrayBuffer->GetBuffer(vm_);
    arrayBuffer->Detach(vm_);
    GTEST_LOG_(INFO) << "sample_ArrayBuffer_getDetachArrayBuffer : " << arrayBuffer->GetBuffer(vm_);
    bool result = arrayBuffer->IsDetach(vm_);
    GTEST_LOG_(INFO) << "sample_ArrayBuffer_getIsDetach : " << result;
}

HWTEST_F_L0(JSNApiSampleTest, sample_ArrayBufferWithBuffer_New_Detach)
{
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 5; // arraybuffer length = 5
    Data *data = new Data();
    data->length = length;
    NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        delete currentData;
        isFree = true;
    };
    LocalScope scope(vm_);
    uint8_t *buffer = new uint8_t[length]();
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, buffer, length, deleter, data);
    GTEST_LOG_(INFO) << "sample_ArrayBufferWithBuffer_byteLength : " << arrayBuffer->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_ArrayBufferWithBuffer_getArrayBuffer : " << arrayBuffer->GetBuffer(vm_);
    arrayBuffer->Detach(vm_);
    GTEST_LOG_(INFO) << "sample_ArrayBufferWithBuffer_getDetachArrayBuffer : " << arrayBuffer->GetBuffer(vm_);
    bool result = arrayBuffer->IsDetach(vm_);
    GTEST_LOG_(INFO) << "sample_ArrayBufferWithBuffer_getIsDetach : " << result;
}

HWTEST_F_L0(JSNApiSampleTest, sample_Buffer_New_GetBuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 5; // buffer length = 5
    Local<BufferRef> buffer = BufferRef::New(vm_, length);
    GTEST_LOG_(INFO) << "sample_Buffer_byteLength : " << buffer->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_Buffer_getBuffer : " << buffer->GetBuffer(vm_);
}

HWTEST_F_L0(JSNApiSampleTest, sample_BufferWithBuffer_New_GetBuffer)
{
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 5; // buffer length = 5
    Data *data = new Data();
    data->length = length;
    NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        delete currentData;
        isFree = true;
    };
    LocalScope scope(vm_);
    uint8_t *buffer = new uint8_t[length]();
    Local<BufferRef> bufferObj = BufferRef::New(vm_, buffer, length, deleter, data);
    GTEST_LOG_(INFO) << "sample_bufferWithBuffer_byteLength : " << bufferObj->ByteLength(vm_);
    GTEST_LOG_(INFO) << "sample_bufferWithBuffer_getBuffer : " << bufferObj->GetBuffer(vm_);
}

/* domo8 异步操作。 ts:
 * new Promise(function (resolve, reject) {
 * var a = 0;
 * var b = 1;
 * if (b == 0) reject("Divide zero");
 * else resolve(a / b);
 * }).then(function (value) {
 * console.log("a / b = " + value);
 * }).catch(function (err) {
 * console.log(err);
 * }).finally(function () {
 * console.log("End");
 * });
 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo8_async_test_1)
{
    GTEST_LOG_(INFO) << "sample_demo8_async_test_1 =======================================";
    LocalScope scope(vm_);

    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    capability->GetPromise(vm_)
        ->Then(vm_, FunctionRef::New(vm_,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            Local<JSValueRef> jsNum = runtimeInfo->GetCallArgRef(0);
            int num = jsNum->Int32Value(vm);
            // ts: console.log("a / b = " + value);
            GTEST_LOG_(INFO) << "a / b = " << num;
            return JSValueRef::Undefined(vm);
        }))
        ->Catch(vm_, FunctionRef::New(vm_,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            Local<JSValueRef> jsStr = runtimeInfo->GetCallArgRef(0);
            std::string err = jsStr->ToString(vm)->ToString(vm);
            // ts: console.log(err);
            GTEST_LOG_(INFO) << err;
            return JSValueRef::Undefined(vm);
        }))
        ->Finally(vm_, FunctionRef::New(vm_,
        // 函数体
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            // ts: console.log("End");
            GTEST_LOG_(INFO) << "End";
            return JSValueRef::Undefined(vm);
        }));

    int a = 0;
    int b = 0;
    if (b == 0) {
        capability->Reject(vm_, StringRef::NewFromUtf8(vm_, "Divide zero"));
    } else {
        capability->Resolve(vm_, NumberRef::New(vm_, int(a / b)));
    }

    GTEST_LOG_(INFO) << "sample_demo8_async_test_1 =======================================";
}

// JSValueRef转为字符串输出。
std::string jsValue2String(EcmaVM *vm, Local<JSValueRef> &jsVal)
{
    if (jsVal->IsString(vm)) {
        return "type string, val : " + jsVal->ToString(vm)->ToString(vm);
    } else if (jsVal->IsNumber()) {
        return "type number, val : " + std::to_string(jsVal->Int32Value(vm));
    } else if (jsVal->IsBoolean()) {
        return "type bool, val : " + std::to_string(jsVal->BooleaValue(vm));
    } else if (jsVal->IsSymbol(vm)) {
        Local<SymbolRef> symbol = jsVal;
        return "type symbol, val : " + symbol->GetDescription(vm)->ToString(vm);
    } else {
        return "type other : " + jsVal->ToString(vm)->ToString(vm);
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
    bool val1IsString = val1->IsString(vm);
    GTEST_LOG_(INFO) << "key1 : IsString:" << val1IsString << "    val:" << val1->ToString(vm)->ToString(vm);

    Local<JSValueRef> val2 = map->Get(vm, StringRef::NewFromUtf8(vm, "key2"));
    bool val2IsNumber = val2->IsNumber();
    GTEST_LOG_(INFO) << "key2 : IsNumber:" << val2IsNumber << "    val:" << val2->Int32Value(vm);

    Local<JSValueRef> val3 = map->Get(vm, StringRef::NewFromUtf8(vm, "key3"));
    bool val3IsBoolean = val3->IsBoolean();
    GTEST_LOG_(INFO) << "key3 : IsBoolean:" << val3IsBoolean << "    val:" << val3->BooleaValue(vm);

    Local<JSValueRef> val4 = map->Get(vm, StringRef::NewFromUtf8(vm, "key4"));
    bool val4IsSymbol = val4->IsSymbol(vm);
    Local<SymbolRef> val4Symbol = val4;
    GTEST_LOG_(INFO) << "key4 : IsSymbol:" << val4IsSymbol << "    val:"
                     << val4Symbol->GetDescription(vm)->ToString(vm);

    int num5 = 55; // random number
    Local<JSValueRef> val5 = map->Get(vm, IntegerRef::New(vm, num5));
    GTEST_LOG_(INFO) << "55 : " << val5->ToString(vm)->ToString(vm);

    int num6 = 66; // random number
    Local<JSValueRef> val6 = map->Get(vm, IntegerRef::New(vm, num6));
    GTEST_LOG_(INFO) << "66 : " << val6->Int32Value(vm);
    Local<JSValueRef> val7 = map->Get(vm, BooleanRef::New(vm, true));
    GTEST_LOG_(INFO) << "true : " << val7->ToString(vm)->ToString(vm);

    Local<JSValueRef> val8 = map->Get(vm, symbolKey);
    GTEST_LOG_(INFO) << "SymbolRef(key8)  : " << val8->ToString(vm)->ToString(vm);

    Local<JSValueRef> val82 = map->Get(vm, SymbolRef::New(vm, StringRef::NewFromUtf8(vm, "key8")));
    GTEST_LOG_(INFO) << "new SymbolRef(key8) is Undefined : " << val82->IsUndefined();

    int32_t size = map->GetSize(vm);
    GTEST_LOG_(INFO) << "size : " << size;
    int32_t totalElement = map->GetTotalElements(vm);
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
    GTEST_LOG_(INFO) << "Map Iterator kind : " << kind->ToString(vm);

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

    int size = weakMap->GetSize(vm_);
    GTEST_LOG_(INFO) << "size : " << size;
    int32_t totalElements = weakMap->GetTotalElements(vm_);
    GTEST_LOG_(INFO) << "total elements : " << totalElements;

    for (int i = 0; i < size; ++i) {
        Local<JSValueRef> jsKey = weakMap->GetKey(vm_, i);
        Local<JSValueRef> jsVal = weakMap->GetValue(vm_, i);
        GTEST_LOG_(INFO) << "for WeakMap index : " << i << "    key : " << jsValue2String(vm_, jsKey) << "    val : " <<
            jsValue2String(vm_, jsVal);
    }

    bool hasKey2 = weakMap->Has(vm_, StringRef::NewFromUtf8(vm_, "key2"));
    GTEST_LOG_(INFO) << "Has Key2 : " << hasKey2;
    bool hasKey222 = weakMap->Has(vm_, StringRef::NewFromUtf8(vm_, "key222"));
    GTEST_LOG_(INFO) << "Has key222 : " << hasKey222;

    GTEST_LOG_(INFO) << "sample_demo9_map_test_2_WeakMapref =======================================";
}

/* demo10 set 的使用 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo10_set)
{
    GTEST_LOG_(INFO) << "sample_demo10_set =======================================";
    LocalScope scope(vm_);
    Local<SetRef> set = SetRef::New(vm_);

    set->Add(vm_, StringRef::NewFromUtf8(vm_, "val1"));
    int num2 = 222; // random number
    set->Add(vm_, NumberRef::New(vm_, num2));
    set->Add(vm_, BooleanRef::New(vm_, true));
    set->Add(vm_, SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "val4")));
    set->Add(vm_, StringRef::NewFromUtf8(vm_, "val5"));
    int num6 = 666; // random number
    set->Add(vm_, IntegerRef::New(vm_, num6));

    int32_t size = set->GetSize(vm_);
    GTEST_LOG_(INFO) << "size : " << size;
    int32_t totalElement = set->GetTotalElements(vm_);
    GTEST_LOG_(INFO) << "total element : " << totalElement;

    for (int i = 0; i < size; ++i) {
        Local<JSValueRef> jsVal = set->GetValue(vm_, i);
        GTEST_LOG_(INFO) << "for set index : " << i << "    val : " << jsValue2String(vm_, jsVal);
    }

    // Iterator
    Local<SetIteratorRef> setIter = SetIteratorRef::New(vm_, set);
    ecmascript::EcmaRuntimeCallInfo *info = setIter->GetEcmaRuntimeCallInfo(vm_);

    Local<StringRef> kind = setIter->GetKind(vm_);
    GTEST_LOG_(INFO) << "Set Iterator kind : " << kind->ToString(vm_);

    for (Local<ArrayRef> array = SetIteratorRef::Next(vm_, info); array->IsArray(vm_);
        array = SetIteratorRef::Next(vm_, info)) {
        int index = setIter->GetIndex() - 1;
        Local<JSValueRef> jsVal = ArrayRef::GetValueAt(vm_, array, 1);
        GTEST_LOG_(INFO) << "for set iterator index : " << index << "    val : " << jsValue2String(vm_, jsVal);
    }
    GTEST_LOG_(INFO) << "sample_demo10_set =======================================";
}

/* demo10 WeakSet 的使用 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo10_weakset)
{
    GTEST_LOG_(INFO) << "sample_demo10_weakset =======================================";
    LocalScope scope(vm_);
    Local<WeakSetRef> weakSet = WeakSetRef::New(vm_);

    weakSet->Add(vm_, StringRef::NewFromUtf8(vm_, "val1"));
    int num2 = 666; // random number
    weakSet->Add(vm_, NumberRef::New(vm_, num2));
    weakSet->Add(vm_, BooleanRef::New(vm_, true));
    weakSet->Add(vm_, SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "val4")));
    weakSet->Add(vm_, StringRef::NewFromUtf8(vm_, "val5"));
    int num6 = 666; // random number
    weakSet->Add(vm_, IntegerRef::New(vm_, num6));

    int32_t size = weakSet->GetSize(vm_);
    GTEST_LOG_(INFO) << "size : " << size;
    int32_t totalElement = weakSet->GetTotalElements(vm_);
    GTEST_LOG_(INFO) << "total element : " << totalElement;

    for (int i = 0; i < size; ++i) {
        Local<JSValueRef> jsVal = weakSet->GetValue(vm_, i);
        GTEST_LOG_(INFO) << "for weakset index : " << i << "    val : " << jsValue2String(vm_, jsVal);
    }
    GTEST_LOG_(INFO) << "sample_demo10_weakset =======================================";
}

// json对象获取值。
void JsonObjGetValue(EcmaVM *vm, Local<ObjectRef> obj)
{
    Local<JSValueRef> jsVal1 = obj->Get(vm, StringRef::NewFromUtf8(vm, "str1"));
    bool jsVal1IsString = jsVal1->IsString(vm);
    Local<StringRef> val1 = jsVal1->ToString(vm);
    GTEST_LOG_(INFO) << "str1 : is string : " << jsVal1IsString << "  value : " << val1->ToString(vm);
    Local<JSValueRef> jsVal2 = obj->Get(vm, StringRef::NewFromUtf8(vm, "str2"));
    bool jsVal2IsNumber = jsVal2->IsNumber();
    int val2 = jsVal2->Int32Value(vm);
    GTEST_LOG_(INFO) << "str2 : is number : " << jsVal2IsNumber << "  value : " << val2;
    Local<JSValueRef> jsVal3 = obj->Get(vm, StringRef::NewFromUtf8(vm, "333"));
    int val3 = jsVal3->Int32Value(vm);
    GTEST_LOG_(INFO) << "333 : " << val3;
    Local<JSValueRef> jsVal4 = obj->Get(vm, StringRef::NewFromUtf8(vm, "444"));
    Local<StringRef> val4 = jsVal4->ToString(vm);
    GTEST_LOG_(INFO) << "str4 : " << val4->ToString(vm);

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
        GTEST_LOG_(INFO) << "arr6 : " << i << "    " << val6Item->ToString(vm)->ToString(vm);
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
        GTEST_LOG_(INFO) << "data7 :   item index:" << i << "    Key:" << itemKey->ToString(vm)->ToString(vm) <<
            "    val:" << itemVal->ToString(vm)->ToString(vm);
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
    GTEST_LOG_(INFO) << "jsStr is String : " << jsStr->IsString(vm_);
    Local<StringRef> strRef = jsStr->ToString(vm_);
    std::string str = strRef->ToString(vm_);
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
        std::string str = arr2Item->ToString(vm_)->ToString(vm_);
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
        int b = arr3Item->BooleaValue(vm_);
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

        GTEST_LOG_(INFO) << "arr4 index : " << i << "  key1 : " << objVal1->ToString(vm_)->ToString(vm_);
        GTEST_LOG_(INFO) << "arr4 index : " << i << "  key2 : " << objVal2->ToString(vm_)->ToString(vm_);
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
        if (arr5Item->IsString(vm_)) {
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  value : " << arr5Item->ToString(vm_)->ToString(vm_);
        } else if (arr5Item->IsNumber()) {
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  value : " << arr5Item->Int32Value(vm_);
        } else if (arr5Item->IsBoolean()) {
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  value : " << arr5Item->ToBoolean(vm_)->Value();
        } else if (arr5Item->IsObject(vm_)) {
            Local<ObjectRef> obj = arr5Item->ToObject(vm_);
            Local<ObjectRef> val1 = obj->Get(vm_, StringRef::NewFromUtf8(vm_, "key1"));
            Local<ObjectRef> val2 = obj->Get(vm_, StringRef::NewFromUtf8(vm_, "key2"));
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  key1 : " << val1->ToString(vm_)->ToString(vm_);
            GTEST_LOG_(INFO) << "arr5 index : " << i << "  key2 : " << val2->ToString(vm_)->ToString(vm_);
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
    GTEST_LOG_(INFO) << " js arr 1 json : " << json1->ToString(vm_)->ToString(vm_);
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
    GTEST_LOG_(INFO) << " js arr 2 json : " << json2->ToString(vm_)->ToString(vm_);
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
    GTEST_LOG_(INFO) << " js arr 3 json : " << json3->ToString(vm_)->ToString(vm_);
    GTEST_LOG_(INFO) << "sample_demo11_json_test_4_stringify_array3 =======================================";
}

/* demo12 异常的抛出和处理 */
// 抛出异常的测试函数
Local<JSValueRef> ThrowErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:Error");
        Local<JSValueRef> error = Exception::Error(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowRangeErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:RangeError");
        Local<JSValueRef> error = Exception::RangeError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowReferenceErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:ReferenceError");
        Local<JSValueRef> error = Exception::ReferenceError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowSyntaxErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:SyntaxError");
        Local<JSValueRef> error = Exception::SyntaxError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowTypeErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:TypeError");
        Local<JSValueRef> error = Exception::TypeError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowAggregateErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:AggregateError");
        Local<JSValueRef> error = Exception::AggregateError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowEvalErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:EvalError");
        Local<JSValueRef> error = Exception::EvalError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowOOMErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:OOMError");
        Local<JSValueRef> error = Exception::OOMError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

Local<JSValueRef> ThrowTerminationErrorFuncTest(const EcmaVM *vm, Local<NumberRef> par)
{
    int i = par->Int32Value(vm);
    if (i == 0) {
        std::string errStr = std::string("function:").append(__func__).append("err:TerminationError");
        Local<JSValueRef> error = Exception::TerminationError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }
    int num = 2; // 函数的功能，原数字的2倍。
    return NumberRef::New(vm, i * num);
}

// 处理异常，用 JSNApi 类中的函数。
void ClearAndPrintException4JSNApi(const EcmaVM *vm, const std::string log)
{
    if (!JSNApi::HasPendingException(vm)) {
        GTEST_LOG_(INFO) << log << " no exception .";
        return;
    }

    JSNApi::PrintExceptionInfo(vm);
    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(vm);
    std::string exceptionName = exception->Get(vm, StringRef::NewFromUtf8(vm, "name"))->ToString(vm)->ToString(vm);
    std::string exceptionMessage = exception->Get(vm, StringRef::NewFromUtf8(vm,
        "message"))->ToString(vm)->ToString(vm);
    std::string exceptionStack = exception->Get(vm,
        StringRef::NewFromUtf8(vm, "stack"))->ToString(vm)->ToString(vm);

    GTEST_LOG_(INFO) << log << " exception name : " << exceptionName;
    GTEST_LOG_(INFO) << log << " exception message : " << exceptionMessage;
    GTEST_LOG_(INFO) << log << " exception stack : " << exceptionStack;
};

// 处理异常，用 TryCatch 类中的函数。
void ClearAndPrintException4TryCatch1(const EcmaVM *vm, const std::string log)
{
    TryCatch tryCatch(vm);
    if (!tryCatch.HasCaught()) {
        GTEST_LOG_(INFO) << log << " no exception .";
        return;
    }
    Local<ObjectRef> exception = tryCatch.GetAndClearException();
    std::string exceptionName = exception->Get(vm, StringRef::NewFromUtf8(vm, "name"))->ToString(vm)->ToString(vm);
    std::string exceptionMessage = exception->Get(vm,
        StringRef::NewFromUtf8(vm, "message"))->ToString(vm)->ToString(vm);
    std::string exceptionStack = exception->Get(vm, StringRef::NewFromUtf8(vm, "stack"))->ToString(vm)->ToString(vm);

    GTEST_LOG_(INFO) << log << " exception name : " << exceptionName;
    GTEST_LOG_(INFO) << log << " exception message : " << exceptionMessage;
    GTEST_LOG_(INFO) << log << " exception stack : " << exceptionStack;
};

// 处理异常，用 TryCatch 类中的函数。
void ClearAndPrintException4TryCatch2(const EcmaVM *vm, const std::string log)
{
    TryCatch tryCatch(vm);
    if (!tryCatch.HasCaught()) {
        GTEST_LOG_(INFO) << log << " no exception .";
        return;
    }

    Local<ObjectRef> exception = tryCatch.GetException();
    tryCatch.ClearException();
    std::string exceptionName = exception->Get(vm, StringRef::NewFromUtf8(vm, "name"))->ToString(vm)->ToString(vm);
    std::string exceptionMessage = exception->Get(vm,
        StringRef::NewFromUtf8(vm, "message"))->ToString(vm)->ToString(vm);
    std::string exceptionStack = exception->Get(vm, StringRef::NewFromUtf8(vm, "stack"))->ToString(vm)->ToString(vm);

    GTEST_LOG_(INFO) << log << " exception name : " << exceptionName;
    GTEST_LOG_(INFO) << log << " exception message : " << exceptionMessage;
    GTEST_LOG_(INFO) << log << " exception stack : " << exceptionStack;
    tryCatch.Rethrow();
};

// 异常没有处理重新抛出异常，用 TryCatch 类中的函数。
void PrintAndRethrowException4TryCatch3(const EcmaVM *vm, const std::string log)
{
    TryCatch tryCatch(vm);
    if (!tryCatch.HasCaught()) {
        GTEST_LOG_(INFO) << log << " no exception .";
        return;
    }

    Local<ObjectRef> exception = tryCatch.GetAndClearException();
    std::string exceptionName = exception->Get(vm, StringRef::NewFromUtf8(vm, "name"))->ToString(vm)->ToString(vm);
    std::string exceptionMessage = exception->Get(vm,
        StringRef::NewFromUtf8(vm, "message"))->ToString(vm)->ToString(vm);
    std::string exceptionStack = exception->Get(vm, StringRef::NewFromUtf8(vm, "stack"))->ToString(vm)->ToString(vm);

    GTEST_LOG_(INFO) << log << " exception name : " << exceptionName;
    GTEST_LOG_(INFO) << log << " exception message : " << exceptionMessage;
    GTEST_LOG_(INFO) << log << " exception stack : " << exceptionStack;
    tryCatch.Rethrow();
};

/* demo12 异常的抛出和处理 */
HWTEST_F_L0(JSNApiSampleTest, sample_demo12_exception_test)
{
    GTEST_LOG_(INFO) << "sample_demo12_exception_test =======================================";
    LocalScope scope(vm_);
    // Error
    Local<JSValueRef> exception1 = ThrowErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception1->IsUndefined()) {
        ClearAndPrintException4JSNApi(vm_, "ThrowErrorFuncTest");
    }
    // RangeError
    Local<JSValueRef> exception2 = ThrowRangeErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception2->IsUndefined()) {
        ClearAndPrintException4TryCatch1(vm_, "ThrowRangeErrorFuncTest");
    }
    // ReferenceError
    Local<JSValueRef> exception3 = ThrowReferenceErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception3->IsUndefined()) {
        ClearAndPrintException4TryCatch2(vm_, "ThrowReferenceErrorFuncTest");
    }
    // SyntaxError
    Local<JSValueRef> exception4 = ThrowSyntaxErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception4->IsUndefined()) {
        PrintAndRethrowException4TryCatch3(vm_, "ThrowSyntaxErrorFuncTest");
        ClearAndPrintException4TryCatch1(vm_, "ThrowSyntaxErrorFuncTest");
    }
    // TypeError
    Local<JSValueRef> exception5 = ThrowTypeErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception5->IsUndefined()) {
        ClearAndPrintException4TryCatch1(vm_, "ThrowTypeErrorFuncTest");
    }
    // AggregateError
    Local<JSValueRef> exception6 = ThrowAggregateErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception6->IsUndefined()) {
        ClearAndPrintException4TryCatch1(vm_, "ThrowAggregateErrorFuncTest");
    }
    // EvalError
    Local<JSValueRef> exception7 = ThrowEvalErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception7->IsUndefined()) {
        ClearAndPrintException4TryCatch1(vm_, "ThrowEvalErrorFuncTest");
    }
    // OOMError
    Local<JSValueRef> exception8 = ThrowOOMErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception8->IsUndefined()) {
        ClearAndPrintException4TryCatch1(vm_, "ThrowOOMErrorFuncTest");
    }
    // TerminationError
    Local<JSValueRef> exception9 = ThrowTerminationErrorFuncTest(vm_, NumberRef::New(vm_, 0));
    if (exception9->IsUndefined()) {
        ClearAndPrintException4TryCatch1(vm_, "ThrowTerminationErrorFuncTest");
    }
    GTEST_LOG_(INFO) << "sample_demo12_exception_test =======================================";
}
}