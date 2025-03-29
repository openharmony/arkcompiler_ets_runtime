/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_OBJECT_FACTORY_H
#define ECMASCRIPT_OBJECT_FACTORY_H

#include "ecmascript/base/error_type.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/enum_cache.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/shared_objects/js_shared_object.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/byte_array.h"
#include "ecmascript/pgo_profiler/pgo_extra_profiler.h"

namespace panda::ecmascript {
struct MethodLiteral;
class Method;
class JSObject;
class JSArray;
class JSAPIPlainArray;
class JSSymbol;
class JSFunctionBase;
class JSFunction;
class JSBoundFunction;
class JSProxyRevocFunction;
class JSAsyncAwaitStatusFunction;
class JSPrimitiveRef;
class GlobalEnv;
class GlobalEnvConstants;
class AccessorData;
class JSGlobalObject;
class LexicalEnv;
class SendableEnv;
class JSDate;
class JSProxy;
class JSRealm;
class JSArguments;
class TaggedQueue;
class JSForInIterator;
class JSSet;
class JSSharedSet;
class JSMap;
class JSSharedMap;
class JSRegExp;
class JSSetIterator;
class JSSharedSetIterator;
class JSRegExpIterator;
class JSMapIterator;
class JSSharedMapIterator;
class JSArrayIterator;
class JSSharedArrayIterator;
class JSAPIPlainArrayIterator;
class JSStringIterator;
class JSGeneratorObject;
class CompletionRecord;
class GeneratorContext;
class JSArrayBuffer;
class JSSendableArrayBuffer;
class JSDataView;
class JSPromise;
class JSPromiseReactionsFunction;
class JSPromiseExecutorFunction;
class JSAsyncModuleFulfilledFunction;
class JSAsyncModuleRejectedFunction;
class JSPromiseAllResolveElementFunction;
class JSAsyncGeneratorResNextRetProRstFtn;
class JSPromiseAnyRejectElementFunction;
class JSPromiseAllSettledElementFunction;
class JSPromiseFinallyFunction;
class JSPromiseValueThunkOrThrowerFunction;
class PromiseReaction;
class PromiseCapability;
class PromiseIteratorRecord;
class JSAsyncFuncObject;
class JSAsyncFunction;
class JSAsyncFromSyncIterUnwarpFunction;
class PromiseRecord;
class JSLocale;
class ResolvingFunctionsRecord;
class EcmaVM;
class Heap;
class SharedHeap;
class ConstantPool;
class Program;
class LayoutInfo;
class JSIntlBoundFunction;
class FreeObject;
class JSNativePointer;
class JSAPIArrayList;
class JSAPIArrayListIterator;
class JSAPIDeque;
class JSAPIDequeIterator;
class TaggedHashArray;
class LinkedNode;
class RBTreeNode;
class JSAPIHashMap;
class JSAPIHashSet;
class JSAPIHashMapIterator;
class JSAPIHashSetIterator;
class JSAPILightWeightMap;
class JSAPILightWeightMapIterator;
class JSAPILightWeightSet;
class JSAPILightWeightSetIterator;
class JSAPIQueue;
class JSAPIQueueIterator;
class JSAPIStack;
class JSAPIStackIterator;
class JSAPITreeSet;
class JSAPITreeMap;
class JSAPITreeSetIterator;
class JSAPITreeMapIterator;
class JSAPIVector;
class JSAPIVectorIterator;
class JSAPIBitVector;
class JSAPIBitVectorIterator;
class JSAPILinkedList;
class JSAPIList;
class JSAPILinkedListIterator;
class JSAPIListIterator;
class ModuleNamespace;
class NativeModuleFailureInfo;
class ImportEntry;
class LocalExportEntry;
class IndirectExportEntry;
class StarExportEntry;
class SourceTextModule;
class CjsModule;
class CjsRequire;
class CjsExports;
class ResolvedBinding;
class ResolvedIndexBinding;
class ResolvedRecordIndexBinding;
class ResolvedRecordBinding;
class BigInt;
class AsyncGeneratorRequest;
class AsyncIteratorRecord;
class JSAsyncGeneratorFunction;
class JSAsyncGeneratorObject;
class CellRecord;
class ClassLiteral;

namespace job {
class MicroJobQueue;
class PendingJob;
}  // namespace job
class TransitionHandler;
class PrototypeHandler;
class TransWithProtoHandler;
class StoreAOTHandler;
class EnumCache;
class PropertyBox;
class ProtoChangeMarker;
class ProtoChangeDetails;
class MarkerCell;
class ProfileTypeInfo;
class MachineCode;
class ClassInfoExtractor;
class AOTLiteralInfo;
class ProfileTypeInfoCell;
class FunctionTemplate;
class VTable;
namespace kungfu {
class TSHClassGenerator;
}  // namespace kungfu

enum class CompletionRecordType : uint8_t;
enum class PrimitiveType : uint8_t;
enum class IterationKind : uint8_t;
enum class MethodIndex : uint8_t;
enum class SharedTypes : uint8_t;
using ErrorType = base::ErrorType;
using base::ErrorType;
using ExtraProfileTypeInfo = pgo::ExtraProfileTypeInfo;

enum class RemoveSlots { YES, NO };
enum class GrowMode { KEEP, GROW };
enum class StackCheck { YES, NO };

class ObjectFactory {
public:
    static constexpr JSTaggedType FREE_MEMMORY_ADDRESS_ZAM_VALUE = 0xDEADFACE;
    ObjectFactory(JSThread *thread, Heap *heap, SharedHeap *sHeap);
    ~ObjectFactory() = default;
    JSHandle<Method> NewMethodForNativeFunction(const void *func, FunctionKind kind = FunctionKind::NORMAL_FUNCTION,
                                                kungfu::BuiltinsStubCSigns::ID builtinId =
                                                BUILTINS_STUB_ID(INVALID),
                                                MemSpaceType methodSpaceType = SHARED_OLD_SPACE);

    JSHandle<ProfileTypeInfo> NewProfileTypeInfo(uint32_t length);
    JSHandle<ConstantPool> NewConstantPool(uint32_t capacity);
    JSHandle<Program> NewProgram();

    JSHandle<JSObject> PUBLIC_API GetJSError(const ErrorType &errorType, const char *data = nullptr,
        StackCheck needCheckStack = StackCheck::YES);

    JSHandle<JSObject> NewJSError(const ErrorType &errorType, const JSHandle<EcmaString> &message,
        StackCheck needCheckStack = StackCheck::YES);

    JSHandle<JSObject> NewJSAggregateError();

    JSHandle<TransitionHandler> NewTransitionHandler();

    JSHandle<PrototypeHandler> NewPrototypeHandler();

    JSHandle<TransWithProtoHandler> NewTransWithProtoHandler();

    JSHandle<StoreAOTHandler> NewStoreAOTHandler();

    JSHandle<JSObject> NewEmptyJSObject(uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);

    JSHandle<JSHClass> GetHClassByFunctionKind(const JSHandle<GlobalEnv> &env, FunctionKind kind);

    // use for others create, prototype is Function.prototype
    // use for native function
    JSHandle<JSFunction> NewJSFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc = nullptr,
                                       FunctionKind kind = FunctionKind::NORMAL_FUNCTION,
                                       kungfu::BuiltinsStubCSigns::ID builtinId = BUILTINS_STUB_ID(INVALID),
                                       MemSpaceType methodSpaceType = SHARED_OLD_SPACE);
    JSHandle<JSFunction> NewSFunction(const JSHandle<GlobalEnv> &env,
                                      const void *nativeFunc = nullptr,
                                      FunctionKind kind = FunctionKind::NORMAL_FUNCTION,
                                      kungfu::BuiltinsStubCSigns::ID builtinId = BUILTINS_STUB_ID(INVALID),
                                      MemSpaceType spaceType = SHARED_OLD_SPACE);
    void InitializeMethod(const MethodLiteral *methodLiteral, JSHandle<Method> &method);
    // use for method
    JSHandle<JSFunction> NewJSFunction(const JSHandle<GlobalEnv> &env, const JSHandle<Method> &method);

    JSHandle<JSFunction> NewJSFunction(const JSHandle<Method> &methodHandle);

    JSHandle<JSFunction> NewJSFunction(const JSHandle<Method> &methodHandle,
                                       const JSHandle<JSTaggedValue> &homeObject);

    JSHandle<JSFunction> NewJSNativeErrorFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc = nullptr);

    JSHandle<JSFunction> NewSpecificTypedArrayFunction(const JSHandle<GlobalEnv> &env,
                                                       const void *nativeFunc = nullptr,
                                                       kungfu::BuiltinsStubCSigns::ID builtinId =
                                                       BUILTINS_STUB_ID(INVALID));

    JSHandle<JSObject> OrdinaryNewJSObjectCreate(const JSHandle<JSTaggedValue> &proto);

    JSHandle<JSObject> CreateNapiObject();
    JSHandle<JSObject> CreateNullJSObject();

    JSHandle<JSFunction> NewAotFunction(uint32_t numArgs, uintptr_t codeEntry);

    JSHandle<JSBoundFunction> NewJSBoundFunction(const JSHandle<JSTaggedValue> &target,
                                                 const JSHandle<JSTaggedValue> &boundThis,
                                                 const JSHandle<TaggedArray> &args);

    JSHandle<JSIntlBoundFunction> NewJSIntlBoundFunction(MethodIndex idx, int functionLength = 1);

    JSHandle<JSProxyRevocFunction> NewJSProxyRevocFunction(const JSHandle<JSProxy> &proxy);

    JSHandle<JSAsyncAwaitStatusFunction> NewJSAsyncAwaitStatusFunction(MethodIndex idx);

    JSHandle<JSGeneratorObject> NewJSGeneratorObject(JSHandle<JSTaggedValue> generatorFunction);

    JSHandle<JSAsyncFuncObject> NewJSAsyncFuncObject();
    JSHandle<JSAsyncGeneratorObject> NewJSAsyncGeneratorObject(JSHandle<JSTaggedValue> generatorFunction);

    JSHandle<JSPrimitiveRef> NewJSPrimitiveRef(const JSHandle<JSFunction> &function,
                                               const JSHandle<JSTaggedValue> &object);
    JSHandle<JSPrimitiveRef> NewJSPrimitiveRef(PrimitiveType type, const JSHandle<JSTaggedValue> &object);

    // get JSHClass for Ecma ClassLinker
    JSHandle<GlobalEnv> NewGlobalEnv(JSHClass *globalEnvClass);

    // get JSHClass for Ecma ClassLinker
    JSHandle<LexicalEnv> NewLexicalEnv(int numSlots);

    inline LexicalEnv *InlineNewLexicalEnv(int numSlots);

    JSHandle<JSSymbol> NewEmptySymbol();

    JSHandle<JSSymbol> NewJSSymbol();

    JSHandle<JSSymbol> NewPrivateSymbol();

    JSHandle<JSSymbol> NewPrivateNameSymbol(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewWellKnownSymbol(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewPublicSymbol(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewSymbolWithTable(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewPrivateNameSymbolWithChar(std::string_view description);

    JSHandle<JSSymbol> NewWellKnownSymbolWithChar(std::string_view description);

    JSHandle<JSSymbol> NewPublicSymbolWithChar(std::string_view description);

    JSHandle<JSSymbol> NewSymbolWithTableWithChar(std::string_view description);

    JSHandle<AccessorData> NewAccessorData();
    JSHandle<AccessorData> NewInternalAccessor(void *setter, void *getter);

    JSHandle<PromiseCapability> NewPromiseCapability();

    JSHandle<JSPromise> NewJSPromise();

    JSHandle<PromiseReaction> NewPromiseReaction();

    JSHandle<PromiseRecord> NewPromiseRecord();
    JSHandle<AsyncGeneratorRequest> NewAsyncGeneratorRequest();

    JSHandle<AsyncIteratorRecord> NewAsyncIteratorRecord(const JSHandle<JSTaggedValue> &itor,
                                                         const JSHandle<JSTaggedValue> &next, bool done);

    JSHandle<ResolvingFunctionsRecord> NewResolvingFunctionsRecord();

    JSHandle<PromiseIteratorRecord> NewPromiseIteratorRecord(const JSHandle<JSTaggedValue> &itor, bool done);

    JSHandle<job::MicroJobQueue> NewMicroJobQueue();

    JSHandle<job::PendingJob> NewPendingJob(const JSHandle<JSFunction> &func, const JSHandle<TaggedArray> &argv);

    JSHandle<JSArray> NewJSArray();
    JSHandle<JSSharedArray> PUBLIC_API NewJSSArray();
    JSHandle<JSArray> PUBLIC_API NewJSArray(size_t length, JSHandle<JSHClass> &hclass);
    JSHandle<TaggedArray> PUBLIC_API NewJsonFixedArray(size_t start, size_t length,
                                                       const std::vector<JSHandle<JSTaggedValue>> &vec);
    JSHandle<TaggedArray> PUBLIC_API NewSJsonFixedArray(size_t start, size_t length,
                                                        const std::vector<JSHandle<JSTaggedValue>> &vec);

    JSHandle<JSProxy> NewJSProxy(const JSHandle<JSTaggedValue> &target, const JSHandle<JSTaggedValue> &handler);
    JSHandle<JSRealm> NewJSRealm();

    JSHandle<JSArguments> NewJSArguments();

    JSHandle<JSPrimitiveRef> NewJSString(const JSHandle<JSTaggedValue> &str, const JSHandle<JSTaggedValue> &newTarget);

    template <typename Derived>
    JSHandle<TaggedArray> ConvertListToArray(const JSThread *thread, const JSHandle<Derived> &list,
                                             uint32_t numberOfNodes)
    {
        MemSpaceType spaceType = numberOfNodes < LENGTH_THRESHOLD ? MemSpaceType::SEMI_SPACE : MemSpaceType::OLD_SPACE;
        JSHandle<TaggedArray> dstElements = NewTaggedArrayWithoutInit(numberOfNodes, spaceType);
        if (numberOfNodes == 0) {
            return dstElements;
        }
        int dataIndex = Derived::ELEMENTS_START_INDEX;
        for (uint32_t i = 0; i < numberOfNodes; i++) {
            dataIndex = list->GetElement(dataIndex + Derived::NEXT_PTR_OFFSET).GetInt();
            dstElements->Set(thread, i, list->GetElement(dataIndex));
        }
        return dstElements;
    }

    JSHandle<JSObject> NewAndCopyJSArrayObject(JSHandle<JSObject> thisObjHandle, uint32_t newLength,
                                               uint32_t oldLength, uint32_t k = 0);
    JSHandle<TaggedArray> NewAndCopyTaggedArray(JSHandle<TaggedArray> &srcElements, uint32_t newLength,
                                                uint32_t oldLength, uint32_t k = 0);
    JSHandle<TaggedArray> NewAndCopyTaggedArraySkipBarrier(JSHandle<TaggedArray> &srcElements, uint32_t newLength,
                                                           uint32_t oldLength, uint32_t k = 0);
    JSHandle<TaggedArray> NewAndCopySNameDictionary(JSHandle<TaggedArray> &srcElements, uint32_t length);
    JSHandle<TaggedArray> NewAndCopyTaggedArrayByObject(JSHandle<JSObject> thisObjHandle, uint32_t newLength,
                                                        uint32_t oldLength, uint32_t k = 0);
    JSHandle<MutantTaggedArray> NewAndCopyMutantTaggedArrayByObject(JSHandle<JSObject> thisObjHandle,
                                                                    uint32_t newLength, uint32_t oldLength,
                                                                    uint32_t k = 0);
    JSHandle<TaggedArray> PUBLIC_API NewTaggedArray(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<TaggedArray> NewTaggedArray(uint32_t length, JSTaggedValue initVal, bool nonMovable);
    JSHandle<TaggedArray> NewTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType);
    // Copy on write array is allocated in nonmovable space by default.
    JSHandle<COWTaggedArray> NewCOWTaggedArray(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<COWMutantTaggedArray> NewCOWMutantTaggedArray(uint32_t length, JSTaggedType initVal = base::SPECIAL_HOLE);
    JSHandle<MutantTaggedArray> NewMutantTaggedArray(uint32_t length, JSTaggedType initVal = base::SPECIAL_HOLE);
    JSHandle<TaggedArray> PUBLIC_API NewDictionaryArray(uint32_t length);
    JSHandle<JSForInIterator> NewJSForinIterator(const JSHandle<JSTaggedValue> &obj,
                                                 const JSHandle<JSTaggedValue> keys,
                                                 const JSHandle<JSTaggedValue> cachedHclass,
                                                 const uint32_t enumCacheKind);

    JSHandle<ByteArray> NewByteArray(uint32_t length, uint32_t size, void *srcData = nullptr,
                                     MemSpaceType spaceType = MemSpaceType::SEMI_SPACE);

    JSHandle<PropertyBox> NewPropertyBox(const JSHandle<JSTaggedValue> &name);
    
    JSHandle<EnumCache> NewEnumCache();

    JSHandle<ProtoChangeMarker> NewProtoChangeMarker();

    JSHandle<ProtoChangeDetails> NewProtoChangeDetails();

    JSHandle<MarkerCell> NewMarkerCell();
    JSHandle<BigInt> NewBigInt(uint32_t length);
    // use for copy properties keys's array to another array
    JSHandle<TaggedArray> ExtendArray(const JSHandle<TaggedArray> &old, uint32_t length,
                                      JSTaggedValue initVal = JSTaggedValue::Hole(),
                                      MemSpaceType type = MemSpaceType::SEMI_SPACE,
                                      ElementsKind kind = ElementsKind::GENERIC);
    JSHandle<TaggedArray> CopyPartArray(const JSHandle<TaggedArray> &old, uint32_t start, uint32_t end);
    JSHandle<TaggedArray> PUBLIC_API CopyArray(const JSHandle<TaggedArray> &old, uint32_t oldLength, uint32_t newLength,
                                               JSTaggedValue initVal = JSTaggedValue::Hole(),
                                               MemSpaceType type = MemSpaceType::SEMI_SPACE,
                                               ElementsKind kind = ElementsKind::GENERIC);
    JSHandle<TaggedArray> CopyFromKeyArray(const JSHandle<TaggedArray> &old);
    JSHandle<TaggedArray> CloneProperties(const JSHandle<TaggedArray> &old);
    JSHandle<TaggedArray> CloneProperties(const JSHandle<TaggedArray> &old, const JSHandle<JSTaggedValue> &env,
                                          const JSHandle<JSObject> &obj);

    JSHandle<LayoutInfo> PUBLIC_API CreateLayoutInfo(int properties, MemSpaceType type = MemSpaceType::SEMI_SPACE,
                                                     GrowMode mode = GrowMode::GROW);

    JSHandle<LayoutInfo> ExtendLayoutInfo(const JSHandle<LayoutInfo> &old, int properties);

    JSHandle<LayoutInfo> CopyLayoutInfo(const JSHandle<LayoutInfo> &old);

    JSHandle<LayoutInfo> CopyAndReSort(const JSHandle<LayoutInfo> &old, int end, int capacity);

    JSHandle<EcmaString> GetEmptyString() const;

    JSHandle<TaggedArray> EmptyArray() const;

    JSHandle<MutantTaggedArray> EmptyMutantArray() const;

    FreeObject *FillFreeObject(uintptr_t address, size_t size, RemoveSlots removeSlots = RemoveSlots::NO,
                               uintptr_t hugeObjectHead = 0);

    TaggedObject *NewObject(const JSHandle<JSHClass> &hclass);

    TaggedObject *NewNonMovableObject(const JSHandle<JSHClass> &hclass, uint32_t inobjPropCount = 0);

    void InitializeExtraProperties(const JSHandle<JSHClass> &hclass, TaggedObject *obj, uint32_t inobjPropCount);

    JSHandle<TaggedQueue> NewTaggedQueue(uint32_t length);

    JSHandle<TaggedQueue> GetEmptyTaggedQueue() const;

    JSHandle<JSSetIterator> NewJSSetIterator(const JSHandle<JSSet> &set, IterationKind kind);
    JSHandle<JSSharedSetIterator> NewJSSetIterator(const JSHandle<JSSharedSet> &set, IterationKind kind);

    JSHandle<JSRegExpIterator> NewJSRegExpIterator(const JSHandle<JSTaggedValue> &matcher,
                                                   const JSHandle<EcmaString> &inputStr, bool global,
                                                   bool fullUnicode);

    JSHandle<JSMapIterator> NewJSMapIterator(const JSHandle<JSMap> &map, IterationKind kind);

    JSHandle<JSSharedMapIterator> NewJSMapIterator(const JSHandle<JSSharedMap> &map, IterationKind kind);

    JSHandle<JSArrayIterator> NewJSArrayIterator(const JSHandle<JSObject> &array, IterationKind kind);
    JSHandle<JSSharedArrayIterator> NewJSSharedArrayIterator(const JSHandle<JSObject> &array, IterationKind kind);

    JSHandle<CompletionRecord> NewCompletionRecord(CompletionRecordType type, JSHandle<JSTaggedValue> value);

    JSHandle<GeneratorContext> NewGeneratorContext();

    JSHandle<JSPromiseReactionsFunction> CreateJSPromiseReactionsFunction(MethodIndex idx);

    JSHandle<JSPromiseExecutorFunction> CreateJSPromiseExecutorFunction();

    JSHandle<JSAsyncModuleFulfilledFunction> CreateJSAsyncModuleFulfilledFunction();

    JSHandle<JSAsyncModuleRejectedFunction> CreateJSAsyncModuleRejectedFunction();

    JSHandle<JSPromiseAllResolveElementFunction> NewJSPromiseAllResolveElementFunction();

    JSHandle<JSPromiseAnyRejectElementFunction> NewJSPromiseAnyRejectElementFunction();

    JSHandle<JSPromiseAllSettledElementFunction> NewJSPromiseAllSettledResolveElementFunction();

    JSHandle<JSPromiseAllSettledElementFunction> NewJSPromiseAllSettledRejectElementFunction();

    JSHandle<JSPromiseFinallyFunction> NewJSPromiseThenFinallyFunction();

    JSHandle<JSPromiseFinallyFunction> NewJSPromiseCatchFinallyFunction();

    JSHandle<JSPromiseValueThunkOrThrowerFunction> NewJSPromiseValueThunkFunction();

    JSHandle<JSPromiseValueThunkOrThrowerFunction> NewJSPromiseThrowerFunction();

    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> NewJSAsyGenResNextRetProRstFulfilledFtn();

    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> NewJSAsyGenResNextRetProRstRejectedFtn();

    JSHandle<JSAsyncFromSyncIterUnwarpFunction> NewJSAsyncFromSyncIterUnwarpFunction();

    JSHandle<JSObject> CloneObjectLiteral(JSHandle<JSObject> object, const JSHandle<JSTaggedValue> &env,
                                          bool canShareHClass = true);
    JSHandle<JSObject> CloneObjectLiteral(JSHandle<JSObject> object);
    JSHandle<JSArray> CloneArrayLiteral(JSHandle<JSArray> object);
    JSHandle<JSFunction> CloneJSFunction(JSHandle<JSFunction> func);
    JSHandle<JSFunction> CloneSFunction(JSHandle<JSFunction> func);
    JSHandle<JSFunction> CloneClassCtor(JSHandle<JSFunction> ctor, const JSHandle<JSTaggedValue> &lexenv,
                                        bool canShareHClass);
    JSHandle<JSFunction> CreateJSFunctionFromTemplate(JSHandle<FunctionTemplate> funcTemp);
    JSHandle<JSFunction> CreateSFunctionFromTemplate(JSHandle<FunctionTemplate> funcTemp);

    void NewJSArrayBufferData(const JSHandle<JSArrayBuffer> &array, int32_t length);
    void NewJSSendableArrayBufferData(const JSHandle<JSSendableArrayBuffer> &array, int32_t length);
    JSHandle<JSSendableArrayBuffer> NewJSSendableArrayBuffer(int32_t length);
    JSHandle<JSSendableArrayBuffer> NewJSSendableArrayBuffer(void *buffer, int32_t length,
                                                             const NativePointerCallback &deleter, void *data);

    JSHandle<JSArrayBuffer> NewJSArrayBuffer(int32_t length);

    JSHandle<JSArrayBuffer> NewJSArrayBuffer(void *buffer, int32_t length, const NativePointerCallback &deleter,
                                             void *data, bool share = false);

    JSHandle<JSDataView> NewJSDataView(JSHandle<JSArrayBuffer> buffer, uint32_t offset, uint32_t length);

    void NewJSSharedArrayBufferData(const JSHandle<JSArrayBuffer> &array, int32_t length);

    JSHandle<JSArrayBuffer> NewJSSharedArrayBuffer(int32_t length);

    JSHandle<JSArrayBuffer> NewJSSharedArrayBuffer(void *buffer, int32_t length);

    void NewJSRegExpByteCodeData(const JSHandle<JSRegExp> &regexp, void *buffer, size_t size);

    template<typename T, typename S>
    inline void NewJSIntlIcuData(const JSHandle<T> &obj, const S &icu, const NativePointerCallback &callback);

    EcmaString *PUBLIC_API InternString(const JSHandle<JSTaggedValue> &key);

    inline JSHandle<JSNativePointer> NewJSNativePointer(void *externalPointer,
                                                        const NativePointerCallback &callBack = nullptr,
                                                        void *data = nullptr,
                                                        bool nonMovable = false,
                                                        size_t nativeBindingsize = 0,
                                                        Concurrent isConcurrent = Concurrent::NO,
                                                        NativeFlag flag = NativeFlag::NO_DIV);

    JSHandle<JSObject> NewOldSpaceObjLiteralByHClass(const JSHandle<JSHClass> &hclass);
    JSHandle<JSHClass> SetLayoutInObjHClass(const JSHandle<TaggedArray> &properties, size_t length,
                                            const JSHandle<JSHClass> &objClass);
    static bool CanObjectLiteralHClassCache(size_t length);
    JSHandle<JSHClass> CreateObjectLiteralRootHClass(size_t length);
    JSHandle<JSHClass> GetObjectLiteralRootHClass(size_t literalLength, size_t maxPropsNum);
    JSHandle<JSHClass> GetObjectLiteralHClass(const JSHandle<TaggedArray> &properties, size_t length);
    // only use for creating Function.prototype and Function
    JSHandle<JSFunction> NewJSFunctionByHClass(const JSHandle<Method> &method, const JSHandle<JSHClass> &clazz,
                                               MemSpaceType type = MemSpaceType::SEMI_SPACE);
    // for native function
    JSTaggedValue GetReadOnlyMethodForNativeFunction(FunctionKind kind);
    JSHandle<JSFunction> NewNativeFunctionByHClass(const JSHandle<JSHClass> &clazz,
                                                   const void *nativeFunc,
                                                   FunctionKind kind,
                                                   MemSpaceType type = MemSpaceType::SEMI_SPACE);
    JSHandle<JSFunction> NewJSFunctionByHClass(const void *func, const JSHandle<JSHClass> &clazz,
                                               FunctionKind kind = FunctionKind::NORMAL_FUNCTION);
    JSHandle<JSFunction> NewJSFunctionByHClassWithoutAccessor(const void *func,
        const JSHandle<JSHClass> &clazz, FunctionKind kind = FunctionKind::NORMAL_FUNCTION);

    JSHandle<Method> NewMethod(const MethodLiteral *methodLiteral, MemSpaceType spaceType = OLD_SPACE);

    JSHandle<Method> NewMethod(const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral,
                               JSHandle<ConstantPool> constpool, uint32_t entryIndex,
                               bool needSetAotFlag, bool *canFastCall = nullptr);

    // used for creating jsobject by constructor
    JSHandle<JSObject> PUBLIC_API NewJSObjectByConstructor(const JSHandle<JSFunction> &constructor,
                                                           const JSHandle<JSTaggedValue> &newTarget);
    JSHandle<JSObject> NewJSObjectByConstructor(JSHandle<GlobalEnv> env,
        const JSHandle<JSFunction> &constructor, uint32_t inlinedProps);
    JSHandle<JSObject> NewJSObjectByConstructor(const JSHandle<JSFunction> &constructor,
                                                uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);
    void InitializeJSObject(const JSHandle<JSObject> &obj, const JSHandle<JSHClass> &jshclass);

    JSHandle<JSObject> NewJSObjectWithInit(const JSHandle<JSHClass> &jshclass);
    uintptr_t NewSpaceBySnapshotAllocator(size_t size);
    TaggedObject *NewMachineCodeObject(size_t length, MachineCodeDesc &desc);
    JSHandle<MachineCode> SetMachineCodeObjectData(TaggedObject *obj, size_t length,
        const MachineCodeDesc &desc, JSHandle<Method> &method, RelocMap &relocInfo);
    JSHandle<MachineCode> NewMachineCodeObject(size_t length, const MachineCodeDesc &desc,
                                               JSHandle<Method> &method, RelocMap &relocInfo);
    JSHandle<ClassInfoExtractor> NewClassInfoExtractor(JSHandle<JSTaggedValue> method);
    JSHandle<ClassLiteral> NewClassLiteral();

    // ----------------------------------- new string ----------------------------------------
    JSHandle<EcmaString> PUBLIC_API NewFromASCII(std::string_view data);
    JSHandle<EcmaString> NewFromUtf8WithoutStringTable(std::string_view data);
    JSHandle<EcmaString> PUBLIC_API NewFromUtf8(std::string_view data);
    JSHandle<EcmaString> NewFromUtf8(std::string_view data, bool canBeCompress);
    JSHandle<EcmaString> NewFromUtf8ReadOnly(std::string_view data);
    JSHandle<EcmaString> NewFromASCIISkippingStringTable(std::string_view data);
    JSHandle<EcmaString> NewFromUtf16WithoutStringTable(std::u16string_view data);
    JSHandle<EcmaString> NewFromUtf16(std::u16string_view data);

    JSHandle<EcmaString> PUBLIC_API NewFromStdString(const std::string &data);

    JSHandle<EcmaString> NewFromUtf8WithoutStringTable(const uint8_t *utf8Data, uint32_t utf8Len);
    JSHandle<EcmaString> NewFromUtf8(const uint8_t *utf8Data, uint32_t utf8Len);

    JSHandle<EcmaString> NewFromUtf16WithoutStringTable(const uint16_t *utf16Data, uint32_t utf16Len);
    JSHandle<EcmaString> PUBLIC_API NewFromUtf16(const uint16_t *utf16Data, uint32_t utf16Len);
    JSHandle<EcmaString> NewFromUtf16Compress(const uint16_t *utf16Data, uint32_t utf16Len);
    JSHandle<EcmaString> NewFromUtf16NotCompress(const uint16_t *utf16Data, uint32_t utf16Len);

    JSHandle<EcmaString> NewFromUtf8Literal(const uint8_t *utf8Data, uint32_t utf8Len);
    JSHandle<EcmaString> PUBLIC_API NewFromUtf8LiteralCompress(const uint8_t *utf8Data, uint32_t utf8Len);
    JSHandle<EcmaString> PUBLIC_API NewFromUtf8LiteralCompressSubString(const JSHandle<EcmaString> &string,
                                                                        uint32_t offset, uint32_t utf8Len);
    JSHandle<EcmaString> PUBLIC_API NewCompressedUtf8(const uint8_t *utf8Data, uint32_t utf8Len);
    JSHandle<EcmaString> PUBLIC_API NewCompressedUtf8SubString(const JSHandle<EcmaString> &string,
                                                               uint32_t offset, uint32_t utf8Len);
    JSHandle<EcmaString> NewFromUtf16Literal(const uint16_t *utf16Data, uint32_t utf16Len);
    JSHandle<EcmaString> NewFromUtf16LiteralCompress(const uint16_t *utf16Data, uint32_t utf16Len);
    JSHandle<EcmaString> PUBLIC_API NewFromUtf16LiteralNotCompress(const uint16_t *utf16Data, uint32_t utf16Len);

    inline EcmaString *AllocLineStringObject(size_t size);
    inline EcmaString *AllocLineStringObjectNoGC(size_t size);
    inline EcmaString *AllocOldSpaceLineStringObject(size_t size);
    inline EcmaString *AllocReadOnlyLineStringObject(size_t size);
    inline EcmaString *AllocNonMovableLineStringObject(size_t size);
    inline EcmaString *AllocSlicedStringObject(MemSpaceType type);
    inline EcmaString *AllocTreeStringObject();

    JSHandle<EcmaString> ConcatFromString(const JSHandle<EcmaString> &firstString,
                                          const JSHandle<EcmaString> &secondString);

    // used for creating Function
    JSHandle<JSObject> PUBLIC_API NewJSObject(const JSHandle<JSHClass> &jshclass);

    // used for creating jshclass in Builtins, Function, Class_Linker
    JSHandle<JSHClass> NewEcmaHClass(uint32_t size, JSType type, const JSHandle<JSTaggedValue> &prototype);
    JSHandle<JSHClass> NewEcmaHClass(uint32_t size, uint32_t inlinedProps, JSType type,
                                     const JSHandle<JSTaggedValue> &prototype);

    // used for creating jshclass in Builtins, Function, Class_Linker
    JSHandle<JSHClass> PUBLIC_API NewEcmaHClass(uint32_t size, JSType type,
                                                uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);

    JSHandle<JSHClass> PUBLIC_API NewEcmaHClass(uint32_t size, uint32_t inlinedProps, JSType type,
        const JSHandle<JSTaggedValue> &prototype, const JSHandle<JSTaggedValue> &layout);

    // It is used to provide iterators for non ECMA standard jsapi containers.
    JSHandle<JSAPIPlainArray> NewJSAPIPlainArray(uint32_t capacity);
    JSHandle<JSAPIPlainArrayIterator> NewJSAPIPlainArrayIterator(const JSHandle<JSAPIPlainArray> &plainarray,
                                                                 IterationKind kind);
    JSHandle<JSAPIArrayList> NewJSAPIArrayList(uint32_t capacity);

    JSHandle<JSAPILightWeightMapIterator> NewJSAPILightWeightMapIterator(const JSHandle<JSAPILightWeightMap> &obj,
                                                                         IterationKind kind);
    JSHandle<JSAPILightWeightSetIterator> NewJSAPILightWeightSetIterator(const JSHandle<JSAPILightWeightSet> &obj,
                                                                         IterationKind kind);
    JSHandle<TaggedArray> CopyQueue(const JSHandle<TaggedArray> &old, uint32_t newLength,
                                    uint32_t front, uint32_t tail);
    JSHandle<JSAPIQueueIterator> NewJSAPIQueueIterator(const JSHandle<JSAPIQueue> &queue);
    JSHandle<TaggedArray> CopyDeque(const JSHandle<TaggedArray> &old, uint32_t newLength, uint32_t oldLength,
                                    uint32_t first, uint32_t last);
    JSHandle<JSAPIDequeIterator> NewJSAPIDequeIterator(const JSHandle<JSAPIDeque> &deque);
    JSHandle<JSAPIArrayListIterator> NewJSAPIArrayListIterator(const JSHandle<JSAPIArrayList> &arrayList);
    JSHandle<JSAPIList> NewJSAPIList();
    JSHandle<JSAPILinkedList> NewJSAPILinkedList();
    JSHandle<JSAPILinkedListIterator> NewJSAPILinkedListIterator(const JSHandle<JSAPILinkedList> &linkedList);
    JSHandle<JSAPIListIterator> NewJSAPIListIterator(const JSHandle<JSAPIList> &list);
    JSHandle<JSAPITreeMapIterator> NewJSAPITreeMapIterator(const JSHandle<JSAPITreeMap> &map, IterationKind kind);
    JSHandle<JSAPITreeSetIterator> NewJSAPITreeSetIterator(const JSHandle<JSAPITreeSet> &set, IterationKind kind);
    JSHandle<JSAPIStackIterator> NewJSAPIStackIterator(const JSHandle<JSAPIStack> &stack);
    JSHandle<JSAPIVector> NewJSAPIVector(uint32_t capacity);
    JSHandle<JSAPIVectorIterator> NewJSAPIVectorIterator(const JSHandle<JSAPIVector> &vector);
    JSHandle<JSAPIBitVector> NewJSAPIBitVector(uint32_t capacity);
    JSHandle<JSAPIBitVectorIterator> NewJSAPIBitVectorIterator(const JSHandle<JSAPIBitVector> &bitVector);
    JSHandle<JSAPIHashMapIterator> NewJSAPIHashMapIterator(const JSHandle<JSAPIHashMap> &hashMap, IterationKind kind);
    JSHandle<JSAPIHashSetIterator> NewJSAPIHashSetIterator(const JSHandle<JSAPIHashSet> &hashSet, IterationKind kind);
    JSHandle<TaggedHashArray> NewTaggedHashArray(uint32_t length);
    JSHandle<LinkedNode> NewLinkedNode(int hash, const JSHandle<JSTaggedValue> &key,
                                       const JSHandle<JSTaggedValue> &value,
                                       const JSHandle<LinkedNode> &next);
    JSHandle<RBTreeNode> NewTreeNode(int hash, const JSHandle<JSTaggedValue> &key,
                                     const JSHandle<JSTaggedValue> &value);
    // --------------------------------------module--------------------------------------------
    JSHandle<ModuleNamespace> NewModuleNamespace();
    JSHandle<NativeModuleFailureInfo> NewNativeModuleFailureInfo();
    JSHandle<ImportEntry> NewImportEntry();
    JSHandle<ImportEntry> NewImportEntry(const uint32_t moduleRequestIdx,
                                         const JSHandle<JSTaggedValue> &importName,
                                         const JSHandle<JSTaggedValue> &localName,
                                         SharedTypes sharedTypes);
    JSHandle<LocalExportEntry> NewLocalExportEntry();
    JSHandle<LocalExportEntry> NewLocalExportEntry(
        const JSHandle<JSTaggedValue> &exportName, const JSHandle<JSTaggedValue> &localName,
        const uint32_t index, SharedTypes sharedTypes);
    JSHandle<IndirectExportEntry> NewIndirectExportEntry();
    JSHandle<IndirectExportEntry> NewIndirectExportEntry(const JSHandle<JSTaggedValue> &exportName,
                                                         const uint32_t moduleRequestIdx,
                                                         const JSHandle<JSTaggedValue> &importName,
                                                         SharedTypes sharedTypes);
    JSHandle<StarExportEntry> NewStarExportEntry();
    JSHandle<StarExportEntry> NewStarExportEntry(const uint32_t moduleRequestIdx,
                                                 SharedTypes sharedTypes);
    JSHandle<SourceTextModule> NewSourceTextModule();
    JSHandle<ResolvedBinding> NewResolvedBindingRecord();
    JSHandle<ResolvedBinding> NewResolvedBindingRecord(const JSHandle<SourceTextModule> &module,
                                                       const JSHandle<JSTaggedValue> &bindingName);
    JSHandle<ResolvedIndexBinding> NewResolvedIndexBindingRecord();
    JSHandle<ResolvedIndexBinding> NewResolvedIndexBindingRecord(
        const JSHandle<SourceTextModule> &module, int32_t index);

    JSHandle<CellRecord> NewCellRecord();
    JSHandle<JSFunction> NewJSAsyncGeneratorFunction(const JSHandle<Method> &method);
    // --------------------------------------require--------------------------------------------
    JSHandle<CjsModule> NewCjsModule();
    JSHandle<CjsExports> NewCjsExports();
    JSHandle<CjsRequire> NewCjsRequire();

    JSHandle<JSHClass> CreateIteratorResultInstanceClass(const JSHandle<GlobalEnv> &env);

    // --------------------------------------old space object--------------------------------------------
    JSHandle<JSObject> NewOldSpaceJSObject(const JSHandle<JSHClass> &jshclass);
    TaggedObject *NewOldSpaceObject(const JSHandle<JSHClass> &hclass);
    JSHandle<TaggedArray> NewOldSpaceTaggedArray(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());

    // ---------------------------------New objects used internally--------------------------------------
    JSHandle<JSArray> NewJSStableArrayWithElements(const JSHandle<TaggedArray> &elements);

    // ---------------------------------------Used by AOT------------------------------------------------
    JSHandle<AOTLiteralInfo> NewAOTLiteralInfo(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<ExtraProfileTypeInfo> NewExtraProfileTypeInfo();
    JSHandle<ProfileTypeInfoCell> NewProfileTypeInfoCell(const JSHandle<JSTaggedValue> &value);
    JSHandle<FunctionTemplate> NewFunctionTemplate(
        const JSHandle<Method> &method, const JSHandle<JSTaggedValue> &module, int32_t length);
    JSHandle<VTable> NewVTable(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<JSHClass> NewEcmaHClass(JSHClass *hclass, uint32_t size, JSType type,
                                     uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);

    // napi interface to create object with initial inline properties
    JSHandle<JSTaggedValue> CreateJSObjectWithProperties(size_t propertyCount, const Local<JSValueRef> *keys,
                                                         const PropertyDescriptor *attributes);
    JSHandle<JSTaggedValue> CreateJSObjectWithNamedProperties(size_t propertyCount, const char **keys,
                                                              const Local<JSValueRef> *values);
    // Fill the given free memory range with special zam value.
    void FillFreeMemoryRange(uintptr_t start, uintptr_t end);

    // -----------------------------------shared object-----------------------------------------
    JSHandle<JSObject> PUBLIC_API NewSharedOldSpaceJSObject(const JSHandle<JSHClass> &jshclass);

    JSHandle<JSObject> PUBLIC_API NewSharedOldSpaceJSObjectWithInit(const JSHandle<JSHClass> &jshclass);

    TaggedObject *NewSharedOldSpaceObject(const JSHandle<JSHClass> &hclass);

    JSHandle<JSTaggedValue> CreateSObjectWithProperties(std::vector<PropertyDescriptor> &descs);

    JSHandle<JSHClass> PUBLIC_API NewSEcmaHClass(uint32_t size, JSType type, uint32_t inlinedProps);

    JSHandle<JSHClass> PUBLIC_API NewSEcmaHClass(JSHClass *hclass, uint32_t size, JSType type,
        uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);

    JSHandle<JSHClass> PUBLIC_API NewSEcmaHClass(uint32_t size, uint32_t inlinedProps, JSType type,
        const JSHandle<JSTaggedValue> &prototype, const JSHandle<JSTaggedValue> &layout);

    JSHandle<JSHClass> PUBLIC_API NewSEcmaHClassDictMode(uint32_t size, uint32_t inlinedProps, JSType type,
                                              const JSHandle<JSTaggedValue> &prototype);

    JSHandle<JSHClass> NewSEcmaHClassClass(JSHClass *hclass, uint32_t size, JSType type);

    JSHandle<JSHClass> InitSClassClass();

    JSHandle<JSHClass> NewSEcmaReadOnlyHClass(JSHClass *hclass, uint32_t size, JSType type,
                                             uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);

    JSHandle<TaggedArray> SharedEmptyArray() const;

    JSHandle<TaggedArray> CopySArray(const JSHandle<TaggedArray> &old, uint32_t oldLength, uint32_t newLength,
                                               JSTaggedValue initVal = JSTaggedValue::Hole(),
                                               ElementsKind kind = ElementsKind::GENERIC);

    JSHandle<TaggedArray> ExtendSArray(const JSHandle<TaggedArray> &old, uint32_t length,
                                       JSTaggedValue initVal, [[maybe_unused]] ElementsKind kind);

    JSHandle<Method> NewSMethodForNativeFunction(const void *func, FunctionKind kind = FunctionKind::NORMAL_FUNCTION,
                                                kungfu::BuiltinsStubCSigns::ID builtinId =
                                                BUILTINS_STUB_ID(INVALID),
                                                MemSpaceType spaceType = SHARED_OLD_SPACE);

    JSHandle<JSFunction> NewSFunctionByHClass(const JSHandle<Method> &methodHandle,
                                              const JSHandle<JSHClass> &hclass);
    // for native function
    JSHandle<JSFunction> NewNativeSFunctionByHClass(const JSHandle<JSHClass> &hclass,
                                                    const void *nativeFunc,
                                                    FunctionKind kind);

    JSHandle<JSFunction> NewSFunctionByHClass(const void *func, const JSHandle<JSHClass> &hclass,
        FunctionKind kind,
        kungfu::BuiltinsStubCSigns::ID builtinId = BUILTINS_STUB_ID(INVALID),
        MemSpaceType spaceType = SHARED_OLD_SPACE);

    JSHandle<JSFunction> NewSFunctionWithAccessor(
        const void *func,
        const JSHandle<JSHClass> &hclass,
        FunctionKind kind,
        kungfu::BuiltinsStubCSigns::ID builtinId = BUILTINS_STUB_ID(INVALID),
        MemSpaceType spaceType = SHARED_OLD_SPACE);

    JSHandle<Method> NewSMethod(const MethodLiteral *methodLiteral, MemSpaceType methodSpaceType = SHARED_OLD_SPACE);

    JSHandle<Method> PUBLIC_API NewSMethod(const JSPandaFile *jsPandaFile,
                                           MethodLiteral *methodLiteral,
                                           JSHandle<ConstantPool> constpool,
                                           uint32_t entryIndex,
                                           bool needSetAotFlag,
                                           bool *canFastCall = nullptr);

    JSHandle<ConstantPool> NewSConstantPool(uint32_t capacity);

    JSHandle<AOTLiteralInfo> NewSAOTLiteralInfo(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());

    JSHandle<COWTaggedArray> NewSCOWTaggedArray(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());

    JSHandle<ClassLiteral> NewSClassLiteral();

    JSHandle<ClassInfoExtractor> NewSClassInfoExtractor(JSHandle<JSTaggedValue> method);

    JSHandle<TaggedArray> NewSOldSpaceTaggedArray(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());

    JSHandle<TaggedArray> PUBLIC_API NewSTaggedArray(uint32_t length,
                                                     JSTaggedValue initVal = JSTaggedValue::Hole(),
                                                     MemSpaceType spaceType = SHARED_OLD_SPACE);

    JSHandle<AccessorData> NewSAccessorData();

    JSHandle<SourceTextModule> NewSSourceTextModule();

    JSHandle<ModuleNamespace> NewSModuleNamespace();

    JSHandle<ImportEntry> NewSImportEntry(const uint32_t moduleRequestIdx,
                                         const JSHandle<JSTaggedValue> &importName,
                                         const JSHandle<JSTaggedValue> &localName);

    JSHandle<LocalExportEntry> NewSLocalExportEntry(const JSHandle<JSTaggedValue> &exportName,
        const JSHandle<JSTaggedValue> &localName, const uint32_t index);

    JSHandle<IndirectExportEntry> NewSIndirectExportEntry(const JSHandle<JSTaggedValue> &exportName,
                                                          const uint32_t moduleRequestIdx,
                                                          const JSHandle<JSTaggedValue> &importName);

    JSHandle<StarExportEntry> NewSStarExportEntry(const uint32_t moduleRequestIdx);

    JSHandle<ResolvedIndexBinding> NewSResolvedIndexBindingRecord();

    JSHandle<ResolvedIndexBinding> NewSResolvedIndexBindingRecord(const JSHandle<SourceTextModule> &module,
        int32_t index);

    JSHandle<ResolvedBinding> NewSResolvedBindingRecord();

    JSHandle<ResolvedBinding> NewSResolvedBindingRecord(const JSHandle<SourceTextModule> &module,
        const JSHandle<JSTaggedValue> &bindingName);

    JSHandle<ResolvedRecordIndexBinding> NewSResolvedRecordIndexBindingRecord();

    JSHandle<ResolvedRecordIndexBinding> NewSResolvedRecordIndexBindingRecord(
        const JSHandle<EcmaString> &moduleRecord, const JSHandle<EcmaString> &abcFileName, int32_t index);

    JSHandle<ResolvedRecordBinding> NewSResolvedRecordBindingRecord();

    JSHandle<ResolvedRecordBinding> NewSResolvedRecordBindingRecord(
        const JSHandle<EcmaString> &moduleRecord, const JSHandle<JSTaggedValue> &bindingName);

    JSHandle<LayoutInfo> CopyAndReSortSLayoutInfo(const JSHandle<LayoutInfo> &old, int end, int capacity);

    JSHandle<LayoutInfo> PUBLIC_API CreateSLayoutInfo(uint32_t properties);

    JSHandle<LayoutInfo> PUBLIC_API NewSEmptyLayoutInfo();

    JSHandle<ProfileTypeInfoCell> NewSEmptyProfileTypeInfoCell();

    JSHandle<Method> NewSEmptyNativeFunctionMethod(FunctionKind kind);

    JSHandle<FunctionTemplate> NewSFunctionTemplate(
        const JSHandle<Method> &method, const JSHandle<JSTaggedValue> &module, int32_t length);

    JSHandle<TaggedArray> NewSEmptyArray(); // only used for EcmaVM.

    JSHandle<MutantTaggedArray> NewSEmptyMutantArray();

    JSHandle<TaggedArray> PUBLIC_API NewSDictionaryArray(uint32_t length);

    JSHandle<TaggedArray> NewSTaggedArrayWithoutInit(uint32_t length,
                                                     MemSpaceType spaceType = MemSpaceType::SHARED_OLD_SPACE);

    JSHandle<JSHClass> CreateSFunctionClass(uint32_t size, JSType type,
                                            const JSHandle<JSTaggedValue> &prototype,
                                            bool isAccessor = true, bool setProto = false);

    JSHandle<JSNativePointer> NewSJSNativePointer(void *externalPointer,
                                                  const NativePointerCallback &callBack,
                                                  void *data = nullptr,
                                                  bool nonMovable = false,
                                                  size_t nativeBindingsize = 0,
                                                  NativeFlag flag = NativeFlag::NO_DIV);

    JSHandle<JSNativePointer> NewSReadOnlyJSNativePointer(void *externalPointer);

    JSHandle<AccessorData> NewSInternalAccessor(void *setter, void *getter);

    JSHandle<JSSymbol> NewSWellKnownSymbol(const JSHandle<JSTaggedValue> &name);
    JSHandle<JSSymbol> NewSConstantPrivateSymbol();
    JSHandle<JSSymbol> NewSEmptySymbol();
    JSHandle<JSSymbol> NewSWellKnownSymbolWithChar(std::string_view description);
    JSHandle<JSSymbol> NewSPublicSymbolWithChar(std::string_view description);
    JSHandle<JSSymbol> NewSPublicSymbol(const JSHandle<JSTaggedValue> &name);
    JSHandle<SendableEnv> NewSendableEnv(int numSlots);
    JSHandle<JSFunction> NewJSSendableFunction(const JSHandle<Method> &methodHandle);

    void SetCodeEntryToFunctionFromMethod(const JSHandle<JSFunction> &func, const JSHandle<Method> &method);
    void SetNativePointerToFunctionFromMethod(const JSHandle<JSFunctionBase> &func, const JSHandle<Method> &method);

private:
    friend class GlobalEnv;
    friend class GlobalEnvConstants;
    friend class EcmaString;
    friend class SnapshotProcessor;
    friend class SingleCharTable;
    void InitObjectFields(const TaggedObject *object);

    JSThread *thread_ {nullptr};
    bool isTriggerGc_ {false};
    bool triggerSemiGC_ {false};

    EcmaVM *vm_ {nullptr};
    Heap *heap_ {nullptr};
    SharedHeap *sHeap_ {nullptr};

    static constexpr uint32_t LENGTH_THRESHOLD = 50;

    NO_COPY_SEMANTIC(ObjectFactory);
    NO_MOVE_SEMANTIC(ObjectFactory);

    void NewObjectHook() const;
    void NewSObjectHook() const;
    // used for creating jshclass in GlobalEnv, EcmaVM
    JSHandle<JSHClass> NewEcmaHClassClass(JSHClass *hclass, uint32_t size, JSType type);

    JSHandle<JSHClass> NewEcmaReadOnlyHClass(JSHClass *hclass, uint32_t size, JSType type,
                                             uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);
    JSHandle<JSHClass> InitClassClass();

    // used to create nonmovable js_object
    JSHandle<JSObject> NewNonMovableJSObject(const JSHandle<JSHClass> &jshclass);

    // used to create read only utf8 string at global constants
    JSHandle<EcmaString> NewFromASCIIReadOnly(std::string_view data);

    // used for creating Function
    JSHandle<JSFunction> NewJSFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &hclass);
    JSHandle<JSHClass> CreateObjectClass(const JSHandle<TaggedArray> &keys, const JSHandle<TaggedArray> &values);
    JSHandle<JSHClass> CreateObjectClass(const JSHandle<TaggedArray> &properties, size_t length);
    JSHandle<JSHClass> CreateFunctionClass(FunctionKind kind, uint32_t size, JSType type,
                                           const JSHandle<JSTaggedValue> &prototype);
    JSHandle<JSHClass> CreateBoundFunctionClass();
    JSHandle<JSHClass> CreateDefaultClassPrototypeHClass(JSHClass *hclass);
    JSHandle<JSHClass> CreateDefaultClassConstructorHClass(JSHClass *hclass);

    // used for creating ref.prototype in buildins, such as Number.prototype
    JSHandle<JSPrimitiveRef> NewJSPrimitiveRef(const JSHandle<JSHClass> &hclass,
                                               const JSHandle<JSTaggedValue> &object);

    JSHandle<EcmaString> GetStringFromStringTable(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress) const;
    JSHandle<EcmaString> GetCompressedSubStringFromStringTable(const JSHandle<EcmaString> &string, uint32_t offset,
                                                               uint32_t utf8Len) const;
    JSHandle<EcmaString> GetStringFromStringTableReadOnly(const uint8_t *utf8Data, uint32_t utf8Len,
                                                          bool canBeCompress = true) const;
    // For MUtf-8 string data
    EcmaString *PUBLIC_API GetRawStringFromStringTable(StringData sd,
                                                       MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE) const;
    EcmaString *GetRawStringFromStringTableWithoutJSHandle(StringData sd,
                                                           MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE) const;

    JSHandle<EcmaString> GetStringFromStringTable(const uint16_t *utf16Data, uint32_t utf16Len,
                                                  bool canBeCompress) const;

    JSHandle<EcmaString> GetStringFromStringTable(const JSHandle<EcmaString> &firstString,
                                                  const JSHandle<EcmaString> &secondString);

    JSHandle<JSHClass> CreateJSArguments(const JSHandle<GlobalEnv> &env);

    JSHandle<JSHClass> CreateJSArrayInstanceClass(JSHandle<JSTaggedValue> proto,
                                                  uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);

    JSHandle<JSHClass> CreateJSRegExpInstanceClass(JSHandle<JSTaggedValue> proto);

    inline TaggedObject *AllocObjectWithSpaceType(size_t size, JSHClass *cls, MemSpaceType type);
    JSHandle<TaggedArray> NewTaggedArrayWithoutInit(uint32_t length, MemSpaceType spaceType);

    // For object with many properties, directly create new HClass instead of searching on transitions
    JSHandle<JSTaggedValue> CreateLargeJSObjectWithProperties(size_t propertyCount,
                                                              const Local<JSValueRef> *keys,
                                                              const PropertyDescriptor *descs);
    JSHandle<JSTaggedValue> CreateLargeJSObjectWithNamedProperties(size_t propertyCount, const char **keys,
                                                                   const Local<JSValueRef> *values);
    // For object with numerous properties, directly create it in dictionary mode
    JSHandle<JSTaggedValue> CreateDictionaryJSObjectWithProperties(size_t propertyCount,
                                                                   const Local<JSValueRef> *keys,
                                                                   const PropertyDescriptor *descs);
    JSHandle<JSTaggedValue> CreateDictionaryJSObjectWithNamedProperties(size_t propertyCount, const char **keys,
                                                                        const Local<JSValueRef> *values);

    JSHandle<MutantTaggedArray> NewMutantTaggedArrayWithoutInit(uint32_t length, MemSpaceType spaceType);
    void MergeSendableClassElementsDic(JSHandle<TaggedArray> &elements,
                                       const JSHandle<JSTaggedValue> &elementsDicOfCtorVal,
                                       const JSHandle<JSTaggedValue> &elementsDicOfTrgVal);

    friend class Builtins;    // create builtins object need hclass
    friend class JSFunction;  // create prototype_or_hclass need hclass
    friend class JSHClass;    // HC transition need hclass
    friend class EcmaVM;      // hold the factory instance
    friend class JsVerificationTest;
    friend class PandaFileTranslator;
    friend class LiteralDataExtractor;
    friend class RuntimeStubs;
    friend class ClassInfoExtractor;
    friend class ModuleDataExtractor;
    friend class ModuleDataAccessor;
    friend class ConstantPool;
    friend class EcmaContext;
    friend class kungfu::TSHClassGenerator;
    friend class panda::FunctionRef;
};

class ClassLinkerFactory {
private:
    friend class GlobalEnv;  // root class in class_linker need hclass
    friend class EcmaVM;     // root class in class_linker need hclass
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_OBJECT_FACTORY_H
