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

#include "ecmascript/base/config.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSHClassTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(JSHClassTest, InitializeClass)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    // Call NewEcmaHClass function set object properties
    JSHandle<JSHClass> objectClass =
        factory->NewEcmaHClass(TaggedArray::SIZE, JSType::TAGGED_ARRAY, nullHandle);
    // Get object properties
    EXPECT_EQ(objectClass->GetLayout(thread), JSTaggedValue::Null());
    EXPECT_EQ(objectClass->GetPrototype(thread), JSTaggedValue::Null());
    EXPECT_EQ(objectClass->GetObjectType(), JSType::TAGGED_ARRAY);
    EXPECT_TRUE(objectClass->IsExtensible());
    EXPECT_TRUE(!objectClass->IsPrototype());
    EXPECT_EQ(objectClass->GetTransitions(thread), JSTaggedValue::Undefined());
    EXPECT_EQ(objectClass->GetProtoChangeMarker(thread), JSTaggedValue::Null());
    EXPECT_EQ(objectClass->GetProtoChangeDetails(thread), JSTaggedValue::Null());
    EXPECT_EQ(objectClass->GetEnumCache(thread), JSTaggedValue::Null());
}

HWTEST_F_L0(JSHClassTest, SizeFromJSHClass)
{
    if (g_isEnableCMCGC) {
        return;
    }
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> objectClass = factory->NewEcmaHClass(TaggedArray::SIZE, JSType::TAGGED_ARRAY, nullHandle);
    EXPECT_TRUE(*objectClass != nullptr);
    size_t objectSize;
    size_t length = 10;
    auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), 10);
    TaggedObject *header = heap->AllocateYoungOrHugeObject(*objectClass, size);
    JSHandle<TaggedArray> array(thread, header);
    array->SetLength(length);
    for (int i = 0; i < length; ++i) {
        array->Set(thread, i, JSTaggedValue::Null());
    }
    objectSize = array->GetClass()->SizeFromJSHClass(header);
    EXPECT_EQ(objectSize, 96U);
    EcmaString *string = EcmaStringAccessor::CreateEmptyString(vm);
    objectSize = string->GetSize();
    EXPECT_EQ(objectSize, 16U);
    string = factory->AllocTreeStringObject();
    objectSize = string->GetClass()->SizeFromJSHClass(string);
    EXPECT_EQ(objectSize, 32U);
    MachineCodeDesc desc;
    desc.codeType = MachineCodeType::BASELINE_CODE;
    desc.instructionsSize = 100;
    TaggedObject *machineCode = factory->NewMachineCodeObject(100, desc);
    MachineCode *code = reinterpret_cast<MachineCode *>(machineCode);
    code->SetPayLoadSizeInBytes(0);
    objectSize = code->GetClass()->SizeFromJSHClass(code);
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    EXPECT_EQ(objectSize, 344U);
#else
    EXPECT_EQ(objectSize, 208U);
#endif
    // size is an integral multiple of eight
    objectClass = factory->NewEcmaHClass(JSObject::SIZE - 1, JSType::JS_OBJECT, nullHandle);
    TaggedObject *header3 = heap->AllocateYoungOrHugeObject(*objectClass, length);
    objectSize = objectClass->SizeFromJSHClass(header3);
    EXPECT_EQ(objectSize, 56U);

    objectClass = factory->NewEcmaHClass(JSObject::SIZE + 1, JSType::JS_OBJECT, nullHandle);
    TaggedObject *header4 = heap->AllocateYoungOrHugeObject(*objectClass, length);
    objectSize = objectClass->SizeFromJSHClass(header4);
    EXPECT_EQ(objectSize, 64U);

    objectClass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);
    TaggedObject *header5 = heap->AllocateYoungOrHugeObject(*objectClass, length);
    objectSize = objectClass->SizeFromJSHClass(header5);
    EXPECT_EQ(objectSize, 64U);
}

HWTEST_F_L0(JSHClassTest, HasReferenceField)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> obj1Class = JSHandle<JSHClass>::Cast(thread->GlobalConstants()->GetHandledLineStringClass());
    JSHandle<JSHClass> obj2Class = JSHandle<JSHClass>::Cast(thread->GlobalConstants()->GetHandledTreeStringClass());
    JSHandle<JSHClass> obj3Class =
        factory->NewEcmaHClass(TaggedArray::SIZE, JSType::JS_NATIVE_POINTER, nullHandle);
    JSHandle<JSHClass> obj4Class = factory->NewEcmaHClass(TaggedArray::SIZE, JSType::JS_OBJECT, nullHandle);
    JSHandle<JSHClass> obj5Class = JSHandle<JSHClass>::Cast(thread->GlobalConstants()->GetHandledSlicedStringClass());
    EXPECT_FALSE(obj1Class->HasReferenceField());
    EXPECT_TRUE(obj2Class->HasReferenceField());
    EXPECT_FALSE(obj3Class->HasReferenceField());
    EXPECT_TRUE(obj4Class->HasReferenceField());
    EXPECT_TRUE(obj5Class->HasReferenceField());
}

HWTEST_F_L0(JSHClassTest, Clone)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> objectClass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);
    // withInlinedProperties is false
    JSHandle<JSHClass> cloneClass = JSHClass::Clone(thread, objectClass, false);
    EXPECT_TRUE(*cloneClass != nullptr);
    EXPECT_TRUE(objectClass->GetObjectSize() == cloneClass->GetObjectSize());
    EXPECT_EQ(cloneClass->GetObjectSize(), 64U); // 64 : 64 not missing the size of inlinedproperties
    EXPECT_TRUE(objectClass->GetLayout(thread) == cloneClass->GetLayout(thread));
    EXPECT_EQ(JSTaggedValue::SameValue(thread, objectClass->GetPrototype(thread), cloneClass->GetPrototype(thread)),
              true);
    EXPECT_TRUE(objectClass->GetBitField() == cloneClass->GetBitField());
    EXPECT_TRUE(objectClass->GetBitField1() == cloneClass->GetBitField1());
    EXPECT_TRUE(objectClass->NumberOfProps() == cloneClass->NumberOfProps());
    EXPECT_EQ(cloneClass->GetNextInlinedPropsIndex(), 0); // 0 : 0 mean index
    // withInlinedProperties is true
    cloneClass = JSHClass::Clone(thread, objectClass, true, 0);
    EXPECT_TRUE(*cloneClass != nullptr);
    EXPECT_TRUE(objectClass->GetObjectSize() > cloneClass->GetObjectSize());
    EXPECT_EQ(cloneClass->GetObjectSize(), 32U); // 32 : 32 missing the size of inlinedproperties
    EXPECT_TRUE(objectClass->GetLayout(thread) == cloneClass->GetLayout(thread));
    EXPECT_EQ(JSTaggedValue::SameValue(thread, objectClass->GetPrototype(thread), cloneClass->GetPrototype(thread)),
              true);
    EXPECT_TRUE(objectClass->GetBitField() == cloneClass->GetBitField());
    EXPECT_TRUE(objectClass->GetBitField1() > cloneClass->GetBitField1());
    EXPECT_TRUE(objectClass->NumberOfProps() == cloneClass->NumberOfProps());
    EXPECT_EQ(cloneClass->GetNextNonInlinedPropsIndex(), 0); // 0 : 0 mean index
}

HWTEST_F_L0(JSHClassTest, TransitionElementsToDictionary)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> jsObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSHClass> objectClass(thread, jsObject->GetJSHClass());
    JSHandle<JSTaggedValue> objKey1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> objKey2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> objKey3(factory->NewFromASCII("key3"));

    JSHandle<JSTaggedValue> objValue1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> objValue2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> objValue3(thread, JSTaggedValue(3));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), objKey1, objValue1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), objKey2, objValue2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), objKey3, objValue3);
    // class is not dictionary mode
    EXPECT_FALSE(jsObject->GetJSHClass()->IsDictionaryMode());
    ElementsKind oldKind = jsObject->GetJSHClass()->GetElementsKind();
    JSHClass::TransitionElementsToDictionary(thread, jsObject);
    JSObject::TryMigrateToGenericKindForJSObject(thread, jsObject, oldKind);
    auto resultDict = NameDictionary::Cast(jsObject->GetProperties(thread).GetTaggedObject());
    EXPECT_TRUE(resultDict != nullptr);
    EXPECT_EQ(resultDict->EntriesCount(), 3); // 3 : 3 entry

    JSHandle<JSHClass> dictionaryClass(thread, jsObject->GetJSHClass());
    EXPECT_TRUE(dictionaryClass->IsDictionaryMode());
    EXPECT_EQ(dictionaryClass->GetObjectSize() + 32U, objectClass->GetObjectSize());
    EXPECT_TRUE(dictionaryClass->IsDictionaryElement());
    EXPECT_FALSE(dictionaryClass->IsStableElements());
}

static JSHandle<JSHClass> CreateJSHClass(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFuncPrototype = env->GetObjectFunctionPrototype();
    JSHandle<JSHClass> hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, objectFuncPrototype);
    return hclass;
}

HWTEST_F_L0(JSHClassTest, SetPropertyOfObjHClass_001)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> accessorData(factory->NewAccessorData());
    JSHandle<EcmaString> keyHandle = factory->NewFromASCII("key");
    JSHandle<JSTaggedValue> keyHandle0(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> keyHandle2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> keyHandle4(factory->NewFromASCII("key4"));
    // empty layoutInfo
    JSHandle<JSHClass> parentsClass = CreateJSHClass(thread);

    uint32_t length = 6;
    JSHandle<TaggedArray> properties = factory->NewTaggedArray(length);
    for (int i = 0; i < static_cast<int>(length); i++) {
        if (i % 2 == 0) {
            JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(i));
            JSHandle<EcmaString> newKey =
                factory->ConcatFromString(keyHandle, JSTaggedValue::ToString(thread, newValue));
            properties->Set(thread, i, newKey.GetTaggedValue());
            continue;
        }
        properties->Set(thread, i, accessorData.GetTaggedValue());
    }
    JSHandle<JSHClass> childClass = factory->SetLayoutInObjHClass(properties, 3, parentsClass);
    JSHandle<JSObject> childObj = factory->NewJSObject(childClass);

    std::vector<JSTaggedValue> keyVector;
    JSObject::GetAllKeysForSerialization(thread, childObj, keyVector);
    EXPECT_EQ(keyVector.size(), 3U);
    EXPECT_TRUE(JSObject::HasProperty(thread, childObj, keyHandle0));
    EXPECT_TRUE(JSObject::HasProperty(thread, childObj, keyHandle2));
    EXPECT_TRUE(JSObject::HasProperty(thread, childObj, keyHandle4));
}

HWTEST_F_L0(JSHClassTest, SetPropertyOfObjHClass_002)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSHClass> objClass = CreateJSHClass(thread);
    JSHandle<JSObject> Obj1 = factory->NewJSObject(objClass);
    JSHandle<JSObject> Obj2 = factory->NewJSObject(objClass);
    PropertyAttributes attr = PropertyAttributes::Default();

    JSHandle<JSTaggedValue> keyE(factory->NewFromASCII("e"));
    JSHandle<JSTaggedValue> keyF(factory->NewFromASCII("f"));
    // not empty layoutInfo
    auto valueE = JSHandle<JSTaggedValue>(thread, JSTaggedValue(7));
    auto valueF = JSHandle<JSTaggedValue>(thread, JSTaggedValue(8));
    JSObject::SetProperty(thread, Obj1, keyE, valueE);
    JSObject::SetProperty(thread, Obj2, keyF, valueE);

    auto repE = PropertyAttributes::TranslateToRep(valueE.GetTaggedValue());
    JSHandle<JSHClass> propertyHclass = JSHClass::SetPropertyOfObjHClass(thread, objClass, keyE, attr, repE);
    JSHandle<JSHClass> obj1Class(thread, Obj1->GetClass());
    EXPECT_TRUE(propertyHclass == obj1Class);

    auto repF = PropertyAttributes::TranslateToRep(valueF.GetTaggedValue());
    propertyHclass = JSHClass::SetPropertyOfObjHClass(thread, objClass, keyF, attr, repF);
    JSHandle<JSHClass> obj2Class(thread, Obj2->GetClass());
    EXPECT_TRUE(propertyHclass == obj2Class);
}

HWTEST_F_L0(JSHClassTest, AddProperty)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<EcmaString> keyHandle = factory->NewFromASCII("key");
    JSHandle<JSTaggedValue> keyHandle0(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> keyHandle1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> keyHandle2(factory->NewFromASCII("key2"));
    PropertyAttributes attr = PropertyAttributes::Default();
    attr.SetIsInlinedProps(true);
    // empty layoutInfo
    JSHandle<JSHClass> objClass1 = CreateJSHClass(thread);
    JSHandle<JSObject> Obj = factory->NewJSObject(objClass1);
    JSHandle<JSHClass> objClass(thread, Obj->GetClass());
    EXPECT_FALSE(objClass1 != objClass);
    int keyLength = 3;
    for (int i = 0; i <keyLength; i++) {
        JSHandle<JSTaggedValue> keyValue(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> keyHandleI(
            factory->ConcatFromString(keyHandle, JSTaggedValue::ToString(thread, keyValue)));
        attr.SetOffset(i);
        ElementsKind oldKind = Obj->GetJSHClass()->GetElementsKind();
        JSHClass::AddProperty(thread, Obj, keyHandleI, attr);
        JSObject::TryMigrateToGenericKindForJSObject(thread, Obj, oldKind);
    }
    EXPECT_TRUE(objClass1 == objClass);
    std::vector<JSTaggedValue> keyVector;
    JSObject::GetAllKeysForSerialization(thread, Obj, keyVector);
    EXPECT_EQ(keyVector.size(), 3U);
    EXPECT_TRUE(JSObject::HasProperty(thread, Obj, keyHandle0));
    EXPECT_TRUE(JSObject::HasProperty(thread, Obj, keyHandle1));
    EXPECT_TRUE(JSObject::HasProperty(thread, Obj, keyHandle2));
}

HWTEST_F_L0(JSHClassTest, TransitionExtension)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> preExtensionsKey = thread->GlobalConstants()->GetHandledPreventExtensionsString();
    JSHandle<JSTaggedValue> keyHandle0(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> keyHandle1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> keyHandle2(factory->NewFromASCII("key2"));
    PropertyAttributes attr = PropertyAttributes(0);
    attr.SetIsInlinedProps(true);
    JSHandle<JSHClass> obj1Class = CreateJSHClass(thread);
    JSHandle<JSHClass> obj2Class = CreateJSHClass(thread);
    obj2Class->SetExtensible(true);
    JSHandle<JSObject> Obj1 = factory->NewJSObject(obj1Class);
    JSHandle<JSObject> Obj2 = factory->NewJSObject(obj2Class);
    JSObject::SetProperty(thread, Obj2, keyHandle0, JSHandle<JSTaggedValue>(thread, JSTaggedValue(7)));
    JSObject::SetProperty(thread, Obj2, keyHandle1, JSHandle<JSTaggedValue>(thread, JSTaggedValue(8)));
    JSObject::SetProperty(thread, Obj2, keyHandle2, JSHandle<JSTaggedValue>(thread, JSTaggedValue(9)));
    // obj has key "PreventExtensions"
    ElementsKind oldKind = Obj1->GetJSHClass()->GetElementsKind();
    JSHClass::AddProperty(thread, Obj1, preExtensionsKey, attr);
    JSObject::TryMigrateToGenericKindForJSObject(thread, Obj1, oldKind);
    JSHandle<JSHClass> newClass1 = JSHClass::TransitionExtension(thread, obj1Class);
    JSHandle<JSHClass> objClass(thread, Obj1->GetClass());
    EXPECT_TRUE(newClass1 == objClass);
    // obj has no key "PreventExtensions"
    JSHandle<JSHClass> newClass2 = JSHClass::TransitionExtension(thread, obj2Class);
    EXPECT_FALSE(newClass2->IsExtensible());
    JSHandle<TransitionsDictionary> dictionary(thread, obj2Class->GetTransitions(thread));
    // find key
    std::vector<JSTaggedValue> keyVector;
    dictionary->GetAllKeysIntoVector(thread, keyVector);
    EXPECT_TRUE((keyVector[0] == keyHandle0.GetTaggedValue()) || (keyVector[0] == preExtensionsKey.GetTaggedValue()));
    EXPECT_TRUE((keyVector[1] == keyHandle0.GetTaggedValue()) || (keyVector[1] == preExtensionsKey.GetTaggedValue()));
}

HWTEST_F_L0(JSHClassTest, TransitionProto)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env =vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcPrototype = env->GetFunctionPrototype();
    JSHandle<JSTaggedValue> prototypeKey = thread->GlobalConstants()->GetHandledPrototypeString();
    JSHandle<JSTaggedValue> obj1Key(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> obj2Key(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> obj3Key(factory->NewFromASCII("key2"));
    PropertyAttributes attr = PropertyAttributes(0);
    attr.SetIsInlinedProps(true);
    JSHandle<JSHClass> objClass = CreateJSHClass(thread);
    JSHandle<JSObject> Obj = factory->NewJSObject(objClass);
    // obj has no key "prototype"
    ElementsKind oldKind = Obj->GetJSHClass()->GetElementsKind();
    JSHClass::AddProperty(thread, Obj, obj1Key, attr);
    JSObject::TryMigrateToGenericKindForJSObject(thread, Obj, oldKind);
    oldKind = Obj->GetJSHClass()->GetElementsKind();
    JSHClass::AddProperty(thread, Obj, obj2Key, attr);
    JSObject::TryMigrateToGenericKindForJSObject(thread, Obj, oldKind);
    oldKind = Obj->GetJSHClass()->GetElementsKind();
    JSHClass::AddProperty(thread, Obj, obj3Key, attr);
    JSObject::TryMigrateToGenericKindForJSObject(thread, Obj, oldKind);
    JSHandle<JSHClass> newClass = JSHClass::TransitionProto(thread, objClass, funcPrototype);
    EXPECT_EQ(newClass->GetPrototype(thread), funcPrototype.GetTaggedValue());
    JSHandle<TransitionsDictionary> transitionDictionary(thread, objClass->GetTransitions(thread));
    // find key
    std::vector<JSTaggedValue> keyVector;
    transitionDictionary->GetAllKeysIntoVector(thread, keyVector);
    EXPECT_EQ(keyVector.size(), 2U);
    EXPECT_EQ(keyVector[0], obj1Key.GetTaggedValue());
    EXPECT_EQ(keyVector[1], prototypeKey.GetTaggedValue());
}

HWTEST_F_L0(JSHClassTest, TransitionToDictionary)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> obj1Key(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> obj2Key(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> obj3Key(factory->NewFromASCII("key3"));
    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> objClass = CreateJSHClass(thread);
    objClass->SetIsPrototype(true);
    JSHandle<JSObject> Obj0 = factory->NewJSObject(objClass);
    JSHandle<JSHClass> obj0Class1(thread, Obj0->GetJSHClass());
    JSHandle<JSObject> Obj1 = JSObject::ObjectCreate(thread, nullHandle);
    JSHandle<JSObject> Obj2 = JSObject::ObjectCreate(thread, Obj1);
    JSHandle<JSHClass> obj2Class(thread, Obj2->GetJSHClass());
    JSHandle<JSObject> Obj3 = JSObject::ObjectCreate(thread, Obj2);
    JSObject::SetProperty(thread, Obj1, obj1Key, JSHandle<JSTaggedValue>(thread, JSTaggedValue(100)));
    JSObject::SetProperty(thread, Obj2, obj2Key, JSHandle<JSTaggedValue>(thread, JSTaggedValue(101)));
    JSObject::SetProperty(thread, Obj3, obj3Key, JSHandle<JSTaggedValue>(thread, JSTaggedValue(102)));
    // empty object
    JSObject::TransitionToDictionary(thread, Obj0);
    JSHandle<JSHClass> obj0Class(thread, Obj0->GetClass());
    EXPECT_TRUE(obj0Class->GetObjectSize() < obj0Class1->GetObjectSize());
    EXPECT_EQ(obj0Class->NumberOfProps(), 0U);
    EXPECT_TRUE(obj0Class->IsDictionaryMode());
    EXPECT_TRUE(obj0Class->IsPrototype());
    // not empty object
    JSHandle<JSHClass> obj3Class(thread, Obj3->GetJSHClass());
    JSHClass::EnableProtoChangeMarker(thread, obj3Class);
    JSObject::TransitionToDictionary(thread, Obj2);
    // refresh users
    JSHandle<JSHClass> obj1Class(thread, Obj1->GetJSHClass());
    JSTaggedValue protoDetails = obj1Class->GetProtoChangeDetails(thread);
    EXPECT_TRUE(protoDetails.IsProtoChangeDetails());
    JSTaggedValue listenersValue = ProtoChangeDetails::Cast(protoDetails.GetTaggedObject())->GetChangeListener(thread);
    JSHandle<ChangeListener> listeners(thread, listenersValue.GetTaggedObject());
    uint32_t holeIndex = ChangeListener::CheckHole(thread, listeners);
    EXPECT_TRUE(holeIndex == 0U);
    // new class
    JSHandle<JSHClass> newClass(thread, Obj2->GetClass());
    EXPECT_TRUE(newClass->GetObjectSize() < obj2Class->GetObjectSize());
    EXPECT_EQ(newClass->NumberOfProps(), 0U);
    EXPECT_TRUE(newClass->IsDictionaryMode());
    EXPECT_TRUE(newClass->IsPrototype());
}

HWTEST_F_L0(JSHClassTest, UpdatePropertyMetaData)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> objKey(factory->NewFromASCII("key0"));
    PropertyAttributes oldAttr = PropertyAttributes(0);
    PropertyAttributes newAttr = PropertyAttributes(1);
    oldAttr.SetIsInlinedProps(true);
    JSHandle<JSHClass> objClass = CreateJSHClass(thread);
    JSHandle<JSObject> Obj = factory->NewJSObject(objClass);
    // Set Transitions
    ElementsKind oldKind = Obj->GetJSHClass()->GetElementsKind();
    JSHClass::AddProperty(thread, Obj, objKey, oldAttr);
    JSObject::TryMigrateToGenericKindForJSObject(thread, Obj, oldKind);
    // update metaData
    objClass->UpdatePropertyMetaData(thread, objKey.GetTaggedValue(), newAttr);
    LayoutInfo *layoutInfo = LayoutInfo::Cast(objClass->GetLayout(thread).GetTaggedObject());
    EXPECT_EQ(layoutInfo->GetAttr(thread, oldAttr.GetOffset()).GetPropertyMetaData(), newAttr.GetPropertyMetaData());
}

HWTEST_F_L0(JSHClassTest, SetPrototype)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env =vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSTaggedValue> objectFuncPrototype = env->GetObjectFunctionPrototype();

    JSHandle<JSHClass> objectClass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);
    EXPECT_EQ(objectClass->GetPrototype(thread), nullHandle.GetTaggedValue());
    objectClass->SetPrototype(thread, objectFuncPrototype);
    EXPECT_EQ(objectClass->GetPrototype(thread), objectFuncPrototype.GetTaggedValue());
}

HWTEST_F_L0(JSHClassTest, ProxyHClassClone)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> objectFn(thread->GetGlobalEnv()->GetObjectFunction());
    JSHandle<JSObject> target = factory->NewJSObjectByConstructor(objectFn, JSHandle<JSTaggedValue>(objectFn));
    JSHandle<JSObject> handler = factory->NewJSObjectByConstructor(objectFn, JSHandle<JSTaggedValue>(objectFn));

    JSHandle<JSProxy> proxy = JSProxy::ProxyCreate(thread,
        JSHandle<JSTaggedValue>(target), JSHandle<JSTaggedValue>(handler));

    JSHandle<JSHClass> hclass(thread, proxy->GetClass());
    EXPECT_FALSE(hclass->IsJSObject());
    EXPECT_GT(hclass->GetObjectSize(), 0);
    EXPECT_EQ(hclass->GetInlinedProperties(), 0);

    JSHandle<JSHClass> newHClass = JSHClass::Clone(thread, hclass);
    EXPECT_EQ(newHClass->GetObjectType(), hclass->GetObjectType());
    EXPECT_EQ(newHClass->GetObjectSize(), hclass->GetObjectSize());
    EXPECT_EQ(newHClass->GetInlinedPropsStartSize(), hclass->GetInlinedPropsStartSize());
    EXPECT_EQ(newHClass->GetInlinedProperties(), hclass->GetInlinedProperties());
    EXPECT_EQ(newHClass->GetLayout(thread), hclass->GetLayout(thread));
}
}  // namespace panda::test
