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

// @ts-nocheck
declare function print(value: any): void;

let customPrototype = [];

function makeReceiver() {
    let receiver = [0, , 2];
    Object.setPrototypeOf(receiver, customPrototype);
    return receiver;
}

function storeElement(receiver, index, value) {
    receiver[index] = value;
}

for (let i = 0; i < 20; i++) {
    storeElement(makeReceiver(), 0, i);
}

ArkTools.arkSteedCompileSync(storeElement);
ArkTools.waitJitCompileFinish(storeElement);
let compiledBeforeMutation = ArkTools.arkSteedIsCompiled(storeElement);
let setterCallCount = 0;
let setterReceiver = null;
Object.defineProperty(customPrototype, "1", {
    set(value) {
        setterCallCount++;
        setterReceiver = this;
    },
    configurable: true
});

let invalidated = !ArkTools.arkSteedIsCompiled(storeElement);
let receiver = makeReceiver();
storeElement(receiver, 1, 42);
let receiverMatched = setterReceiver === receiver;
let hasOwnElement = Object.prototype.hasOwnProperty.call(receiver, "1");
delete customPrototype[1];

print(compiledBeforeMutation);
print(invalidated);
print(setterCallCount);
print(receiverMatched);
print(hasOwnElement);
