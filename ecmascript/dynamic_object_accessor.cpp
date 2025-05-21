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

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/dynamic_object_accessor.h"
#include "common_interfaces/objects/base_object_dispatcher.h"

namespace panda::ecmascript {

DynamicObjectAccessor DynamicObjectAccessor::dynObjectAccessor_;

void DynamicObjectAccessor::Initialize()
{
    BaseObjectDispatcher::GetDispatcher().RegisterDynamicObjectAccessor(&dynObjectAccessor_);
}

bool DynamicObjectAccessor::HasProperty([[maybe_unused]] ThreadHolder *thread,
                                        [[maybe_unused]] const BaseObject *obj,
                                        [[maybe_unused]] const char *name) const
{
    return false;
}

JSTaggedValue DynamicObjectAccessor::GetProperty([[maybe_unused]] ThreadHolder *thread,
                                                 [[maybe_unused]] const BaseObject *obj,
                                                 [[maybe_unused]] const char *name) const
{
    return JSTaggedValue::Undefined();
}

bool DynamicObjectAccessor::SetProperty([[maybe_unused]] ThreadHolder *thread,
                                        [[maybe_unused]] BaseObject *obj,
                                        [[maybe_unused]] const char *name,
                                        [[maybe_unused]] JSTaggedValue value)
{
    return false;
}

bool DynamicObjectAccessor::HasElementByIdx([[maybe_unused]] ThreadHolder *thread,
                                            [[maybe_unused]] const BaseObject *obj,
                                            [[maybe_unused]] const uint32_t index) const
{
    return false;
}

JSTaggedValue DynamicObjectAccessor::GetElementByIdx([[maybe_unused]] ThreadHolder *thread,
                                                     [[maybe_unused]] const BaseObject *obj,
                                                     [[maybe_unused]] const uint32_t index) const
{
    return JSTaggedValue::Undefined();
}

bool DynamicObjectAccessor::SetElementByIdx([[maybe_unused]] ThreadHolder *thread,
                                            [[maybe_unused]] BaseObject *obj,
                                            [[maybe_unused]] const uint32_t index,
                                            [[maybe_unused]] JSTaggedValue value)
{
    return false;
}
}