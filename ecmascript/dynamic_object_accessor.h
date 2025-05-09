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

#ifndef ECMASCRIPT_DYNAMIC_OBJECT_ACCESSOR_H
#define ECMASCRIPT_DYNAMIC_OBJECT_ACCESSOR_H

#include "common_interfaces/objects/base_object.h"
#include "common_interfaces/objects/base_object_accessor.h"

namespace panda::ecmascript {
class DynamicObjectAccessor : public DynamicObjectAccessorInterface {
public:
    static void Initialize();

    bool HasProperty(ThreadHolder *thread, BaseObject *obj,
                     char *name) override;

    JSTaggedValue GetProperty(ThreadHolder *thread, BaseObject *obj,
                              char *name) override;

    bool SetProperty(ThreadHolder *thread, BaseObject *obj,
                     char *name, JSTaggedValue value) override;

    bool HasElementByIdx(ThreadHolder *thread,
                         BaseObject *obj,
                         uint32_t index) override;

    JSTaggedValue GetElementByIdx(ThreadHolder *thread,
                                  BaseObject *obj,
                                  uint32_t index) override;

    bool SetElementByIdx(ThreadHolder *thread, BaseObject *obj,
                         uint32_t index, JSTaggedValue value) override;

private:
    static DynamicObjectAccessor dynObjectAccessor_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MEM_DYNAMIC_OBJECT_ACCESSOR_H
