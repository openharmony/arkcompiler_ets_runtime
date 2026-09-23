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

function forceYoungGC() {
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("young", undefined, true);
        return;
    }
    ArkTools.gc();
}

function storeMixedPhi(holder, flag, marker) {
    let value = 7;
    if (flag) {
        value = { marker: marker, payload: [marker, marker + 1] };
    }
    holder.value = value;
    return holder.value;
}

let holder = new Holder();

for (let i = 0; i < 20000; i++) {
    storeMixedPhi(holder, (i & 1) === 0, i);
}

let ok = true;
ArkTools.arkSteedCompileSync(storeMixedPhi);
ArkTools.waitJitCompileFinish(storeMixedPhi);
ok = ok && ArkTools.isAOTCompiled(storeMixedPhi);

ok = ok && storeMixedPhi(holder, false, 42) === 7;
forceFullGC();
storeMixedPhi(holder, true, 99);
forceYoungGC();
ok = ok && holder.value.marker === 99 && holder.value.payload[1] === 100;

print(ok ? "PASS" : "FAIL");
