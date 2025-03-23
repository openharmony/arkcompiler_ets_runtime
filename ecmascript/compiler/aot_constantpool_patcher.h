/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_AOT_CONSTANTPOOL_PATCHER_H
#define ECMASCRIPT_COMPILER_AOT_CONSTANTPOOL_PATCHER_H

#include "ecmascript/js_handle.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/transitions_dictionary.h"

namespace panda::ecmascript {
class ConstantPool;
class AotConstantpoolPatcher {
public:
    static void SetObjectFunctionFromConstPool(JSThread *thread, JSHandle<ConstantPool> newConstPool);
    static void SetPrototypeForTransitions(JSThread *thread, JSHClass *hclass, JSTaggedValue proto);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_AOT_CONSTANTPOOL_PATCHER_H
