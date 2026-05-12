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

function double(x: number): number {
    return x * 2;
}

function triple(x: number): number {
    return x * 3;
}

function processValue(x: number, useTriple: boolean): number {
    return useTriple ? triple(x) : double(x);
}

ArkTools.arkSteedCompileAsync(processValue);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(processValue(5, true));
print(processValue(5, false));
print(processValue(10, true));
print(processValue(10, false));