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
function if_elseif_else_return_some(x, a, b, c)
{
    let result = a;
    if (x > 0) {
        return result + b;
    } else if (x < 0) {
        return result - b;
    }
    result = result * 2 + c;
    return result;
}

ArkTools.arkSteedCompileAsync(if_elseif_else_return_some);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(if_elseif_else_return_some(10, 100, 20, 5));
print(if_elseif_else_return_some(-5, 100, 20, 5));
print(if_elseif_else_return_some(0, 100, 20, 5));