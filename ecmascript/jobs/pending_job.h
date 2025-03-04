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

#ifndef ECMASCRIPT_JOBS_PENDING_JOB_H
#define ECMASCRIPT_JOBS_PENDING_JOB_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jobs/hitrace_scope.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/record.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript::job {
class PendingJob final : public Record {
public:
    static PendingJob *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsPendingJob());
        return static_cast<PendingJob *>(object);
    }

    static JSTaggedValue ExecutePendingJob(const JSHandle<PendingJob> &pendingJob, JSThread *thread)
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        EXECUTE_JOB_HITRACE(pendingJob);
        EXECUTE_JOB_TRACE(thread, pendingJob);

        JSHandle<JSTaggedValue> job(thread, pendingJob->GetJob());
        ASSERT(job->IsCallable());
        JSHandle<TaggedArray> argv(thread, pendingJob->GetArguments());
        const uint32_t argsLength = argv->GetLength();
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        bool stackTrace = thread->GetEcmaVM()->GetJsDebuggerManager()->IsAsyncStackTrace();
        if (stackTrace && argsLength >= 1) {
            thread->GetEcmaVM()->GetAsyncStackTrace()->InsertCurrentAsyncTaskStack(argv->Get(0));
        }
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, job, undefined, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(argsLength, argv);
        JSTaggedValue result = JSFunction::Call(info);
        if (stackTrace && argsLength >= 1) {
            thread->GetEcmaVM()->GetAsyncStackTrace()->RemoveAsyncTaskStack(argv->Get(0));
        }
        return result;
    }

    static constexpr size_t JOB_OFFSET = Record::SIZE;
    ACCESSORS(Job, JOB_OFFSET, ARGUMENT_OFFSET);
#if defined(ENABLE_HITRACE)
    ACCESSORS(Arguments, ARGUMENT_OFFSET, CHAINID_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(ChainId, uint64_t, CHAINID_OFFSET, SPANID_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(SpanId, uint64_t, SPANID_OFFSET, PARENTSPANID_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ParentSpanId, uint64_t, PARENTSPANID_OFFSET, FLAGS_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Flags, uint32_t, FLAGS_OFFSET, JOBID_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(JobId, uint64_t, JOBID_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(JOB_OFFSET, CHAINID_OFFSET)
#else
    ACCESSORS(Arguments, ARGUMENT_OFFSET, SIZE);

    DECL_VISIT_OBJECT(JOB_OFFSET, SIZE)
#endif

    DECL_DUMP()
};
}  // namespace panda::ecmascript::job
#endif  // ECMASCRIPT_JOBS_PENDING_JOB_H
