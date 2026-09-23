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

//! METHOD storeOffHeap
//! HAS DeoptIfArrayBufferDetached
//! HAS StoreIntTypedArrayElement
function storeOffHeap(receiver, index, value) {
    receiver[index] = value;
}

let offHeap = new Int32Array(5000);
print(ArkTools.isOnHeap(offHeap));
for (let i = 0; i < 20; i++) {
    storeOffHeap(offHeap, 4097, i);
}
ArkTools.arkSteedCompileSync(storeOffHeap);
ArkTools.arrayBufferDetach(offHeap.buffer);
try {
    storeOffHeap(offHeap, 4097, 42);
    print(false);
} catch (error) {
    print(error instanceof TypeError);
}

//! METHOD storeMixedBacking
//! HAS DeoptIfArrayBufferDetached
//! HAS StoreIntTypedArrayElement
function storeMixedBacking(receiver, index, value) {
    receiver[index] = value;
}

let onHeap = new Int32Array(8);
let mixedOffHeap = new Int32Array(5000);
print(ArkTools.isOnHeap(onHeap));
print(ArkTools.isOnHeap(mixedOffHeap));
for (let i = 0; i < 20; i++) {
    if ((i & 1) === 0) {
        storeMixedBacking(onHeap, 3, i);
    } else {
        storeMixedBacking(mixedOffHeap, 4097, i);
    }
}
ArkTools.arkSteedCompileSync(storeMixedBacking);
storeMixedBacking(onHeap, 3, -777);
print(onHeap[3]);
ArkTools.arrayBufferDetach(mixedOffHeap.buffer);
try {
    storeMixedBacking(mixedOffHeap, 4097, 42);
    print(false);
} catch (error) {
    print(error instanceof TypeError);
}
