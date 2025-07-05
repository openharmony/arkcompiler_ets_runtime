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

#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <ctime>

#include "ecmascript/accessor_data.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/template_map.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/transitions_dictionary.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
using MicroJobQueue = panda::ecmascript::job::MicroJobQueue;
using PendingJob = panda::ecmascript::job::PendingJob;
class HProfTest : public testing::Test {
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
};

class HProfTestHelper {
public:
    explicit HProfTestHelper(EcmaVM *vm) : instance(vm) {}

    ~HProfTestHelper()
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

    bool ContrastJSONLineHeader(const std::string &filePath, std::string lineHeader)
    {
        std::string line;
        std::ifstream inputStream(filePath);
        while (getline(inputStream, line)) {
            if (line.find(lineHeader) != line.npos) {
                return true;
            }
        }
        return false;
    }

    bool ContrastJSONSectionPayload(const std::string &filePath, std::string dataLable, int fieldNum)
    {
        std::string line;
        int i = 1;
        std::ifstream inputStream(filePath);
        while (getline(inputStream, line)) {
            if (i > 10 && line.find(dataLable) != line.npos) {  // 10 : Hit the line
                std::string::size_type pos = 0;
                int loop = 0;
                while ((pos = line.find(",", pos)) != line.npos) {
                    pos++;
                    loop++;  // "," count
                }
                return loop == fieldNum - 1;
            }
            i++;  // Search the Next Line
        }
        return false;  // Lost the Line
    }

    bool ContrastJSONPayloadCntAndHitStrField(const std::string &filePath, std::string dataLable, int fieldCnt,
                                              std::string fieldStr)
    {
        std::string line;
        int i = 1;
        std::ifstream inputStream(filePath);
        while (getline(inputStream, line)) {
            if (line.find(dataLable) != line.npos) {  // 3 : Hit the line
                std::string::size_type pos = 0;
                int loop = 0;
                while ((pos = line.find(",", pos)) != line.npos) {
                    pos++;
                    loop++;  // "," count
                }
                if (loop != fieldCnt - 1) {
                    return false;
                }
                return line.find(fieldStr) != line.npos; // check if hit the target field
            }
            i++;  // Search the Next Line
        }
        return false;  // Lost the Line
    }

    bool ContrastJSONNativeSizeNum(const std::string &filePath, std::string nodesLable, int nNum)
    {
        std::string line;
        bool hit = false;
        std::vector<std::string> nodeNativeSizes;
        std::ifstream inputStream(filePath);
        while (getline(inputStream, line)) {
            if (!hit && line.find(nodesLable) == line.npos) {
                continue; // Not Get
            }
            if (line.find("[]") != line.npos) {  // Empty
                break;
            }
            if (!hit) {
                hit = true;
            }
            if (hit && (line.find("],") != line.npos)) {
                break; // Reach End
            }
            int rCommaInd = line.rfind(','); // find last one
            if (rCommaInd == line.npos) {
                return false;
            }
            std::string nativeSizeStr = line.substr(rCommaInd + 1, line.size() - rCommaInd);
            if (nativeSizeStr.compare("0") != 0) {
                nodeNativeSizes.push_back(nativeSizeStr);
            }
        }
        const Heap* heap = instance->GetHeap();
        if (heap->nativePointerList_.size() != nodeNativeSizes.size() || nodeNativeSizes.size() != nNum) {
            return false;
        }
        int ind = 0;
        for (JSTaggedValue it : heap->nativePointerList_) {
            size_t size = reinterpret_cast<JSNativePointer *>(it.GetTaggedObject())->GetBindingSize();
            if (nodeNativeSizes[ind].compare(std::to_string(size)) != 0) {
                return false;
            }
            ind++;
        }
        return true;
    }

    bool ContrastJSONClousure(const std::string &filePath)
    {
        std::string lineBk;  // The Last Line
        std::string line;
        std::ifstream inputStream(filePath);
        while (getline(inputStream, line)) {
            lineBk = line;
        }
        return lineBk.compare("}") == 0;
    }

    int ExtractCountFromMeta(const std::string &filePath, std::string typeLable)
    {
        std::string line;
        std::ifstream inputStream(filePath);
        while (getline(inputStream, line)) {
            int length = line.length() - typeLable.length() - 1;
            if (line.find(typeLable) != line.npos) {  // Get
                if (line.find(",") == line.npos) {    // "trace_function_count" end without ","
                    length = line.length() - typeLable.length();
                }
                line = line.substr(typeLable.length(), length);
                return std::stoi(line.c_str());
            }
        }
        return -1;
    }

    int ExtractCountFromPayload(const std::string &filePath, std::string dataLabel)
    {
        std::string line;
        bool hit = false;
        int loop = 0;
        std::ifstream inputStream(filePath);
        while (getline(inputStream, line)) {
            if (!hit && line.find(dataLabel) != line.npos) {  // Get
                loop += 1;                                    // First Line
                hit = true;
                if (line.find("[]") != line.npos) {  // Empty
                    loop = 0;
                    return loop;
                } else {
                    continue;
                }
            }
            if (hit) {
                if (line.find("],") != line.npos) {  // Reach End
                    loop += 1;                       // End Line
                    return loop;
                } else {
                    loop++;
                    continue;
                }
            }
        }
        return -1;
    }

private:
    EcmaVM *instance {nullptr};
};

HWTEST_F_L0(HProfTest, ParseJSONHeader)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test1.heapsnapshot");
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "{\"snapshot\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "{\"meta\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "{\"node_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "\"node_types\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "\"edge_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "\"edge_types\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "\"trace_function_info_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "\"trace_node_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "\"sample_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("test1.heapsnapshot", "\"location_fields\":"));
}

HWTEST_F_L0(HProfTest, ContrastTraceFunctionInfo)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test2.heapsnapshot");
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("test2.heapsnapshot", "\"trace_function_infos\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastTraceTree)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test3.heapsnapshot");
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("test3.heapsnapshot", "\"trace_tree\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastSamples)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test4.heapsnapshot");
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("test4.heapsnapshot", "\"samples\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastLocations)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test5.heapsnapshot");
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("test5.heapsnapshot", "\"locations\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastString)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test6.heapsnapshot");
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("test6.heapsnapshot", "\"strings\":[", 2));
}

HWTEST_F_L0(HProfTest, ContrastClosure)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test7.heapsnapshot");
    ASSERT_TRUE(tester.ContrastJSONClousure("test7.heapsnapshot"));
}

HWTEST_F_L0(HProfTest, ContrastEdgeCount)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test8.heapsnapshot");
    ASSERT_TRUE(tester.ExtractCountFromMeta("test8.heapsnapshot", "\"edge_count\":") ==
                tester.ExtractCountFromPayload("test8.heapsnapshot", "\"edges\":["));
}

HWTEST_F_L0(HProfTest, TraceFuncInfoCount)
{
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test9.heapsnapshot");
    ASSERT_TRUE(tester.ExtractCountFromMeta("test9.heapsnapshot", "\"trace_function_count\":") ==
                tester.ExtractCountFromPayload("test9.heapsnapshot", "\"trace_function_infos\":"));
}

HWTEST_F_L0(HProfTest, DumpNativeSize)
{
    int nativeSizeNum = 6;
    int count = nativeSizeNum / 2;
    while (count-- > 0) {
        instance->GetFactory()->NewJSArrayBuffer(10);
        instance->GetFactory()->NewJSArrayBuffer(20);
    }
    HProfTestHelper tester(instance);
    tester.GenerateSnapShot("test10.heapsnapshot");

    ASSERT_TRUE(tester.ContrastJSONPayloadCntAndHitStrField("test10.heapsnapshot", "{\"node_fields\":",
                                                            9, "native_size"));
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("test10.heapsnapshot", "\"nodes\":[", 8));
    ASSERT_TRUE(tester.ContrastJSONNativeSizeNum("test10.heapsnapshot", "\"nodes\":[", nativeSizeNum));
}

HWTEST_F_L0(HProfTest, TestSetDumpFormatInRandomNum)
{
    std::srand(std::time(nullptr));
    for (int i = 0; i < 20; i++) {
        size_t size = std::rand() % 128;
        if (size <= 0) {
            continue;
        }
        EcmaVM *vm = instance;
        size_t maxEnumNum = static_cast<size_t>(DumpFormat::OTHER) + 1;
        DumpFormat dumpFormat = static_cast<DumpFormat>(size % maxEnumNum);
        DumpSnapShotOption dumpOption;
        dumpOption.dumpFormat = dumpFormat;
        dumpOption.isVmMode = true;
        dumpOption.isPrivate = false;
        dumpOption.captureNumericValue = false;
        if (size > sizeof(double)) {
            size = sizeof(double);
        }
        uint8_t* data = reinterpret_cast<uint8_t*>(malloc(size));
        for (size_t j = 0; j < size - 1; j++) {
            data[j] = size;
        }
        data[size - 1] = 0;
        std::string path(data, data + size);
        FileStream stream(path);
        Progress *progress = nullptr;
        DFXJSNApi::DumpHeapSnapshot(vm, &stream, dumpOption, progress);
        HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
        heapProfile->DumpHeapSnapshot(&stream, dumpOption);
        ASSERT_TRUE(heapProfile->GetIdCount() > 0);
        free(data);
    }
}

HWTEST_F_L0(HProfTest, TestIdConsistency)
{
    HProfTestHelper tester(instance);
    int64_t count1 = tester.GenerateSnapShot("TestIdConsistency_1.heapsnapshot");
    for (int i = 0; i < 100; ++i) {
        instance->GetFactory()->NewJSAsyncFuncObject();
        instance->GetFactory()->NewJSSymbol();
    }
    int64_t count2 = tester.GenerateSnapShot("TestIdConsistency_2.heapsnapshot");
    ASSERT_TRUE(std::abs(count1 - count2) <= 500LL);
    // load two heapsnapshots into chrome, and further use "Comparision View"
}
}  // namespace panda::test
