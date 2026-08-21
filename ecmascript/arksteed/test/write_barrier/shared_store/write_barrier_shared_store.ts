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
class SharedHolder {
    value: SendableArray<number> | null = null;

    constructor(value: SendableArray<number>) {
        "use sendable";
        this.value = value;
    }
}

function storeShared(holder: SharedHolder, value: SendableArray<number>): number {
    holder.value = value;
    return holder.value[0];
}

let holder = new SharedHolder(new SendableArray<number>(0));
for (let i = 0; i < 20000; i++) {
    storeShared(holder, new SendableArray<number>(i));
}

let ok = ArkTools.getICState(storeShared, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeShared);
ArkTools.waitJitCompileFinish(storeShared);
ok = ok && ArkTools.isAOTCompiled(storeShared);

for (let i = 0; i < 2000; i++) {
    let value = new SendableArray<number>(i);
    storeShared(holder, value);
    if ((i & 127) === 0) {
        ArkTools.triggerSharedGC("shared");
    }
}

ok = ok && holder.value[0] === 1999;
print(ok ? "PASS" : "FAIL");
