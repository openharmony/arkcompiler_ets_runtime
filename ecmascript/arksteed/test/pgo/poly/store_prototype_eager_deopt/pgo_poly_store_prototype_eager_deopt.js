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

//! PARAMS --compiler-enable-jit-lazy-deopt=false

let setterReceiver = null;
let setterValue = -1;

let proto = {};
Object.defineProperty(proto, "value", {
    configurable: true,
    set(value) {
        setterReceiver = this;
        setterValue = value;
    }
});

function storePoly(receiver, value) {
    receiver.value = value;
    return value;
}

let own = { value: 0, marker: 0 };
let inherited = Object.create(proto);
for (let i = 0; i < 20000; i++) {
    storePoly((i & 1) === 0 ? own : inherited, i);
}

ArkTools.arkSteedCompileSync(storePoly);
ArkTools.waitJitCompileFinish(storePoly);
let ok = ArkTools.isAOTCompiled(storePoly);

let ownCheck = { value: 0, marker: 1 };
ok = ok && storePoly(ownCheck, 42) === 42 && ownCheck.value === 42;

let accessorCheck = Object.create(proto);
ok = ok && storePoly(accessorCheck, 55) === 55;
ok = ok && setterReceiver === accessorCheck && setterValue === 55;

Object.defineProperty(proto, "value", {
    configurable: true,
    writable: true,
    value: 1
});
let deoptCheck = Object.create(proto);
ok = ok && storePoly(deoptCheck, 99) === 99 && deoptCheck.value === 99;
ok = ok && Object.prototype.hasOwnProperty.call(deoptCheck, "value");

print(ok ? "PASS" : "FAIL");
