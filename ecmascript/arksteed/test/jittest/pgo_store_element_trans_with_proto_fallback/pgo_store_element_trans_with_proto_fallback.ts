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

Object.defineProperty(Array.prototype, "1", {
    value: 20,
    writable: true,
    configurable: true
});

function storeInheritedAndTransition(receiver, value) {
    receiver[1] = value;
}

for (let i = 0; i < 20; i++) {
    let receiver = [1, , 3];
    storeInheritedAndTransition(receiver, i + 0.5);
}

ArkTools.arkSteedCompileSync(storeInheritedAndTransition);
ArkTools.waitJitCompileFinish(storeInheritedAndTransition);
let receiver = [1, , 3];
storeInheritedAndTransition(receiver, 41.5);
let storedValue = receiver[1];
let hasOwnElement = Object.prototype.hasOwnProperty.call(receiver, "1");
delete Array.prototype[1];

print(storedValue);
print(hasOwnElement);
