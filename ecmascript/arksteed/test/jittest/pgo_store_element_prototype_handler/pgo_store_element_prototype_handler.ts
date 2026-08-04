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

let indexedPrototype = [10, 20, 30];

function makeReceiver() {
    let receiver = [1, , 3];
    Object.setPrototypeOf(receiver, indexedPrototype);
    return receiver;
}

function storeInheritedElement(receiver, value) {
    receiver[1] = value;
}

for (let i = 0; i < 20; i++) {
    storeInheritedElement(makeReceiver(), i + 0.5);
}

ArkTools.arkSteedCompileSync(storeInheritedElement);
ArkTools.waitJitCompileFinish(storeInheritedElement);
let compiled = ArkTools.arkSteedIsCompiled(storeInheritedElement);
let receiver = makeReceiver();
storeInheritedElement(receiver, 41.5);

print(compiled);
print(receiver[1]);
print(Object.prototype.hasOwnProperty.call(receiver, "1"));
