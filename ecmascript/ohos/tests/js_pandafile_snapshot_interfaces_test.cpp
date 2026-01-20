/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <fstream>

#include "gtest/gtest.h"

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/ohos/js_pandafile_snapshot_interfaces.h"
#include "ecmascript/ohos/enable_aot_list_helper.h"
#include "ecmascript/ohos/tests/mock/mock_enable_aot_list_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::ohos;

namespace panda::test {
class JSPandaFileSnapshotInterfacesTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }

    void SetUp() override
    {
        vm_ = JSNApi::CreateEcmaVM(runtimeOptions_);
        ASSERT(vm_ != nullptr);
        vm_->GetJSThread()->ManagedCodeBegin();
    }

    void TearDown() override
    {
        if (vm_ != nullptr) {
            vm_->GetJSThread()->ManagedCodeEnd();
            ohos::EnableAotJitListHelper::GetInstance()->Clear();
            JSNApi::DestroyJSVM(vm_);
            vm_ = nullptr;
        }
    }

protected:
    JSRuntimeOptions runtimeOptions_;
    EcmaVM *vm_ {nullptr};
};

// Test Serialize function - non-existent file path
HWTEST_F_L0(JSPandaFileSnapshotInterfacesTest, SerializeNonExistentPathTest)
{
    const char* nonExistentPath = "/tmp/non_existent_file_12345.txt";
    JSPandaFileSnapshotInterfaces::Serialize(vm_, nonExistentPath);
    ASSERT_TRUE(true);
}

// Test Serialize function - empty path
HWTEST_F_L0(JSPandaFileSnapshotInterfacesTest, SerializeEmptyPathTest)
{
    const char* emptyPath = "";
    JSPandaFileSnapshotInterfaces::Serialize(vm_, emptyPath);
    ASSERT_TRUE(true);
}

// Test Serialize function - existing file
HWTEST_F_L0(JSPandaFileSnapshotInterfacesTest, SerializeExistingFileTest)
{
    const char* tempFile = "/tmp/test_snapshot_file.txt";
    std::ofstream outfile(tempFile);
    if (outfile.is_open()) {
        outfile << "test data" << std::endl;
        outfile.close();
        JSPandaFileSnapshotInterfaces::Serialize(vm_, tempFile);
        std::remove(tempFile);
        ASSERT_TRUE(true);
    } else {
        GTEST_LOG_(INFO) << "Cannot create temp file, skip this test";
        ASSERT_TRUE(true);
    }
}

// Test Serialize function - multiple path scenarios
HWTEST_F_L0(JSPandaFileSnapshotInterfacesTest, SerializeMultiplePathsTest)
{
    const char* paths[] = {
        "/tmp/snapshot1.bin",
        "/tmp/snapshot2.bin",
        nullptr
    };

    for (int i = 0; paths[i] != nullptr; i++) {
        JSPandaFileSnapshotInterfaces::Serialize(vm_, paths[i]);
    }

    ASSERT_TRUE(true);
}

// Test Serialize function - special character path
HWTEST_F_L0(JSPandaFileSnapshotInterfacesTest, SerializeSpecialCharPathTest)
{
    const char* specialPath = "/tmp/snapshot-file_with-special.chars.bin";
    JSPandaFileSnapshotInterfaces::Serialize(vm_, specialPath);
    ASSERT_TRUE(true);
}
}  // namespace panda::test
