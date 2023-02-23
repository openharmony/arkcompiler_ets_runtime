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
#include "ecmascript/ts_types/ts_obj_layout_info.h"

namespace panda::ecmascript {
void TSObjLayoutInfo::AddKeyAndType(const JSThread *thread, const JSTaggedValue &key, const JSTaggedValue &typeIdVal)
{
    DISALLOW_GARBAGE_COLLECTION;
    int number = static_cast<int>(GetNumOfProperties());
    SetNumOfProperties(thread, number + 1);
    SetKey(thread, number, key);
    SetTypeId(thread, number, typeIdVal);
}
}  // namespace panda::ecmascript
