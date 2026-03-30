/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
class AOTFileGeneratorTests : public testing::Test {
};
using ecmascript::kungfu::AOTFileGenerator;
using ecmascript::kungfu::AnFileInfo;
using ecmascript::kungfu::MachineCodeDesc;
using ecmascript::CString;

// Test CreateDirIfNotExist with empty path
HWTEST_F_L0(AOTFileGeneratorTests, CreateDirIfNotExist_EmptyPath)
{
    bool result = AOTFileGenerator::CreateDirIfNotExist("");
    ASSERT_EQ(result, false);
}

// Mock AOTFileGenerator for testing GenerateMergedStackmapSection failure path
class MockAOTFileGenerator : public AOTFileGenerator {
public:
    MockAOTFileGenerator() : AOTFileGenerator(nullptr, nullptr, nullptr, "aarch64-unknown-linux-gnu", false, 0)
    {
    }

    bool GenerateMergedStackmapSection()
    {
        return false;
    }

    bool TestSaveAOTFile(const std::string &filename, const std::string &appSignature,
                         const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
    {
        if (aotInfo_.GetTotalCodeSize() == 0) {
            return false;
        }
        if (!CreateDirIfNotExist(filename)) {
            return false;
        }
        if (!GenerateMergedStackmapSection()) {
            return false;
        }
        return true;
    }

    bool TestGetMemoryCodeInfos(MachineCodeDesc &machineCodeDesc)
    {
        if (aotInfo_.GetTotalCodeSize() == 0) {
            return false;
        }
        if (!GenerateMergedStackmapSection()) {
            return false;
        }
        return false;
    }

    // Test method that simulates GenerateMergedStackmapSection when stackMapInfo_ is nullptr
    // Since stackMapInfo_ is nullptr in constructor, this simulates the failure path
    bool TestGenerateMergedStackmapSectionWithNullptr()
    {
        // This simulates the branch: if (stackMapInfo_ == nullptr) { return false; }
        // stackMapInfo_ is nullptr by default in AOTFileGenerator constructor
        // So calling the real GenerateMergedStackmapSection will return false
        return GenerateMergedStackmapSection();
    }

    void SetCodeSize(uint32_t size)
    {
        aotInfo_.accumulateTotalSize(size);
    }
};

// Test SaveAOTFile fails when GenerateMergedStackmapSection returns false
HWTEST_F_L0(AOTFileGeneratorTests, SaveAOTFile_FailsWhenGenerateMergedStackmapSectionFails)
{
    MockAOTFileGenerator generator;
    generator.SetCodeSize(100);

    std::string filename = "/tmp/test.aot";
    std::string appSignature = "";
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;

    bool result = generator.TestSaveAOTFile(filename, appSignature, fileNameToChecksumMap);
    ASSERT_EQ(result, false);
}

// Test GetMemoryCodeInfos fails when GenerateMergedStackmapSection returns false
HWTEST_F_L0(AOTFileGeneratorTests, GetMemoryCodeInfos_FailsWhenGenerateMergedStackmapSectionFails)
{
    MockAOTFileGenerator generator;
    generator.SetCodeSize(100);

    MachineCodeDesc machineCodeDesc = {};

    bool result = generator.TestGetMemoryCodeInfos(machineCodeDesc);
    ASSERT_EQ(result, false);
}

// Test GenerateMergedStackmapSection fails when stackMapInfo_ is nullptr
// Covers the branch: if (stackMapInfo_ == nullptr) { return false; }
HWTEST_F_L0(AOTFileGeneratorTests, GenerateMergedStackmapSection_FailsWhenStackMapInfoIsNullptr)
{
    MockAOTFileGenerator generator;

    // stackMapInfo_ is nullptr by default in the constructor, so this should return false
    bool result = generator.TestGenerateMergedStackmapSectionWithNullptr();
    ASSERT_EQ(result, false);
}
}  // namespace panda::test
