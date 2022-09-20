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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_OUT_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_OUT_TEST_H

#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsStepOutTest : public TestEvents {
public:
    JsStepOutTest()
    {
        vmStart = [this] {
            // 74、36: line number for breakpoint array
            size_t breakpoint[5][2] = {{74, 0}, {36, 0}, {50, 0}, {61, 0}, {96, 0}};
            // 28: line number for stepinto array
            size_t stepOut[4][2] = {{28, 0}, {43, 0}, {57, 0}, {88, 5}};
            SetJSPtLocation(breakpoint[0], POINTER_SIZE, pointerLocations_);
            SetJSPtLocation(stepOut[0], STEP_SIZE, stepLocations_);
            return true;
        };

        vmDeath = [this]() {
            ASSERT_EQ(breakpointCounter_, pointerLocations_.size());  // size: break point counter
            ASSERT_EQ(stepCompleteCounter_, stepLocations_.size());  // size: step complete counter
            return true;
        };

        loadModule = [this](std::string_view moduleName) {
            TestUtil::SuspendUntilContinue(DebugEvent::LOAD_MODULE);
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
            debugger_->StepOut();
            return true;
        };

        singleStep = [this](const JSPtLocation &location) {
            if (debugger_->NotifySingleStep(location)) {
                ASSERT_TRUE(location.GetMethodId().IsValid());
                ASSERT_LOCATION_EQ(location, stepLocations_.at(stepCompleteCounter_));
                stepCompleteCounter_++;
                TestUtil::SuspendUntilContinue(DebugEvent::STEP_COMPLETE, location);
                return true;
            }
            return false;
        };

        scenario = [this]() {
            TestUtil::WaitForLoadModule();
            TestUtil::Continue();
            size_t index = 0;
            while (index < pointerLocations_.size()) {
                TestUtil::WaitForBreakpoint(pointerLocations_.at(index));
                TestUtil::Continue();
                if (index < STEP_SIZE) {
                    TestUtil::WaitForStepComplete(stepLocations_.at(index));
                    TestUtil::Continue();
                }
                ++index;
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
    static constexpr size_t LINE_COLUMN = 2;
    static constexpr size_t POINTER_SIZE = 5;
    static constexpr size_t STEP_SIZE = 4;

    std::string pandaFile_ = DEBUGGER_ABC_DIR "step.abc";
    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation location1_ {nullptr, JSPtLocation::EntityId(0), 0};
    JSPtLocation location2_ {nullptr, JSPtLocation::EntityId(0), 0};
    size_t breakpointCounter_ = 0;
    size_t stepCompleteCounter_ = 0;
    std::vector<JSPtLocation> pointerLocations_;
    std::vector<JSPtLocation> stepLocations_;

    void SetJSPtLocation(size_t *arr, size_t number, std::vector<JSPtLocation> &locations)
    {
        for (size_t i = 0; i < number; i++) {
            JSPtLocation location_ =
                TestUtil::GetLocation(arr[i * LINE_COLUMN], arr[i * LINE_COLUMN + 1], pandaFile_.c_str());
            locations.push_back(location_);
        }
    };
};

std::unique_ptr<TestEvents> GetJsStepOutTest()
{
    return std::make_unique<JsStepOutTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_OUT_TEST_H
