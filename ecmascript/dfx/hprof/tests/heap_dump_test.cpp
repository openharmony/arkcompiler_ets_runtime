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

#include <chrono>
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
using namespace panda::ecmascript;
using ErrorType = base::ErrorType;

class HeapDumpTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(ecmaVm_, thread_, scope_);
        ecmaVm_->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm_, scope_);
    }

    EcmaVM *ecmaVm_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    JSThread *thread_ {nullptr};
};

class HeapDumpTestHelper {
public:
    explicit HeapDumpTestHelper(EcmaVM *vm) : instance(vm) {}

    ~HeapDumpTestHelper()
    {
        HeapProfilerInterface::Destroy(instance);
    }

    size_t GenerateSnapShot(const std::string &filePath)
    {
        // first generate this file of filePath if not exist,
        // so the function `realpath` of FileStream can not failed on arm/arm64.
        fstream outputString(filePath, std::ios::out);
        outputString.close();
        outputString.clear();
        FileStream stream(filePath.c_str());
        HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
        DumpSnapShotOption dumpOption;
        dumpOption.dumpFormat = DumpFormat::JSON;
        heapProfile->DumpHeapSnapshot(&stream, dumpOption);
        return heapProfile->GetIdCount();
    }

    bool MatchHeapDumpString(const std::string &filePath, std::string targetStr)
    {
        std::string line;
        std::ifstream inputStream(filePath);
        std::size_t lineNum = 0;
        while (getline(inputStream, line)) {
            lineNum = line.find(targetStr);
            if (lineNum != line.npos) {
                return true;
            }
        }
        GTEST_LOG_(INFO) << "_______________" << targetStr << std::to_string(lineNum) <<"_______________ not found";
        return false;  // Lost the Line
    }

    JSHandle<JSTypedArray> CreateNumberTypedArray(JSType jsType)
    {
        JSHandle<GlobalEnv> env = instance->GetGlobalEnv();
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> handleTagValFunc = env->GetInt8ArrayFunction();
        switch (jsType) {
            case JSType::JS_INT8_ARRAY:
                break;
            case JSType::JS_UINT8_ARRAY:
                handleTagValFunc = env->GetUint8ArrayFunction();
                break;
            case JSType::JS_UINT8_CLAMPED_ARRAY:
                handleTagValFunc = env->GetUint8ClampedArrayFunction();
                break;
            case JSType::JS_INT16_ARRAY:
                handleTagValFunc = env->GetInt16ArrayFunction();
                break;
            case JSType::JS_UINT16_ARRAY:
                handleTagValFunc = env->GetUint16ArrayFunction();
                break;
            case JSType::JS_INT32_ARRAY:
                handleTagValFunc = env->GetInt32ArrayFunction();
                break;
            case JSType::JS_UINT32_ARRAY:
                handleTagValFunc = env->GetUint32ArrayFunction();
                break;
            case JSType::JS_FLOAT32_ARRAY:
                handleTagValFunc = env->GetFloat32ArrayFunction();
                break;
            case JSType::JS_FLOAT64_ARRAY:
                handleTagValFunc = env->GetFloat64ArrayFunction();
                break;
            case JSType::JS_BIGINT64_ARRAY:
                handleTagValFunc = env->GetBigInt64ArrayFunction();
                break;
            case JSType::JS_BIGUINT64_ARRAY:
                handleTagValFunc = env->GetBigUint64ArrayFunction();
                break;
            default:
                ASSERT_PRINT(false, "wrong jsType used in CreateNumberTypedArray function");
                break;
        }
        return JSHandle<JSTypedArray>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(handleTagValFunc), handleTagValFunc));
    }

    JSHandle<JSObject> NewObject(uint32_t size, JSType type, JSHandle<JSTaggedValue> proto)
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSHClass> hclass = factory->NewEcmaHClass(size, type, proto);
        return factory->NewJSObjectWithInit(hclass);
    }

    JSHandle<JSObject> NewSObject(uint32_t size, JSType type, JSHandle<JSTaggedValue> proto)
    {
        ObjectFactory *factory = instance->GetFactory();
        auto emptySLayout = instance->GetJSThread()->GlobalConstants()->GetHandledEmptySLayoutInfo();
        JSHandle<JSHClass> hclass = factory->NewSEcmaHClass(size, 0, type, proto, emptySLayout);
        return factory->NewJSObjectWithInit(hclass);
    }

    // JS_SET
    JSHandle<JSSet> NewJSSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsSetObject = NewObject(JSSet::SIZE, JSType::JS_SET, proto);
        JSHandle<JSSet> jsSet = JSHandle<JSSet>::Cast(jsSetObject);
        JSHandle<LinkedHashSet> linkedSet(LinkedHashSet::Create(thread));
        jsSet->SetLinkedSet(thread, linkedSet);
        return jsSet;
    }

    // JS_SHARED_SET
    JSHandle<JSSharedSet> NewJSSharedSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetSFunctionPrototype();
        JSHandle<JSObject> jsSSetObject = NewSObject(JSSharedSet::SIZE, JSType::JS_SHARED_SET, proto);
        JSHandle<JSSharedSet> jsSSet = JSHandle<JSSharedSet>::Cast(jsSSetObject);
        JSHandle<LinkedHashSet> linkedSet(
            LinkedHashSet::Create(thread, LinkedHashSet::MIN_CAPACITY, MemSpaceKind::SHARED));
        jsSSet->SetLinkedSet(thread, linkedSet);
        jsSSet->SetModRecord(0);
        return jsSSet;
    }

    // JS_MAP
    JSHandle<JSMap> NewJSMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsMapObject = NewObject(JSMap::SIZE, JSType::JS_MAP, proto);
        JSHandle<JSMap> jsMap = JSHandle<JSMap>::Cast(jsMapObject);
        JSHandle<LinkedHashMap> linkedMap(LinkedHashMap::Create(thread));
        jsMap->SetLinkedMap(thread, linkedMap);
        return jsMap;
    }

    // JS_SHARED_MAP
    JSHandle<JSSharedMap> NewJSSharedMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetSFunctionPrototype();
        JSHandle<JSObject> jsSMapObject = NewSObject(JSSharedMap::SIZE, JSType::JS_SHARED_MAP, proto);
        JSHandle<JSSharedMap> jsSMap = JSHandle<JSSharedMap>::Cast(jsSMapObject);
        JSHandle<LinkedHashMap> linkedMap(
            LinkedHashMap::Create(thread, LinkedHashMap::MIN_CAPACITY, MemSpaceKind::SHARED));
        jsSMap->SetLinkedMap(thread, linkedMap);
        jsSMap->SetModRecord(0);
        return jsSMap;
    }

    //JS_WEAK_SET
    JSHandle<JSWeakSet> NewJSWeakSet()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsWeakSetObject = NewObject(JSWeakSet::SIZE, JSType::JS_WEAK_SET, proto);
        JSHandle<JSWeakSet> jsWeakSet = JSHandle<JSWeakSet>::Cast(jsWeakSetObject);
        JSHandle<LinkedHashSet> weakLinkedSet(LinkedHashSet::Create(thread));
        jsWeakSet->SetLinkedSet(thread, weakLinkedSet);
        return jsWeakSet;
    }

    //JS_WEAK_MAP
    JSHandle<JSWeakMap> NewJSWeakMap()
    {
        JSThread *thread = instance->GetJSThread();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSObject> jsWeakMapObject = NewObject(JSWeakMap::SIZE, JSType::JS_WEAK_MAP, proto);
        JSHandle<JSWeakMap> jsWeakMap = JSHandle<JSWeakMap>::Cast(jsWeakMapObject);
        JSHandle<LinkedHashMap> weakLinkedMap(LinkedHashMap::Create(thread));
        jsWeakMap->SetLinkedMap(thread, weakLinkedMap);
        return jsWeakMap;
    }

    // JS_PROXY
    JSHandle<JSProxy> NewJSProxy()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSFunction *newTarget = instance->GetGlobalEnv()->GetObjectFunction().GetObject<JSFunction>();
        JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
        JSHandle<JSObject> jsObject =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(newTargetHandle), newTargetHandle);
        JSHandle<JSTaggedValue> emptyObj(thread, jsObject.GetTaggedValue());
        return factory->NewJSProxy(emptyObj, emptyObj);
    }

    // JS_FORIN_ITERATOR
    JSHandle<JSForInIterator> NewJSForInIterator()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> arrayEmpty(thread, factory->NewJSArray().GetTaggedValue());
        JSHandle<JSTaggedValue> keys(thread, factory->EmptyArray().GetTaggedValue());
        JSHandle<JSTaggedValue> hclass(thread, JSTaggedValue::Undefined());
        return factory->NewJSForinIterator(arrayEmpty, keys, hclass);
    }

    // JS_REG_EXP_ITERATOR
    JSHandle<JSRegExpIterator> NewJSRegExpIterator()
    {
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSTaggedValue> proto = instance->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<EcmaString> emptyString = factory->GetEmptyString();
        JSHandle<JSTaggedValue> jsRegExp(NewObject(JSRegExp::SIZE, JSType::JS_REG_EXP, proto));
        return factory->NewJSRegExpIterator(jsRegExp, emptyString, false, false);
    }

    // PROMISE_ITERATOR_RECORD
    JSHandle<PromiseIteratorRecord> NewPromiseIteratorRecord()
    {
        JSThread *thread = instance->GetJSThread();
        ObjectFactory *factory = instance->GetFactory();
        JSFunction *newTarget = instance->GetGlobalEnv()->GetObjectFunction().GetObject<JSFunction>();
        JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
        JSHandle<JSObject> jsObject =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(newTargetHandle), newTargetHandle);
        JSHandle<JSTaggedValue> emptyObj(thread, jsObject.GetTaggedValue());
        return factory->NewPromiseIteratorRecord(emptyObj, false);
    }

private:
    EcmaVM *instance {nullptr};
};

class MockHeapProfiler : public HeapProfilerInterface {
public:
    NO_MOVE_SEMANTIC(MockHeapProfiler);
    NO_COPY_SEMANTIC(MockHeapProfiler);
    explicit MockHeapProfiler(HeapProfilerInterface *profiler) : profiler_(profiler) {}
    ~MockHeapProfiler() override
    {
        allocEvtObj_.clear();
    };

    void AllocationEvent(TaggedObject *address, size_t size) override
    {
        allocEvtObj_.emplace(address, true);
        profiler_->AllocationEvent(address, size);
    }

    void MoveEvent(uintptr_t address, TaggedObject *forwardAddress, size_t size) override
    {
        return profiler_->MoveEvent(address, forwardAddress, size);
    }

    bool DumpHeapSnapshot(Stream *stream, const DumpSnapShotOption &dumpOption, Progress *progress = nullptr) override
    {
        return profiler_->DumpHeapSnapshot(stream, dumpOption, progress);
    }

    void DumpHeapSnapshot(const DumpSnapShotOption &dumpOption) override
    {
        profiler_->DumpHeapSnapshot(dumpOption);
    }

    bool StartHeapTracking(double timeInterval, bool isVmMode = true, Stream *stream = nullptr,
                           bool traceAllocation = false, bool newThread = true) override
    {
        return profiler_->StartHeapTracking(timeInterval, isVmMode, stream, traceAllocation, newThread);
    }

    bool StopHeapTracking(Stream *stream, Progress *progress = nullptr, bool newThread = true) override
    {
        return profiler_->StopHeapTracking(stream, progress, newThread);
    }

    bool UpdateHeapTracking(Stream *stream) override
    {
        return profiler_->UpdateHeapTracking(stream);
    }

    bool StartHeapSampling(uint64_t samplingInterval, int stackDepth = 128) override
    {
        return profiler_->StartHeapSampling(samplingInterval, stackDepth);
    }

    void StopHeapSampling() override
    {
        profiler_->StopHeapSampling();
    }

    const struct SamplingInfo *GetAllocationProfile() override
    {
        return profiler_->GetAllocationProfile();
    }

    size_t GetIdCount() override
    {
        return profiler_->GetIdCount();
    }

    std::unordered_map<TaggedObject *, bool> allocEvtObj_;
    HeapProfilerInterface *profiler_ {nullptr};
};

HWTEST_F_L0(HeapDumpTest, TestAllocationEvent)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR"heapdump.abc";
    const std::string jsFileName = HPROF_TEST_JS_FILES_DIR"heapdump.js";
    MockHeapProfiler mockHeapProfiler(ecmaVm_->GetOrNewHeapProfile());
    ecmaVm_->SetHeapProfile(&mockHeapProfiler);

    std::unordered_map<TaggedObject *, bool> ObjBeforeExecute;
    std::unordered_map<TaggedObject *, bool> *ObjMap = &ObjBeforeExecute;
    auto heap = ecmaVm_->GetHeap();
    ASSERT_NE(heap, nullptr);
    auto countCb = [&ObjMap](TaggedObject *obj) {
        ObjMap->emplace(obj, true);
    };
    heap->IterateOverObjects(countCb);
    RootVisitor rootVisitor = [&countCb]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value((slot).GetTaggedType());
        if (!value.IsHeapObject()) {
            return;
        }
        TaggedObject *root = value.GetTaggedObject();
        countCb(root);
    };
    RootRangeVisitor rangeVisitor = [&rootVisitor]([[maybe_unused]] Root type,
                                    ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            rootVisitor(type, slot);
        }
    };
    RootBaseAndDerivedVisitor derivedVisitor = []
        ([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base, [[maybe_unused]] ObjectSlot derived,
         [[maybe_unused]] uintptr_t baseOldObject) {};
    ecmaVm_->Iterate(rootVisitor, rangeVisitor, VMRootVisitType::HEAP_SNAPSHOT);
    thread_->Iterate(rootVisitor, rangeVisitor, derivedVisitor);

    bool result = JSNApi::Execute(ecmaVm_, abcFileName, "heapdump");
    EXPECT_TRUE(result);

    std::unordered_map<TaggedObject *, bool> ObjAfterExecute;
    ObjMap = &ObjAfterExecute;
    heap->IterateOverObjects(countCb);
    ecmaVm_->Iterate(rootVisitor, rangeVisitor, VMRootVisitType::HEAP_SNAPSHOT);
    thread_->Iterate(rootVisitor, rangeVisitor, derivedVisitor);
    ecmaVm_->SetHeapProfile(mockHeapProfiler.profiler_);

    std::unordered_map<std::string, int> noTraceObjCheck =
       {{"TaggedArray", 1}, {"AsyncFunction", 2}, {"LexicalEnv", 2}, {"Array", 3}, {"Function", 7}, {"Map", 1}};
    bool pass = true;
    std::unordered_map<std::string, int> noTraceObj;
    for (auto o = ObjAfterExecute.begin(); o != ObjAfterExecute.end(); o++) {
        if (ObjBeforeExecute.find(o->first) != ObjBeforeExecute.end()) {
            continue;
        }
        if (mockHeapProfiler.allocEvtObj_.find(o->first) != mockHeapProfiler.allocEvtObj_.end()) {
            continue;
        }
        auto objType = o->first->GetClass()->GetObjectType();
        std::string typeName = ConvertToStdString(JSHClass::DumpJSType(objType));
        if (noTraceObjCheck.size() == 0) {
            LOG_ECMA(ERROR) << "Object not traced, Addr=" << o->first << ", TypeName=" << typeName;
            pass = false;
        }
        if (noTraceObj.find(typeName) == noTraceObj.end()) {
            noTraceObj.emplace(typeName, 0);
        }
        noTraceObj[typeName] += 1;
    }
    for (auto o = noTraceObj.begin(); o != noTraceObj.end(); o++) {
        if (noTraceObjCheck.find(o->first) == noTraceObjCheck.end()) {
            LOG_ECMA(ERROR) << "Object not traced, TypeName=" << o->first << ", count=" << o->second;
            pass = false;
        }
    }
    ASSERT_TRUE(pass);
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpFunctionUrl)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR"heapdump.abc";

    bool result = JSNApi::Execute(ecmaVm_, abcFileName, "heapdump");
    EXPECT_TRUE(result);

    HeapDumpTestHelper tester(ecmaVm_);
    tester.GenerateSnapShot("testFunctionUrl.heapsnapshot");

    // match function url
    std::string line;
    std::ifstream inputStream("testFunctionUrl.heapsnapshot");
    bool strMatched = false;
    while (getline(inputStream, line)) {
        if (line == "\"heapdump greet(line:98)\",") {
            strMatched = true;
            break;
        }
    }
    ASSERT_TRUE(strMatched);
}

HWTEST_F_L0(HeapDumpTest, TestAllocationMassiveMoveNode)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR"allocation.abc";
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(ecmaVm_);
    
    // start allocation
    bool start = heapProfile->StartHeapTracking(50);
    EXPECT_TRUE(start);

    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeBeforeMs =
        std::chrono::time_point_cast<std::chrono::milliseconds>(currentTime).time_since_epoch().count();
    
    bool result = JSNApi::Execute(ecmaVm_, abcFileName, "allocation");
    
    currentTime = std::chrono::system_clock::now();
    auto currentTimeAfterMs =
        std::chrono::time_point_cast<std::chrono::milliseconds>(currentTime).time_since_epoch().count();
    EXPECT_TRUE(result);

    std::string fileName = "test.allocationtime";
    fstream outputString(fileName, std::ios::out);
    outputString.close();
    outputString.clear();

    // stop allocation
    FileStream stream(fileName.c_str());
    bool stop = heapProfile->StopHeapTracking(&stream, nullptr);
    EXPECT_TRUE(stop);
    HeapProfilerInterface::Destroy(ecmaVm_);

    auto timeSpent = currentTimeAfterMs - currentTimeBeforeMs;
    long long int limitedTimeMs = 30000;
    ASSERT_TRUE(timeSpent < limitedTimeMs);
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName1)
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    // TAGGED_ARRAY
    factory->NewTaggedArray(10);
    // LEXICAL_ENV
    factory->NewLexicalEnv(10);
    // CONSTANT_POOL
    factory->NewConstantPool(10);
    // PROFILE_TYPE_INFO
    factory->NewProfileTypeInfo(10);
    // TAGGED_DICTIONARY
    factory->NewDictionaryArray(10);
    // AOT_LITERAL_INFO
    factory->NewAOTLiteralInfo(10);
    // VTABLE
    factory->NewVTable(10);
    // COW_TAGGED_ARRAY
    factory->NewCOWTaggedArray(10);
    // HCLASS
    JSHandle<JSTaggedValue> proto = env->GetFunctionPrototype();
    factory->NewEcmaHClass(JSHClass::SIZE, JSType::HCLASS, proto);
    // LINKED_NODE
    JSHandle<LinkedNode> linkedNode(thread_, JSTaggedValue::Hole());
    factory->NewLinkedNode(1, JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Hole()),
        JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Hole()), linkedNode);
    // JS_NATIVE_POINTER
    auto newData = ecmaVm_->GetNativeAreaAllocator()->AllocateBuffer(8);
    factory->NewJSNativePointer(newData);

    tester.GenerateSnapShot("testGenerateNodeName_1.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalArray["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"LexicalEnv["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalConstantPool["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalProfileTypeInfo["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalDict["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalAOTLiteralInfo["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalVTable["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"ArkInternalCOWArray["));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"HiddenClass(NonMovable)"));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"LinkedNode\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "\"JSNativePointer\""));
    // Test Not Found
    ASSERT_TRUE(!tester.MatchHeapDumpString("testGenerateNodeName_1.heapsnapshot", "*#@failed case"));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName2)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    // JS_ERROR
    JSHandle<EcmaString> handleMessage(thread_, EcmaStringAccessor::CreateEmptyString(ecmaVm_));
    factory->NewJSError(ErrorType::ERROR, handleMessage);
    // JS_EVAL_ERROR
    factory->NewJSError(ErrorType::EVAL_ERROR, handleMessage);
    // JS_RANGE_ERROR
    factory->NewJSError(ErrorType::RANGE_ERROR, handleMessage);
    // JS_TYPE_ERROR
    factory->NewJSError(ErrorType::TYPE_ERROR, handleMessage);
    // JS_AGGREGATE_ERROR
    factory->NewJSAggregateError();
    // JS_REFERENCE_ERROR
    factory->NewJSError(ErrorType::REFERENCE_ERROR, handleMessage);
    // JS_URI_ERROR
    factory->NewJSError(ErrorType::URI_ERROR, handleMessage);
    // JS_SYNTAX_ERROR
    factory->NewJSError(ErrorType::SYNTAX_ERROR, handleMessage);
    // JS_OOM_ERROR
    factory->NewJSError(ErrorType::OOM_ERROR, handleMessage);
    // JS_TERMINATION_ERROR
    factory->NewJSError(ErrorType::TERMINATION_ERROR, handleMessage);

    tester.GenerateSnapShot("testGenerateNodeName_2.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Eval Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Range Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Type Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Aggregate Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Reference Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Uri Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Syntax Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"OutOfMemory Error\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_2.heapsnapshot", "\"Termination Error\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName3)
{
    HeapDumpTestHelper tester(ecmaVm_);

    // JS_INT8_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_INT8_ARRAY);
    // JS_UINT8_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT8_ARRAY);
    // JS_UINT8_CLAMPED_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT8_CLAMPED_ARRAY);
    // JS_INT16_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_INT16_ARRAY);
    // JS_UINT16_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT16_ARRAY);
    // JS_INT32_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_INT32_ARRAY);
    // JS_UINT32_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_UINT32_ARRAY);
    // JS_FLOAT32_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_FLOAT32_ARRAY);
    // JS_FLOAT64_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_FLOAT64_ARRAY);
    // JS_BIGINT64_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_BIGINT64_ARRAY);
    // JS_BIGUINT64_ARRAY
    tester.CreateNumberTypedArray(JSType::JS_BIGUINT64_ARRAY);
    
    tester.GenerateSnapShot("testGenerateNodeName_3.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Int8 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint8 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint8 Clamped Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Int16 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint16 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Int32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Uint32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Float32 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"Float64 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"BigInt64 Array\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_3.heapsnapshot", "\"BigUint64 Array\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName4)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    
    JSHandle<JSTaggedValue> proto = ecmaVm_->GetGlobalEnv()->GetFunctionPrototype();
    // JS_SET
    tester.NewJSSet();
    // JS_SHARED_SET
    tester.NewJSSharedSet();
    // JS_MAP
    tester.NewJSMap();
    // JS_SHARED_MAP
    tester.NewJSSharedMap();
    // JS_WEAK_SET
    tester.NewJSWeakSet();
    // JS_WEAK_MAP
    tester.NewJSWeakMap();
    // JS_ARRAY
    factory->NewJSArray();
    // JS_TYPED_ARRAY
    tester.NewObject(JSTypedArray::SIZE, JSType::JS_TYPED_ARRAY, proto);
    tester.GenerateSnapShot("testGenerateNodeName_4.heapsnapshot");

    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Set\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Map\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"SharedMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakSet\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"WeakMap\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"JSArray\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_4.heapsnapshot", "\"Typed Array\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName5)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    JSHandle<JSTaggedValue> proto = ecmaVm_->GetGlobalEnv()->GetFunctionPrototype();
    // JS_REG_EXP
    tester.NewObject(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    // JS_DATE
    tester.NewObject(JSDate::SIZE, JSType::JS_DATE, proto);
    // JS_ARGUMENTS
    factory->NewJSArguments();
    // JS_PROXY
    tester.NewJSProxy();
    // JS_PRIMITIVE_REF
    tester.NewObject(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, proto);
    // JS_DATA_VIEW
    factory->NewJSDataView(factory->NewJSArrayBuffer(10), 5, 5);

    tester.GenerateSnapShot("testGenerateNodeName_5.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Regexp\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Date\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Arguments\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Proxy\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"Primitive\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_5.heapsnapshot", "\"DataView\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName6)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);

    // JS_FORIN_ITERATOR
    tester.NewJSForInIterator();
    // JS_MAP_ITERATOR
    factory->NewJSMapIterator(tester.NewJSMap(), IterationKind::KEY);
    // JS_SHARED_MAP_ITERATOR
    factory->NewJSMapIterator(tester.NewJSSharedMap(), IterationKind::KEY);
    // JS_SET_ITERATOR
    factory->NewJSSetIterator(tester.NewJSSet(), IterationKind::KEY);
    // JS_SHARED_SET_ITERATOR
    factory->NewJSSetIterator(tester.NewJSSharedSet(), IterationKind::KEY);
    // JS_REG_EXP_ITERATOR
    tester.NewJSRegExpIterator();
    // JS_ARRAY_ITERATOR
    factory->NewJSArrayIterator(JSHandle<JSObject>::Cast(factory->NewJSArray()), IterationKind::KEY);
    // JS_STRING_ITERATOR
    JSStringIterator::CreateStringIterator(thread_, factory->GetEmptyString());

    tester.GenerateSnapShot("testGenerateNodeName_6.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"ForinInterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"MapIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"SharedMapIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"SetIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"SharedSetIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"RegExpIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"ArrayIterator\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_6.heapsnapshot", "\"StringIterator\""));
}

HWTEST_F_L0(HeapDumpTest, TestHeapDumpGenerateNodeName7)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    HeapDumpTestHelper tester(ecmaVm_);
    // JS_ARRAY_BUFFER
    factory->NewJSArrayBuffer(10);
    // JS_SHARED_ARRAY_BUFFER
    factory->NewJSSharedArrayBuffer(10);
    // PROMISE_REACTIONS
    factory->NewPromiseReaction();
    // PROMISE_CAPABILITY
    factory->NewPromiseCapability();
    // PROMISE_ITERATOR_RECORD
    tester.NewPromiseIteratorRecord();
    // PROMISE_RECORD
    factory->NewPromiseRecord();
    // RESOLVING_FUNCTIONS_RECORD
    factory->NewResolvingFunctionsRecord();
    // JS_PROMISE
    JSHandle<JSTaggedValue> proto = ecmaVm_->GetGlobalEnv()->GetFunctionPrototype();
    tester.NewObject(JSPromise::SIZE, JSType::JS_PROMISE, proto);
    // ASYNC_GENERATOR_REQUEST
    factory->NewAsyncGeneratorRequest();

    tester.GenerateSnapShot("testGenerateNodeName_7.heapsnapshot");
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"ArrayBuffer\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"SharedArrayBuffer\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseReaction\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseCapability\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseIteratorRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"PromiseRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"ResolvingFunctionsRecord\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"Promise\""));
    ASSERT_TRUE(tester.MatchHeapDumpString("testGenerateNodeName_7.heapsnapshot", "\"AsyncGeneratorRequest\""));
}
}