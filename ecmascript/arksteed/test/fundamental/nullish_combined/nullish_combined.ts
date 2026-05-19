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

function nullish_combined(a: number | null | undefined, b: number | null | undefined): number {
    let result = a ?? 100;
    result += b ?? 50;
    a ??= 200;
    b ??= 150;
    return result + a + b;
}

ArkTools.arkSteedCompileSync(nullish_combined);


print(nullish_combined(10, 20));
print(nullish_combined(null, 20));
print(nullish_combined(10, null));
print(nullish_combined(null, null));
print(nullish_combined(undefined, 20));
print(nullish_combined(10, undefined));
print(nullish_combined(undefined, undefined));
print(nullish_combined(null, undefined));
print(nullish_combined(undefined, null));