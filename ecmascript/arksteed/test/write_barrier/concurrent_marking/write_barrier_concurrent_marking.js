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

//! PARAMS --enable-force-gc=true

class Holder {
    constructor() {
        this.value = null;
    }
}

function forceFullGC() {
    ArkTools.forceFullGC();
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("full", undefined, true);
    }
}

function makeNoise(seed) {
    let result = 0;
    for (let i = 0; i < 128; i++) {
        let tmp = { x: seed + i, y: [seed, i, seed + i] };
        result += tmp.x;
    }
    return result;
}

function storeDuringMarking(holder, marker) {
    holder.value = { marker: marker, payload: [marker, marker + 1, marker + 2] };
}

let holder = new Holder();
for (let i = 0; i < 20000; i++) {
    storeDuringMarking(holder, i);
}

forceFullGC();
let ok = ArkTools.getICState(storeDuringMarking, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeDuringMarking);
ArkTools.waitJitCompileFinish(storeDuringMarking);
ok = ok && ArkTools.isAOTCompiled(storeDuringMarking);

let gcId = 0;
if (ArkTools.GC !== undefined) {
    gcId = ArkTools.GC.startGC("old", undefined, false);
}

for (let i = 0; i < 4000; i++) {
    storeDuringMarking(holder, i);
    makeNoise(i);
}

if (ArkTools.GC !== undefined) {
    ArkTools.GC.waitForFinishGC(gcId);
    ArkTools.GC.startGC("full", undefined, true);
}

ok = ok && holder.value.marker === 3999 && holder.value.payload[2] === 4001;
print(ok ? "PASS" : "FAIL");
