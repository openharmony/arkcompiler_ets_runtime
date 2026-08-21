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

class Box {
    value: number;

    constructor(value: number) {
        this.value = value;
    }
}

function readTwice(box: Box): number {
    return box.value + box.value;
}

function writeThenRead(box: Box): number {
    const before = box.value;
    box.value = before + 4;
    return before * 10 + box.value;
}

function branchWrite(box: Box, update: boolean): number {
    const before = box.value;
    if (update) {
        box.value = before + 2;
    }
    return before * 10 + box.value;
}

function loopWrite(box: Box): number {
    let sum = 0;
    for (let i = 0; i < 3; i++) {
        sum += box.value;
        box.value = box.value + 1;
    }
    return sum * 10 + box.value;
}

for (let i = 0; i < 10000; i++) {
    readTwice(new Box(i));
    writeThenRead(new Box(i));
    branchWrite(new Box(i), (i & 1) === 0);
    loopWrite(new Box(i));
}

ArkTools.arkSteedCompileSync(readTwice);
ArkTools.arkSteedCompileSync(writeThenRead);
ArkTools.arkSteedCompileSync(branchWrite);
ArkTools.arkSteedCompileSync(loopWrite);

print(readTwice(new Box(3)));
print(writeThenRead(new Box(3)));
print(branchWrite(new Box(3), true));
print(branchWrite(new Box(3), false));
print(loopWrite(new Box(1)));
