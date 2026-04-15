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

#include "ecmascript/mem/shared_heap/global_gc_marker.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/shared_heap/global_gc_visitor-inl.h"
#include "ecmascript/mem/work_manager-inl.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {

void GlobalGCMarker::MarkRoots(Heap *heap, GlobalGCWorkNodeHolder *holder) const
{
    GlobalMarkRootVisitor rootVisitor(holder);
    EcmaVM *vm = heap->GetEcmaVM();
    ObjectXRay::VisitVMRoots(vm, rootVisitor);

    WeakRootVisitor markWeak = [holder](TaggedObject *header) -> TaggedObject* {
        Region *region = Region::ObjectAddressToRange(header);
        if (region->InSharedHeap()) {
            return header;
        }
        if (region->AtomicMark(header)) {
            holder->Push(header);
        }
        return header;
    };
    vm->GetJSThread()->IterateWeakEcmaGlobalStorage(markWeak);
    vm->ProcessReferences(markWeak);
    vm->ProcessSnapShotEnv(markWeak);
    vm->GetJSThread()->UpdateJitCodeMapReference(markWeak);
    vm->IterateWeakGlobalEnvList(markWeak);
}

void GlobalGCMarker::ProcessMarkStack(GlobalGCWorkManager *workManager, uint32_t threadIndex) const
{
    GlobalGCWorkNodeHolder *holder = workManager->GetHolder(threadIndex);
    GlobalMarkObjectVisitor visitor(holder);
    TaggedObject *obj = nullptr;
    while (holder->Pop(&obj)) {
        JSHClass *hclass = obj->GetClass();
        visitor.VisitHClass(hclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclass, visitor);
    }
}

void GlobalGCMarker::MarkJitCodeMaps(GlobalGCWorkManager *workManager, uint32_t threadIndex) const
{
    GlobalGCWorkNodeHolder *holder = workManager->GetHolder(threadIndex);
    GlobalMarkObjectVisitor objectVisitor(holder);
    JitCodeMapVisitor visitor = [&objectVisitor](std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps) {
        for (auto it = jitCodeMaps.begin(); it != jitCodeMaps.end(); ++it) {
            auto *jsError = reinterpret_cast<TaggedObject *>(it->first);
            Region *region = Region::ObjectAddressToRange(jsError);
            if (!region->Test(jsError)) {
                continue;
            }
            for (auto &entry : *(it->second)) {
                auto *jitCode = std::get<0>(entry);
                objectVisitor.HandleObject(jitCode, Region::ObjectAddressToRange(jitCode));
            }
        }
    };
    Runtime::GetInstance()->GCIterateThreadList([&visitor](JSThread *iteratedThread) {
        iteratedThread->IterateJitCodeMap(visitor);
    });
    ProcessMarkStack(workManager, threadIndex);
}

}  // namespace panda::ecmascript
