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

#include "containersprivatelinkedlist_fuzzer.h"
#include "test/fuzztest/containersprivatecommon_fuzzer/containersprivatecommon_fuzzer.h"

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"


using namespace panda;
using namespace panda::test;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace OHOS {
    void ContainersPrivateLinkedListFuzzTest(const uint8_t* data, size_t size)
    {
        ContainersPrivateFuzzTestHelper::ContainersPrivateCommonFuzzTest(data, size, ContainerTag::LinkedList);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ContainersPrivateLinkedListFuzzTest(data, size);
    return 0;
}