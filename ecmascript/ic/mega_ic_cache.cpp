/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
 
#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/ecma_context.h"

namespace panda::ecmascript {
void MegaICCache::Set(JSHClass *jsHclass, JSTaggedValue key, JSTaggedValue handler,
                      [[maybe_unused]] JSThread *thread)
{
    int hash = Hash(jsHclass, key);
    PropertyKey &prop = keys_[hash];
    prop.hclass_ = jsHclass;
    prop.key_ = key;
    keys_[hash].results_ = handler;
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
        thread->GetCurrentEcmaContext()->IncMegaUpdateCount();
#endif
}
}