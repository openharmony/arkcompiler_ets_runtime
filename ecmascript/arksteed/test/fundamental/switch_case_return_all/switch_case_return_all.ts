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
function switch_case_return_all(x, a, b, c, d)
{
    switch (x) {
        case 1:
            return a;
        case 2:
            return b;
        case 3:
            return c;
        default:
            return d;
    }
}

ArkTools.arkSteedCompileSync(switch_case_return_all);


print(switch_case_return_all(1, 100, 200, 300, 400));
print(switch_case_return_all(2, 100, 200, 300, 400));
print(switch_case_return_all(3, 100, 200, 300, 400));
print(switch_case_return_all(5, 100, 200, 300, 400));