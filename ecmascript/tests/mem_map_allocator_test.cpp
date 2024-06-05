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


#include "ecmascript/ecma_vm.h"

#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {

constexpr size_t HUGE_OBJECT_CAPACITY = 1024_MB;

class MemMapAllocatorTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(MemMapAllocatorTest, GetMemFromList)
{
    MemMap memMap = PageMap(HUGE_OBJECT_CAPACITY, PAGE_PROT_NONE, DEFAULT_REGION_SIZE);
    PageRelease(memMap.GetMem(), memMap.GetSize());
    MemMapFreeList memMapFreeList;
    memMapFreeList.Initialize(memMap, memMap.GetSize() * 2);

    // From FreeList
    size_t size1 = 256 * 1024 * 1024;
    auto mem1 = memMapFreeList.GetMemFromList(size1);
    EXPECT_EQ(mem1.GetSize(), size1);

    // From FreeList
    size_t size2 = 512 * 1024 * 1024;
    auto mem2 = memMapFreeList.GetMemFromList(size2);
    EXPECT_EQ(mem2.GetSize(), size2);

    // From PageMap
    size_t size3 = 512 * 1024 * 1024;
    auto mem3 = memMapFreeList.GetMemFromList(size3);
    EXPECT_EQ(mem3.GetSize(), size3);

    memMapFreeList.AddMemToList(mem3);
    memMapFreeList.Finalize();
}

}  // namespace panda::test
