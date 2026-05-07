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
function default_param_undefined_return(a?: number, b?: number, c?: number): number {
    let missing = 0;
    if (a === undefined) {
        missing++;
    }
    if (b === undefined) {
        missing++;
    }
    if (c === undefined) {
        missing++;
    }
    return missing;
}

ArkTools.arkSteedCompileAsync(default_param_undefined_return);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(default_param_undefined_return());
print(default_param_undefined_return(1));
print(default_param_undefined_return(1, 2));
print(default_param_undefined_return(1, 2, 3));
