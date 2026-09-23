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

let transitionReceiver = false;
let invalidatedInsideCall = false;

function maybeTransitionReceiver(receiver) {
    if (transitionReceiver) {
        receiver.extra = 1;
        invalidatedInsideCall = !ArkTools.arkSteedIsCompiled(storeAroundCall);
    }
}

function storeAroundCall(receiver, value) {
    receiver.value = value;
    maybeTransitionReceiver(receiver);
    receiver.value = value + 1;
    return receiver.value;
}

function newReceiver() {
    return { value: 0 };
}

for (let i = 0; i < 20000; i++) {
    storeAroundCall(newReceiver(), i);
}

ArkTools.arkSteedCompileSync(storeAroundCall);
ArkTools.waitJitCompileFinish(storeAroundCall);
let ok = ArkTools.arkSteedIsCompiled(storeAroundCall);

transitionReceiver = true;
let receiver = newReceiver();
let result = storeAroundCall(receiver, 41);
ok = ok && result === 42;
ok = ok && receiver.value === 42 && receiver.extra === 1;
ok = ok && invalidatedInsideCall;
ok = ok && !ArkTools.arkSteedIsCompiled(storeAroundCall);

print(ok ? "PASS" : "FAIL");
