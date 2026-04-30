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

#ifndef ECMASCRIPT_ARKSTEED_TASK_H
#define ECMASCRIPT_ARKSTEED_TASK_H

#include "ecmascript/common.h"
#include "ecmascript/jit/jit_task.h"

namespace panda::ecmascript::arksteed {

class ArkSteedTask : public JitTask {
public:
    ArkSteedTask(JSThread *hostThread, JSThread *compilerThread, Jit *jit, JSHandle<JSFunction> &jsFunction,
                 CompilerTier tier, CString &methodName, int32_t offset, JitCompileMode mode);
    ~ArkSteedTask() = default;

    void Compile();
    void InstallCode() override;

    class AsyncTask : public common::Task {
    public:
        explicit AsyncTask(std::shared_ptr<ArkSteedTask> task, int32_t id) : common::Task(id), task_(task) {}

        bool Run(uint32_t threadIndex) override;

    private:
        std::shared_ptr<ArkSteedTask> task_;
    };
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_TASK_H
