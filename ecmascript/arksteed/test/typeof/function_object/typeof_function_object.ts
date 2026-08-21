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
function typeof_function_object_inner() {
    return 1;
}

function typeof_function_object() {
    print(typeof typeof_function_object_inner === "function" ? 1 : 0);
    let obj = { call: typeof_function_object_inner };
    print(typeof obj.call === "function" ? 1 : 0);
}

ArkTools.arkSteedCompileSync(typeof_function_object);


typeof_function_object();
