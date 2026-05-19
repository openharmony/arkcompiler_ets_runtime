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
function loop_return_multiple(n, offset)
{
    let sum = 0;
    for (let i = 0; i < n; i++) {
        sum += (i + offset);
        if (i == 0) return 1 + offset;
        sum += (i + offset) * 2;
        if (i == 1) return 2 + offset;
        sum += (i + offset) * 3;
        if (i == 2) return 3 + offset;
        sum += (i + offset) * 4;
        if (i == 3) return 4 + offset;
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_return_multiple);


print(loop_return_multiple(8, 10));
print(loop_return_multiple(0, 5));
print(loop_return_multiple(3, 20));
print(loop_return_multiple(12, 3));
