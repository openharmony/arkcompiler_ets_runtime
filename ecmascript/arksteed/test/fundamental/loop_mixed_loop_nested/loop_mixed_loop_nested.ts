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
// Mixed loop types: for + while + do-while nested
function mixed_loop_nested(n, p)
{
    let sum = 0;

    for (let i = 0; i < n; i++) {
        let j = 0;
        while (j < n) {
            let k = n;
            do {
                k -= p;
                sum += (i + 1) * (j + 1) * (k + 1);
            } while (k > 0);
            j++;
        }
    }

    return sum;
}

ArkTools.arkSteedCompileSync(mixed_loop_nested);


print(mixed_loop_nested(0, 1));
print(mixed_loop_nested(4, 2));
print(mixed_loop_nested(8, 1));
print(mixed_loop_nested(12, 2));
print(mixed_loop_nested(16, 1));
print(mixed_loop_nested(20, 2));
