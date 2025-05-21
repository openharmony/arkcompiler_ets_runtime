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

#ifndef ECMASCRIPT_DYNAMIC_TYPE_CONVERTER_H
#define ECMASCRIPT_DYNAMIC_TYPE_CONVERTER_H

#include "common_interfaces/objects/base_object.h"
#include "common_interfaces/objects/base_type_converter.h"

namespace panda::ecmascript {
class DynamicTypeConverter : public DynamicTypeConverterInterface {
public:
    static void Initialize();

    JSTaggedValue PUBLIC_API WrapTagged(ThreadHolder *thread, PandaType value) override;

    PandaType PUBLIC_API UnWrapTagged(JSTaggedValue value) override;

private:
    static DynamicTypeConverter dynTypeConverter_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MEM_DYNAMIC_TYPE_CONVERTER_H
