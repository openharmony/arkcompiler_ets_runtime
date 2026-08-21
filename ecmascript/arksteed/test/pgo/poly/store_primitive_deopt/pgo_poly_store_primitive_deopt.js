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

class PrimitiveHolderA {
    constructor(value) {
        this.value = value;
    }
}

class PrimitiveHolderB {
    constructor(value) {
        this.padding = 0;
        this.value = value;
    }
}

function storePolyPrimitive(holder, value) {
    holder.value = value;
    return holder.value;
}

let a = new PrimitiveHolderA(0);
let b = new PrimitiveHolderB(0);
for (let i = 0; i < 20000; i++) {
    storePolyPrimitive((i & 1) === 0 ? a : b, i);
}

ArkTools.arkSteedCompileSync(storePolyPrimitive);
ArkTools.waitJitCompileFinish(storePolyPrimitive);
let ok = ArkTools.isAOTCompiled(storePolyPrimitive);
let threw = false;
try {
    storePolyPrimitive(1, 7);
} catch (error) {
    threw = error instanceof TypeError;
}
ok = ok && threw;

print(ok ? "PASS" : "FAIL");
