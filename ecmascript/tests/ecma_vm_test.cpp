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
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaVMTest : public BaseTestWithOutScope {
};

/*
 * @tc.name: CreateEcmaVMInTwoWays
 * @tc.desc: Create EcmaVM in 2 ways, check the Options state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaVMTest, CreateEcmaVMInTwoWays)
{
    RuntimeOption options;
    options.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM::SetMultiThreadCheck(true);
    EcmaVM *ecmaVm1 = JSNApi::CreateJSVM(options);
    EXPECT_TRUE(ecmaVm1->GetMultiThreadCheck());
    auto jsthread1 = ecmaVm1->GetJSThread();
    EXPECT_TRUE(jsthread1 != nullptr);

    std::thread t1([&]() {
        JSRuntimeOptions options2;
        options2.SetEnableArkTools(false);
        options2.SetEnableForceGC(false);
        options2.SetForceFullGC(false);
        options2.SetArkProperties(ArkProperties::GC_STATS_PRINT);
        options2.SetMemConfigProperty("jsHeap500");
        // A non-production gc strategy. Prohibit stw-gc 10 times.
        EcmaVM::SetMultiThreadCheck(false);
        EcmaVM *ecmaVm2 = JSNApi::CreateEcmaVM(options2);
        auto jsthread2 = ecmaVm2->GetJSThread();
        EXPECT_FALSE(ecmaVm2->GetMultiThreadCheck());
        EXPECT_TRUE(jsthread2 != nullptr);
        EXPECT_TRUE(ecmaVm1 != ecmaVm2);

        JSRuntimeOptions options1Out = ecmaVm1->GetJSOptions();
        JSRuntimeOptions options2Out = ecmaVm2->GetJSOptions();

        EXPECT_TRUE(&options1Out != &options2Out);

        EXPECT_TRUE(options1Out.EnableArkTools() != options2Out.EnableArkTools());
        EXPECT_TRUE(options1Out.EnableForceGC() != options2Out.EnableForceGC());
        EXPECT_TRUE(options1Out.ForceFullGC() != options2Out.ForceFullGC());
        EXPECT_TRUE(options1Out.GetArkProperties() != options2Out.GetArkProperties());
        EXPECT_TRUE(options2Out.GetHeapSize() == 500_MB);

        options2.SetAsmOpcodeDisableRange("1,10");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 1); // 1 targer start
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == 10); // 10 targer end

        options2.SetAsmOpcodeDisableRange("0x1,0xa");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 1); // 1 targer start
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == 10); // 10 targer end

        options2.SetAsmOpcodeDisableRange(",");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 0);
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == kungfu::BYTECODE_STUB_END_ID);

        options2.SetAsmOpcodeDisableRange("@,@");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 0);
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == kungfu::BYTECODE_STUB_END_ID);

        JSNApi::DestroyJSVM(ecmaVm2);
    });
    t1.join();
    JSNApi::DestroyJSVM(ecmaVm1);
}
}  // namespace panda::ecmascript
