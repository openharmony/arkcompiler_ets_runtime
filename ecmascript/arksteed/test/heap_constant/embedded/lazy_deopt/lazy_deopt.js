/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! PARAMS --enable-force-gc=true
//! METHOD storeWithHeapLiteral
//! HAS HeapConstant

let transitionReceiver = false;
let invalidatedInsideCall = false;

function maybeTransitionReceiver(receiver) {
    if (transitionReceiver) {
        ArkTools.forceFullGC();
        receiver.extra = 1;
        invalidatedInsideCall = !ArkTools.arkSteedIsCompiled(storeWithHeapLiteral);
    }
}

function storeWithHeapLiteral(receiver, value) {
    const prefix = "heap-literal:";
    receiver.value = value;
    maybeTransitionReceiver(receiver);
    receiver.value = value + 1;
    return prefix + receiver.value;
}

function newReceiver() {
    return { value: 0 };
}

for (let i = 0; i < 20000; i++) {
    storeWithHeapLiteral(newReceiver(), i);
}

ArkTools.arkSteedCompileSync(storeWithHeapLiteral);
ArkTools.waitJitCompileFinish(storeWithHeapLiteral);
let ok = ArkTools.arkSteedIsCompiled(storeWithHeapLiteral);

transitionReceiver = true;
let receiver = newReceiver();
let result = storeWithHeapLiteral(receiver, 41);
ok = ok && result === "heap-literal:42";
ok = ok && receiver.value === 42 && receiver.extra === 1;
ok = ok && invalidatedInsideCall;
ok = ok && !ArkTools.arkSteedIsCompiled(storeWithHeapLiteral);

print(ok ? "PASS" : "FAIL");
