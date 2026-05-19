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
function switch_case_nested_inner(x, y, a, b, c, d)
{
    let result = a;
    switch (x) {
        case 1:
            switch (y) {
                case 1:
                    result = result + 10;
                    break;
                case 2:
                    result = result - 5;
                    break;
                default:
                    result = result * 2;
                    break;
            }
            return result + b;
        case 2:
            switch (y) {
                case 1:
                    result = result + 20;
                    break;
                default:
                    result = result - 10;
                    break;
            }
            return result * c;
        default:
            return result + d;
    }
}

ArkTools.arkSteedCompileSync(switch_case_nested_inner);


print(switch_case_nested_inner(1, 1, 10, 5, 3, 7));
print(switch_case_nested_inner(1, 2, 10, 5, 3, 7));
print(switch_case_nested_inner(1, 5, 10, 5, 3, 7));
print(switch_case_nested_inner(2, 1, 10, 5, 3, 7));
print(switch_case_nested_inner(2, 3, 10, 5, 3, 7));
print(switch_case_nested_inner(5, 1, 10, 5, 3, 7));