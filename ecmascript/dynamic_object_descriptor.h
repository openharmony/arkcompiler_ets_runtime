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

#ifndef ECMASCRIPT_DYNAMIC_OBJECT_DESCRIPTOR_H
#define ECMASCRIPT_DYNAMIC_OBJECT_DESCRIPTOR_H

#include "common_interfaces/objects/base_object.h"
#include "common_interfaces/objects/base_object_descriptor.h"
#include "ecmascript/ic/ic_handler.h"

namespace panda::ecmascript {
class DynamicObjectDescriptor : public common::DynamicObjectDescriptorInterface {
public:
    static void Initialize();

    std::pair<JSTaggedValue, HandlerBase> GetProperty(ThreadHolder *thread,
                                                      const BaseObject *obj,
                                                      const char *name) const override;

    std::pair<bool, HandlerBase> SetProperty(ThreadHolder *thread, BaseObject *obj, const char *name,
                                             JSTaggedValue value) override;

    std::pair<JSTaggedValue, HandlerBase> GetElementByIdx(ThreadHolder *thread, const BaseObject *obj,
                                                          uint32_t index) const override;

    std::pair<bool, HandlerBase> SetElementByIdx(ThreadHolder *thread, BaseObject *obj, uint32_t index,
                                                 JSTaggedValue value) override;

private:
    static DynamicObjectDescriptor dynObjectDescriptor_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MEM_DYNAMIC_OBJECT_DESCRIPTOR_H
