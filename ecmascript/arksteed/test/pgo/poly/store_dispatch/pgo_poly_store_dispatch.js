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

class HolderA {
    constructor(value) {
        this.value = value;
    }
}

class HolderB {
    constructor(value) {
        this.padding = 0;
        this.value = value;
    }
}

class HolderC {
    constructor(value) {
        this.padding0 = 0;
        this.padding1 = 0;
        this.value = value;
    }
}

function storePoly(holder, value) {
    holder.value = value;
    return holder.value;
}

let a = new HolderA(0);
let b = new HolderB(0);
let marker = { value: 1 };

for (let i = 0; i < 20000; i++) {
    storePoly((i & 1) === 0 ? a : b, marker);
}

ArkTools.arkSteedCompileSync(storePoly);
ArkTools.waitJitCompileFinish(storePoly);
let ok = ArkTools.isAOTCompiled(storePoly);
ok = ok && storePoly(a, 1) === 1;
ok = ok && storePoly(b, 2) === 2;

let c = new HolderC(0);
ok = ok && storePoly(c, 3) === 3;
ok = ok && c.value === 3;

print(ok ? "PASS" : "FAIL");
