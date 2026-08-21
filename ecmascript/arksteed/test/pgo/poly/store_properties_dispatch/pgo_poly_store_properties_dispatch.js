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

function makePropertiesA() {
    let holder = {};
    holder.a0 = 0;
    holder.a1 = 1;
    holder.a2 = 2;
    holder.a3 = 3;
    holder.a4 = 4;
    holder.a5 = 5;
    holder.a6 = 6;
    holder.a7 = 7;
    holder.a8 = 8;
    holder.a9 = 9;
    holder.a10 = 10;
    holder.a11 = 11;
    holder.value = 0;
    return holder;
}

function makePropertiesB() {
    let holder = {};
    holder.b0 = 0;
    holder.b1 = 1;
    holder.b2 = 2;
    holder.b3 = 3;
    holder.b4 = 4;
    holder.b5 = 5;
    holder.b6 = 6;
    holder.b7 = 7;
    holder.b8 = 8;
    holder.b9 = 9;
    holder.b10 = 10;
    holder.b11 = 11;
    holder.b12 = 12;
    holder.b13 = 13;
    holder.b14 = 14;
    holder.b15 = 15;
    holder.value = 0;
    return holder;
}

function storePolyProperties(holder, value) {
    holder.value = value;
    return holder.value;
}

let a = makePropertiesA();
let b = makePropertiesB();
let marker = { marker: 1 };
for (let i = 0; i < 20000; i++) {
    storePolyProperties((i & 1) === 0 ? a : b, marker);
}

ArkTools.arkSteedCompileSync(storePolyProperties);
ArkTools.waitJitCompileFinish(storePolyProperties);
let ok = ArkTools.isAOTCompiled(storePolyProperties);
for (let i = 0; i < 1024; i++) {
    let value = { marker: i };
    ok = ok && storePolyProperties(a, value) === value;
    ok = ok && storePolyProperties(b, value) === value;
    if ((i & 63) === 0) {
        ArkTools.gc();
    }
}

print(ok ? "PASS" : "FAIL");
