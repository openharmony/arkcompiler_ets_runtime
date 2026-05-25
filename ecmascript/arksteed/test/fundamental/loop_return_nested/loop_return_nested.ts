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
function loop_return_nested(a, b, target_i, target_j)
{
    let sum = 0;
    for (let i = 0; i < a; i++) {
        for (let j = 0; j < b; j++) {
            sum += (i + 1) * (j + 1);
            if (i == target_i && j == target_j) {
                return i * 100 + j;
            }
            sum += (i + 1) + (j + 1);
        }
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_return_nested);


print(loop_return_nested(8, 8, 2, 3));
print(loop_return_nested(5, 10, 4, 9));
print(loop_return_nested(6, 6, 5, 5));
