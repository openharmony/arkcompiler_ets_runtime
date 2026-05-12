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
// Multi-loop variables in single for loop
function multi_var_for(n)
{
    let result = 0;

    // Multiple loop variables
    for (let i = 0, j = n; i < n && j >= 0; i++, j--) {
        result += (i + 1) * (j + 1);
    }

    return result;
}

ArkTools.arkSteedCompileAsync(multi_var_for);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

for (let i = 0; i < 10; i++) {
    print(multi_var_for(i));
}
