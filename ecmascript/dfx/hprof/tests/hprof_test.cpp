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
    HProfTestHelper(const std::string &aFilePath, EcmaVM *vm)
        : instance(vm),
          filePath(aFilePath)
    {
        fstream outputString(filePath, std::ios::out);
        outputString.close();
        outputString.clear();

        FileStream stream(filePath.c_str());
        HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
        heapProfile->DumpHeapSnapshot(DumpFormat::JSON, &stream);

        inputStream = fstream(filePath, std::ios::in);
        HeapProfilerInterface::Destroy(instance);
    }

    ~HProfTestHelper()
    {
        inputStream.close();
        inputStream.clear();
    }

    bool ContrastJSONLineHeader(std::string lineHeader)
    {
        bool allSame = false;
        std::string line;
        int i = 1;
        while (getline(inputStream, line)) {
            if (line.find(lineHeader) != line.npos) {
                allSame = true;
                break;
            }
            i++;
        }
        return allSame;
    }

    bool ContrastJSONSectionPayload(std::string dataLable, int fieldNum)
    {
        std::string line;
        int i = 1;
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

    bool ContrastJSONClousure()
    {
        std::string lineBk;  // The Last Line
        std::string line;
        while (getline(inputStream, line)) {
            lineBk = line;
        }
        return lineBk.compare("}") == 0;
    }

    int ExtractCountFromMeta(std::string typeLable)
    {
        std::string line;
        int i = 1;
        while (getline(inputStream, line)) {
            int length = line.length() - typeLable.length() - 1;
            if (line.find(typeLable) != line.npos) {  // Get
                if (line.find(",") == line.npos) {    // "trace_function_count" end without ","
                    length = line.length() - typeLable.length();
                }
                line = line.substr(typeLable.length(), length);
                return std::stoi(line.c_str());
            }
            i++;
        }
        return -1;
    }

    int ExtractCountFromPayload(std::string dataLabel)
    {
        std::string line;
        bool hit = false;
        int loop = 0;
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
    std::string filePath;
    std::fstream inputStream {};
};

HWTEST_F_L0(HProfTest, ParseJSONHeader)
{
    HProfTestHelper tester("ParseJSONHeader.heapsnapshot", instance);
    ASSERT_TRUE(tester.ContrastJSONLineHeader("{\"snapshot\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("{\"meta\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("{\"node_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("\"node_types\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("\"edge_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("\"edge_types\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("\"trace_function_info_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("\"trace_node_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("\"sample_fields\":"));
    ASSERT_TRUE(tester.ContrastJSONLineHeader("\"location_fields\":"));
}

HWTEST_F_L0(HProfTest, ContrastTraceFunctionInfo)
{
    HProfTestHelper tester("ContrastTraceFunctionInfo.heapsnapshot", instance);
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("\"trace_function_infos\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastTraceTree)
{
    HProfTestHelper tester("ContrastTraceTree.heapsnapshot", instance);
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("\"trace_tree\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastSamples)
{
    HProfTestHelper tester("ContrastSamples.heapsnapshot", instance);
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("\"samples\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastLocations)
{
    HProfTestHelper tester("ContrastLocations.heapsnapshot", instance);
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("\"locations\":", 2));  // Empty
}

HWTEST_F_L0(HProfTest, ContrastString)
{
    HProfTestHelper tester("ContrastString.heapsnapshot", instance);
    ASSERT_TRUE(tester.ContrastJSONSectionPayload("\"strings\":[", 1 + 1));
}

HWTEST_F_L0(HProfTest, ContrastClosure)
{
    HProfTestHelper tester("ContrastClosure.heapsnapshot", instance);
    ASSERT_TRUE(tester.ContrastJSONClousure());
}

HWTEST_F_L0(HProfTest, ContrastEdgeCount)
{
    HProfTestHelper tester("ContrastEdgeCount.heapsnapshot", instance);
    ASSERT_TRUE(tester.ExtractCountFromMeta("\"edge_count\":") == tester.ExtractCountFromPayload("\"edges\":["));
}

HWTEST_F_L0(HProfTest, ContrastTraceFunctionInfoCount)
{
    HProfTestHelper tester("ContrastTraceFunctionInfoCount.heapsnapshot", instance);
    ASSERT_TRUE(tester.ExtractCountFromMeta("\"trace_function_count\":") ==
                tester.ExtractCountFromPayload("\"trace_function_infos\":"));
}
}  // namespace panda::test
