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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_INTO_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_INTO_TEST_H

#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsStepIntoTest : public TestEvents {
public:
    JsStepIntoTest()
    {
        vmStart = [this] {
            // 45、20: line number for breakpoint array
            int32_t breakpoint[6][2] = {{45, 0}, {20, 0}, {25, 0}, {31, 0}, {33, 0}, {48, 0}};
            // 16、21: line number for stepinto array
            int32_t stepInto[5][2] = {{16, 0}, {21, 0}, {30, 0}, {32, 0}, {38, 0}};
            // 6、2: breakpoint array for total index
            SetJSPtLocation("StepInto.js", breakpoint[0], 6, 2, pointerLocations_);
            // 5、2：stepinto array for total index
            SetJSPtLocation("StepInto.js", stepInto[0], 5, 2, stepLocations_);
            return true;
        };

        vmDeath = [this]() {
            ASSERT_EQ(breakpointCounter_, pointerLocations_.size());  // size: break point counter
            ASSERT_EQ(stepCompleteCounter_, stepLocations_.size());  // size: step complete counter
            return true;
        };

        loadModule = [this](std::string_view moduleName) {
            ASSERT_EQ(moduleName, pandaFile_);
            debugger_->NotifyScriptParsed(0, moduleName.data());
            auto condFuncRef = FunctionRef::Undefined(vm_);
            for (auto &iter : pointerLocations_) {
                auto ret = debugInterface_->SetBreakpoint(iter, condFuncRef);
                ASSERT_TRUE(ret);
            }
            return true;
        };

        breakpoint = [this](const JSPtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            ASSERT_LOCATION_EQ(location, pointerLocations_.at(breakpointCounter_));
            ++breakpointCounter_;
            TestUtil::SuspendUntilContinue(DebugEvent::BREAKPOINT, location);
            debugger_->StepInto(StepIntoParams());
            return true;
        };

        singleStep = [this](const JSPtLocation &location) {
            if (debugger_->NotifySingleStep(location)) {
                ASSERT_TRUE(location.GetMethodId().IsValid());
                ASSERT_LOCATION_EQ(location, stepLocations_.at(stepCompleteCounter_));
                stepCompleteCounter_++;
                if (stepCompleteCounter_ == (int32_t)stepLocations_.size()) {
                    TestUtil::Event(DebugEvent::STEP_COMPLETE);
                }
                return true;
            }
            return false;
        };

        scenario = [this]() {
            while (true) {
                if (breakpointCounter_ >= (int32_t)pointerLocations_.size())  {
                    TestUtil::WaitForStepComplete();
                    break;
                }
                ASSERT_BREAKPOINT_SUCCESS(pointerLocations_.at(breakpointCounter_));
                TestUtil::Continue();
                auto ret = debugInterface_->RemoveBreakpoint(pointerLocations_.at(breakpointCounter_-1));
                ASSERT_TRUE(ret);
            }
            ASSERT_EXITED();
            return true;
        };
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {pandaFile_, entryPoint_};
    }

private:
    std::string pandaFile_ = DEBUGGER_ABC_DIR "StepInto.abc";
    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation location1_ {nullptr, JSPtLocation::EntityId(0), 0};
    JSPtLocation location2_ {nullptr, JSPtLocation::EntityId(0), 0};
    int32_t breakpointCounter_ = 0;
    int32_t stepCompleteCounter_ = 0;
    std::vector<JSPtLocation> pointerLocations_;
    std::vector<JSPtLocation> stepLocations_;

    void SetJSPtLocation(const char *sFile, int32_t *arr, int32_t m, int32_t n, std::vector<JSPtLocation> &vct_)
    {
        for (int32_t i = 0; i < m; i++) {
            JSPtLocation location_ = TestUtil::GetLocation(sFile, arr[i*n], arr[i*n + 1], pandaFile_.c_str());
            vct_.push_back(location_);
        }
    };
};

std::unique_ptr<TestEvents> GetJsStepIntoTest()
{
    return std::make_unique<JsStepIntoTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_INTO_TEST_H
