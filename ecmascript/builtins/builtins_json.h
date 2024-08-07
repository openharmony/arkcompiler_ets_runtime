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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_JSON_H
#define ECMASCRIPT_BUILTINS_BUILTINS_JSON_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/json_helper.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::builtins {

class BuiltinsJson : public base::BuiltinsBase {
public:
    using TransformType = base::JsonHelper::TransformType;
    using ParseOptions =  base::JsonHelper::ParseOptions;
    static JSTaggedValue Parse(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Stringify(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ParseWithTransformType(EcmaRuntimeCallInfo *argv, TransformType transformType);
    static JSTaggedValue StringifyWithTransformType(EcmaRuntimeCallInfo *argv, TransformType transformType);
private:
    static JSTaggedValue ParseWithTransformType(const EcmaVM *vm, JSHandle<JSTaggedValue> &msg,
                                                JSHandle<JSTaggedValue> &reviverVal, TransformType transformType,
                                                ParseOptions options);
};

class BuiltinsSendableJson : public base::BuiltinsBase {
public:
    static JSTaggedValue Parse(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Stringify(EcmaRuntimeCallInfo *argv);
};

class BuiltinsBigIntJson : public base::BuiltinsBase {
public:
    static JSTaggedValue Parse(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Stringify(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_JSON_H
