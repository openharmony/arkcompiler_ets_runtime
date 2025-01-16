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
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
void MegaICCache::Set(JSHClass *jsHclass, JSTaggedValue key, JSTaggedValue handler,
                      [[maybe_unused]] JSThread *thread)
{
    int hash = PrimaryHash(jsHclass, key);
    PropertyKey &primaryProp = primary_[hash];
    if (primaryProp.key_ != JSTaggedValue::Hole()) {
        JSHClass* oldHclass = primaryProp.hclass_;
        JSTaggedValue oldKey = primaryProp.key_;
        int secondaryHash = SecondaryHash(oldHclass, oldKey);
        PropertyKey &secondaryProp = secondary_[secondaryHash];
        secondaryProp.hclass_ = oldHclass;
        secondaryProp.key_ = oldKey;
        secondaryProp.results_ = primaryProp.results_;
    }
    primaryProp.hclass_ = jsHclass;
    primaryProp.key_ = key;
    primaryProp.results_ = handler;
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
        thread->IncMegaUpdateCount();
#endif
}
}