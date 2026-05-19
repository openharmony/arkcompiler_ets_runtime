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
function loop_return_for(n, target, offset)
{
    let sum = 0;
    for (let i = 0; i < n; i++) {
        sum += (i + offset);
        if (i == target) {
            return i;
        }
        sum += (i + offset) * 2;
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_return_for);


print(loop_return_for(10, 3, 1));
print(loop_return_for(8, 7, 2));
print(loop_return_for(15, 5, 3));
