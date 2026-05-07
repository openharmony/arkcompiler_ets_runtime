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
function switch_case_return_some(x, a, b, c, d)
{
    let result = a;
    switch (x) {
        case 1:
            return a;
        case 2:
            result = b;
            break;
        case 3:
            return c;
    }
    return result + d;
}

ArkTools.arkSteedCompileAsync(switch_case_return_some);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_case_return_some(1, 100, 200, 300, 400));
print(switch_case_return_some(2, 100, 200, 300, 400));
print(switch_case_return_some(3, 100, 200, 300, 400));
print(switch_case_return_some(4, 100, 200, 300, 400));