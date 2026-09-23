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

// @ts-nocheck
class SharedPressureHolder {
    value: SendableArray<number> | null = null;

    constructor(value: SendableArray<number>) {
        "use sendable";
        this.value = value;
    }
}

function triggerSharedGC(round: number) {
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC((round & 1) === 0 ? "shared" : "shared_full", undefined, true);
        return;
    }
    ArkTools.triggerSharedGC((round & 1) === 0 ? "shared" : "shared_full");
}

function expectedChecksum(seed: number): number {
    let sum = seed + (seed + 1) + (seed + 2) + (seed + 3);
    for (let i = 1; i <= 12; i++) {
        sum += seed + i;
    }
    sum += (seed * 3) | 0;
    return sum;
}

function storeSharedWithPressure(holder: SharedPressureHolder, value: SendableArray<number>, seed: number): number {
    let live0 = { value: seed + 1 };
    let live1 = { value: seed + 2 };
    let live2 = { value: seed + 3 };
    let live3 = { value: seed + 4 };
    let live4 = { value: seed + 5 };
    let live5 = { value: seed + 6 };
    let live6 = { value: seed + 7 };
    let live7 = { value: seed + 8 };
    let live8 = { value: seed + 9 };
    let live9 = { value: seed + 10 };
    let live10 = { value: seed + 11 };
    let live11 = { value: seed + 12 };
    let intLive = (seed * 3) | 0;

    holder.value = value;

    let current = holder.value;
    if (current !== value) {
        return -1;
    }
    let sum = current[0] + current[1] + current[2] + current[3] + intLive;
    sum += live0.value + live1.value + live2.value + live3.value;
    sum += live4.value + live5.value + live6.value + live7.value;
    sum += live8.value + live9.value + live10.value + live11.value;
    return sum;
}

let holder = new SharedPressureHolder(new SendableArray<number>(0, 1, 2, 3));
for (let i = 0; i < 20000; i++) {
    storeSharedWithPressure(holder, new SendableArray<number>(i, i + 1, i + 2, i + 3), i);
}

let ok = ArkTools.getICState(storeSharedWithPressure, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeSharedWithPressure);
ArkTools.waitJitCompileFinish(storeSharedWithPressure);
ok = ok && ArkTools.isAOTCompiled(storeSharedWithPressure);

for (let i = 0; i < 1024; i++) {
    let value = new SendableArray<number>(i, i + 1, i + 2, i + 3);
    let result = storeSharedWithPressure(holder, value, i);
    if ((i & 63) === 0) {
        triggerSharedGC(i);
    }
    if (result !== expectedChecksum(i) || holder.value !== value || holder.value[3] !== i + 3) {
        ok = false;
        break;
    }
}

triggerSharedGC(1);
ok = ok && holder.value[0] === 1023 && holder.value[3] === 1026;
print(ok ? "PASS" : "FAIL");
