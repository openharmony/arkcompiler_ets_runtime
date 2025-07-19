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

#include "ecmascript/cross_vm/dynamic_object_descriptor.h"

#include "ecmascript/js_tagged_value.h"
#include "common_interfaces/objects/base_object_dispatcher.h"

namespace panda::ecmascript {

DynamicObjectDescriptor DynamicObjectDescriptor::dynObjectDescriptor_;

void DynamicObjectDescriptor::Initialize()
{
    common::BaseObjectDispatcher::GetDispatcher().RegisterDynamicObjectDescriptor(&dynObjectDescriptor_);
}

std::pair<JSTaggedValue, HandlerBase> DynamicObjectDescriptor::GetProperty(ThreadHolder *thread,
                                                                           const BaseObject *obj,
                                                                           const char *name) const
{
    return std::make_pair(JSTaggedValue(), HandlerBase());
}

std::pair<bool, HandlerBase> DynamicObjectDescriptor::SetProperty(ThreadHolder *thread,
                                                                  BaseObject *obj,
                                                                  const char *name,
                                                                  JSTaggedValue value)
{
    return std::make_pair(false, HandlerBase());
}

std::pair<JSTaggedValue, HandlerBase> DynamicObjectDescriptor::GetElementByIdx(ThreadHolder *thread,
                                                                               const BaseObject *obj,
                                                                               uint32_t index) const
{
    return std::make_pair(JSTaggedValue::Undefined(), HandlerBase());
}

std::pair<bool, HandlerBase> DynamicObjectDescriptor::SetElementByIdx(ThreadHolder *thread,
                                                                      BaseObject *obj,
                                                                      uint32_t index,
                                                                      JSTaggedValue value)
{
    return std::make_pair(false, HandlerBase());
}
}