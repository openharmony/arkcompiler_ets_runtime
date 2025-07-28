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

#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {

    void JSThread::SetupXRefFields()
    {
        if (!vm_->GetJSOptions().EnableGlobalLeakCheck()) {
            newXRefGlobalHandle_ = [this](JSTaggedType value) {
                return globalStorage_->NewGlobalHandle<NodeKind::UNIFIED_NODE>(value);
            };
            disposeXRefGlobalHandle_ = [this](uintptr_t nodeAddr) {
                globalStorage_->DisposeGlobalHandle<NodeKind::UNIFIED_NODE>(nodeAddr);
            };
            setNodeKind_ = [this](NodeKind nodeKind) { globalStorage_->SetNodeKind(nodeKind); };
        } else {
            newXRefGlobalHandle_ = [this](JSTaggedType value) {
                return globalDebugStorage_->NewGlobalHandle<NodeKind::UNIFIED_NODE>(value);
            };
            disposeXRefGlobalHandle_ = [this](uintptr_t nodeAddr) {
                globalDebugStorage_->DisposeGlobalHandle<NodeKind::UNIFIED_NODE>(nodeAddr);
            };
            setNodeKind_ = [this](NodeKind nodeKind) { globalDebugStorage_->SetNodeKind(nodeKind); };
        }
    }
}