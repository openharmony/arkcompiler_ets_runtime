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
// Deeply nested loops with cross-level variable usage
function deep_nested_cross_use(n, p, q)
{
    let result = 0;

    for (let i = 0; i < n; i++) {
        let temp_i = i * p + q;
        for (let j = 0; j < n; j++) {
            let temp_j = j * p + q;
            for (let k = 0; k < n; k++) {
                // Use variables from all outer loops
                result += i + j + k;
                result += temp_i * k;
                result += temp_j * i;
            }
        }
    }

    return result;
}

ArkTools.arkSteedCompileSync(deep_nested_cross_use);


print(deep_nested_cross_use(2, 3, 1));
print(deep_nested_cross_use(3, 5, 2));
print(deep_nested_cross_use(4, 7, 3));
