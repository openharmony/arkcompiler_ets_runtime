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

#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/include/jsnapi.h"
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
    ecmaVm_->Iterate(rootVisitor, rangeVisitor);
    thread_->Iterate(rootVisitor, rangeVisitor, derivedVisitor);

    bool result = JSNApi::Execute(ecmaVm_, abcFileName, "heapdump");
    EXPECT_TRUE(result);

    std::unordered_map<TaggedObject *, bool> ObjAfterExecute;
    ObjMap = &ObjAfterExecute;
    heap->IterateOverObjects(countCb);
    ecmaVm_->Iterate(rootVisitor, rangeVisitor);
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
}