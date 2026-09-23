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

let named = { x: 0 };
let polyArray = [0, 1, 2];
let polyTypedArray = new Int8Array(3);
let bigInts = new BigInt64Array(3);
let dictionaryArray = [];
dictionaryArray[100000] = 1;

function storeFallback(mode, receiver, key, value) {
    if (mode === 0) {
        receiver[key] = value;
        return receiver[key];
    }
    if (mode === 1) {
        receiver[key] = value;
        return receiver[key];
    }
    if (mode === 2) {
        receiver[key] = value;
        return receiver.length;
    }
    if (mode === 3) {
        receiver[key] = value;
        return receiver[key];
    }
    if (mode === 4) {
        receiver[key] = value;
        return receiver[key];
    }
    receiver[key] = value;
    return receiver[key];
}

for (let i = 0; i < 20; i++) {
    storeFallback(0, named, "x", i);
    storeFallback(1, (i & 1) === 0 ? polyArray : polyTypedArray, 1, i);
    storeFallback(2, [0], 2, i);
    storeFallback(3, bigInts, 1, BigInt(i));
    storeFallback(4, dictionaryArray, 100000, i);
}
ArkTools.arkSteedCompileSync(storeFallback);

print(storeFallback(0, named, "x", 51));
print(storeFallback(1, polyArray, 1, 52));
print(storeFallback(1, polyTypedArray, 1, 53));
print(storeFallback(2, [0], 3, 54));
print(storeFallback(3, bigInts, 1, 55n) === 55n);
print(storeFallback(4, dictionaryArray, 100000, 56));
print(storeFallback(5, [0, 1], 1, 57));
