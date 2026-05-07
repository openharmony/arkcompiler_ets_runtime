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

function getDefault(): number {
    return 42;
}

function processConfig(base: number | null | undefined, offset: number | null | undefined, multiplier: number | null | undefined): number {
    let b = base ?? getDefault();
    let o = offset ?? 10;
    let m = multiplier ?? 1;
    base ??= 100;
    offset ??= 50;
    multiplier ??= 2;
    return (b + (offset ?? 0)) * (multiplier ?? 1);
}

ArkTools.arkSteedCompileAsync(processConfig);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(processConfig(5, 3, 2));
print(processConfig(null, 3, 2));
print(processConfig(5, null, 2));
print(processConfig(5, 3, null));
print(processConfig(null, null, null));
print(processConfig(undefined, 3, 2));
print(processConfig(5, undefined, 2));
print(processConfig(5, 3, undefined));
print(processConfig(undefined, undefined, undefined));
print(processConfig(null, undefined, null));
print(processConfig(undefined, null, undefined));