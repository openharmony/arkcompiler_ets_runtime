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

let installSetter = false;
let invalidatedInsideCall = false;
let setterReceiver = null;
let setterValue = -1;
let proto = {};

function maybeInstallSetter() {
    if (installSetter) {
        Object.defineProperty(proto, "added", {
            configurable: true,
            set(value) {
                setterReceiver = this;
                setterValue = value;
            }
        });
        invalidatedInsideCall = !ArkTools.arkSteedIsCompiled(storeTransitionAfterCall);
    }
}

function storeTransitionAfterCall(receiver, value) {
    maybeInstallSetter();
    receiver.added = value;
    return value;
}

function newReceiver() {
    return Object.create(proto);
}

for (let i = 0; i < 20000; i++) {
    storeTransitionAfterCall(newReceiver(), i);
}

ArkTools.arkSteedCompileSync(storeTransitionAfterCall);
ArkTools.waitJitCompileFinish(storeTransitionAfterCall);
let ok = ArkTools.arkSteedIsCompiled(storeTransitionAfterCall);

installSetter = true;
let receiver = newReceiver();
let result = storeTransitionAfterCall(receiver, 42);
ok = ok && result === 42;
ok = ok && setterReceiver === receiver && setterValue === 42;
ok = ok && !Object.prototype.hasOwnProperty.call(receiver, "added");
ok = ok && invalidatedInsideCall;
ok = ok && !ArkTools.arkSteedIsCompiled(storeTransitionAfterCall);

print(ok ? "PASS" : "FAIL");
