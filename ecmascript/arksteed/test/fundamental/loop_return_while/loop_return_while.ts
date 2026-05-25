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
function loop_return_while(n, target, mult)
{
    let sum = 0;
    let i = 0;
    while (i < n) {
        sum += (i + mult);
        if (i == target) {
            return i * mult;
        }
        sum += (i + mult) * 3;
        i++;
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_return_while);


print(loop_return_while(12, 4, 5));
print(loop_return_while(8, 3, 2));
print(loop_return_while(20, 10, 7));
