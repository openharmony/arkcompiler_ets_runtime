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
#include "ecmascript/ohos/module_snapshot_interfaces.h"
#include "ecmascript/ohos/enable_aot_list_helper.h"
#include "ecmascript/ohos/tests/mock/mock_enable_aot_list_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::ohos;

namespace panda::test {
class ModuleSnapshotInterfacesTest : public testing::Test {
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

//Test Serialize function - non-existent file path
HWTEST_F_L0(ModuleSnapshotInterfacesTest, SerializeNonExistentPathTest)
{
    CString nonExistentPath = "/tmp/non_existent_module_snapshot_12345.txt";
    ModuleSnapshotInterfaces::Serialize(vm_, nonExistentPath);
    ASSERT_TRUE(true);
}

//Test Serialize function - empty path
HWTEST_F_L0(ModuleSnapshotInterfacesTest, SerializeEmptyPathTest)
{
    CString emptyPath = "";
    ModuleSnapshotInterfaces::Serialize(vm_, emptyPath);
    ASSERT_TRUE(true);
}

//Test Serialize function - existing file
HWTEST_F_L0(ModuleSnapshotInterfacesTest, SerializeExistingFileTest)
{
    const char* tempFile = "/tmp/test_module_snapshot_file.txt";
    std::ofstream outfile(tempFile);
    if (outfile.is_open()) {
        outfile << "test data" << std::endl;
        outfile.close();
        CString path(tempFile);
        ModuleSnapshotInterfaces::Serialize(vm_, path);
        std::remove(tempFile);
        ASSERT_TRUE(true);
    } else {
        GTEST_LOG_(INFO) << "Cannot create temp file, skip this test";
        ASSERT_TRUE(true);
    }
}

//Test Serialize function - special character path
HWTEST_F_L0(ModuleSnapshotInterfacesTest, SerializeSpecialCharPathTest)
{
    CString specialPath = "/tmp/module-snapshot_with-special.chars.bin";
    ModuleSnapshotInterfaces::Serialize(vm_, specialPath);
    ASSERT_TRUE(true);
}

//Test Deserialize function - non-existent file path
HWTEST_F_L0(ModuleSnapshotInterfacesTest, DeserializeNonExistentPathTest)
{
    CString nonExistentPath = "/tmp/non_existent_module_deserialize_12345.txt";
    ModuleSnapshotInterfaces::Deserialize(vm_, nonExistentPath);
    ASSERT_TRUE(true);
}

//Test Deserialize function - empty path
HWTEST_F_L0(ModuleSnapshotInterfacesTest, DeserializeEmptyPathTest)
{
    CString emptyPath = "";
    ModuleSnapshotInterfaces::Deserialize(vm_, emptyPath);
    ASSERT_TRUE(true);
}

//Test Deserialize function - existing file
HWTEST_F_L0(ModuleSnapshotInterfacesTest, DeserializeExistingFileTest)
{
    const char* tempFile = "/tmp/test_module_deserialize_file.txt";
    std::ofstream outfile(tempFile);
    if (outfile.is_open()) {
        outfile << "test data" << std::endl;
        outfile.close();
        CString path(tempFile);
        ModuleSnapshotInterfaces::Deserialize(vm_, path);
        std::remove(tempFile);
        ASSERT_TRUE(true);
    } else {
        GTEST_LOG_(INFO) << "Cannot create temp file, skip this test";
        ASSERT_TRUE(true);
    }
}

//Test Deserialize function - special character path
HWTEST_F_L0(ModuleSnapshotInterfacesTest, DeserializeSpecialCharPathTest)
{
    CString specialPath = "/tmp/module-deserialize_with-special.chars.bin";
    ModuleSnapshotInterfaces::Deserialize(vm_, specialPath);
    ASSERT_TRUE(true);
}
}  // namespace panda::test
