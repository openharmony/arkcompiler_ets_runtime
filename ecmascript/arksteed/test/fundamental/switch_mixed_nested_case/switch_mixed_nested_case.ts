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
function switch_mixed_nested_case(x, y, a, b, c, d)
{
    let exprResult = 0;
    switch (x) {
        case 1:
            exprResult = a + 100;
            break;
        case 2:
            exprResult = a + 200;
            break;
        default:
            exprResult = a + 300;
            break;
    }
    let result = 0;
    switch (y) {
        case 1:
            result = exprResult + b;
            break;
        case 2:
            result = exprResult - b;
            break;
        default:
            result = exprResult * 2 + c;
            break;
    }
    return result;
}

ArkTools.arkSteedCompileAsync(switch_mixed_nested_case);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_mixed_nested_case(1, 1, 10, 5, 3, 7));
print(switch_mixed_nested_case(1, 2, 10, 5, 3, 7));
print(switch_mixed_nested_case(2, 1, 10, 5, 3, 7));
print(switch_mixed_nested_case(3, 3, 10, 5, 3, 7));