/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
function call_compiled_function_via_call_this_object_impl(this: { base: number }, x: number): number {
    return this.base + x;
}

function call_compiled_function_via_call_this_object() {
    print(call_compiled_function_via_call_this_object_impl.call({ base: 10 }, 5));
    print(call_compiled_function_via_call_this_object_impl.call({ base: -3 }, 8));
}

ArkTools.arkSteedCompileSync(call_compiled_function_via_call_this_object_impl);
ArkTools.arkSteedCompileSync(call_compiled_function_via_call_this_object);


call_compiled_function_via_call_this_object();
