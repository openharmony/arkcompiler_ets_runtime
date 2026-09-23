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
class SharedCounter {
    value: number = 0;

    constructor(value: number) {
        "use sendable";
        this.value = value;
    }
}

function storeSharedPrimitive(holder: SharedCounter, value: number): number {
    holder.value = value;
    return holder.value;
}

let holder = new SharedCounter(0);
for (let i = 0; i < 20000; i++) {
    storeSharedPrimitive(holder, i);
}

let ok = ArkTools.getICState(storeSharedPrimitive, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeSharedPrimitive);
ArkTools.waitJitCompileFinish(storeSharedPrimitive);
ok = ok && ArkTools.isAOTCompiled(storeSharedPrimitive);

for (let i = 0; i < 2048; i++) {
    let result = storeSharedPrimitive(holder, i);
    if ((i & 255) === 0) {
        ArkTools.triggerSharedGC((i & 511) === 0 ? "shared" : "shared_full");
    }
    if (result !== i || holder.value !== i) {
        ok = false;
        break;
    }
}

ArkTools.triggerSharedGC("shared_full");
ok = ok && holder.value === 2047;
print(ok ? "PASS" : "FAIL");
