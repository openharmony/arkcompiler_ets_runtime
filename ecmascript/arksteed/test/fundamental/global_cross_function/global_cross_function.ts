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

let sharedValue = 10;

function readShared(): number {
    return sharedValue;
}

function writeShared(value: number): number {
    sharedValue = value;
    return sharedValue;
}

function modifyShared(multiplier: number): number {
    sharedValue = sharedValue * multiplier;
    return sharedValue;
}

ArkTools.arkSteedCompileAsync(readShared);
ArkTools.arkSteedCompileAsync(writeShared);
ArkTools.arkSteedCompileAsync(modifyShared);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(readShared());
print(writeShared(20));
print(readShared());
print(modifyShared(3));
print(readShared());