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
#ifndef ECMASCRIPT_CROSS_VM_JS_TAGGED_VALUE_HYBRID_INL_H
#define ECMASCRIPT_CROSS_VM_JS_TAGGED_VALUE_HYBRID_INL_H

#include "ecmascript/js_tagged_value.h"

#include "ecmascript/js_hclass.h"

namespace panda::ecmascript {

    inline bool JSTaggedValue::IsJSXRefObject() const
    {
        return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSXRefObject();
    }
}
#endif  // ECMASCRIPT_CROSS_VM_JS_HCLASS_HYBRID_H
