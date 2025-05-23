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
#include "ecmascript/dynamic_type_converter.h"
#include "common_interfaces/objects/base_object_dispatcher.h"

namespace panda::ecmascript {

DynamicTypeConverter DynamicTypeConverter::dynTypeConverter_;

void DynamicTypeConverter::Initialize()
{
    BaseObjectDispatcher::GetDispatcher().RegisterDynamicTypeConverter(&dynTypeConverter_);
}

JSTaggedValue DynamicTypeConverter::WrapTagged([[maybe_unused]] ThreadHolder *thread,
                                               [[maybe_unused]] PandaType value)
{
    // fixme(liuzhijie)
    return JSTaggedValue::Undefined();
}

PandaType DynamicTypeConverter::UnwrapTagged([[maybe_unused]] JSTaggedValue value)
{
    // fixme(liuzhijie)
    return PandaType();
}
}