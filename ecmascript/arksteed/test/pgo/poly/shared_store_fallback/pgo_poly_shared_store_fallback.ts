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
class PolySharedA {
    value: SendableArray<number> | null = null;

    constructor(value: SendableArray<number>) {
        "use sendable";
        this.value = value;
    }
}

class PolySharedB {
    padding: number = 0;
    value: SendableArray<number> | null = null;

    constructor(value: SendableArray<number>) {
        "use sendable";
        this.padding = 1;
        this.value = value;
    }
}

function storePolyShared(holder: PolySharedA | PolySharedB, value: SendableArray<number>): number {
    holder.value = value;
    return holder.value[0];
}

let a = new PolySharedA(new SendableArray<number>(0));
let b = new PolySharedB(new SendableArray<number>(0));
for (let i = 0; i < 20000; i++) {
    storePolyShared((i & 1) === 0 ? a : b, new SendableArray<number>(i));
}

ArkTools.arkSteedCompileSync(storePolyShared);
ArkTools.waitJitCompileFinish(storePolyShared);
let ok = ArkTools.isAOTCompiled(storePolyShared);
ok = ok && storePolyShared(a, new SendableArray<number>(11)) === 11;
ok = ok && storePolyShared(b, new SendableArray<number>(12)) === 12;
ok = ok && a.value[0] === 11 && b.value[0] === 12;
print(ok ? "PASS" : "FAIL");
