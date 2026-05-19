/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str: number):string;

function ternary_multiple(a: number, b: number, c: number): number {
    let result = 0;
    result += a > 0 ? a : 0;
    result += b > 0 ? b * 10 : 0;
    result += c > 0 ? c * 100 : 0;
    return result;
}

ArkTools.arkSteedCompileSync(ternary_multiple);


print(ternary_multiple(1, 2, 3));
print(ternary_multiple(-1, 2, 3));
print(ternary_multiple(1, -2, 3));
print(ternary_multiple(1, 2, -3));
print(ternary_multiple(-1, -2, -3));