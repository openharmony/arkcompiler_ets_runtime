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

// @ts-nocheck
declare function print(arg: any): string;

function loadUint8Element(value, index) {
    return value[index];
}

const uint8 = new Uint8Array([5, 9, 250, 17]);
for (let i = 0; i < 1000; i++) {
    loadUint8Element(uint8, i % uint8.length);
}

ArkTools.arkSteedCompileSync(loadUint8Element);
print(loadUint8Element(uint8, 2));
print(loadUint8Element(uint8, uint8.length) === undefined);
print(loadUint8Element(uint8, -1) === undefined);
print(loadUint8Element(uint8, "2"));

const uint16 = new Uint16Array([100, 200, 500, 800]);
print(loadUint8Element(uint16, 2));

function loadUint32Element(value, index) {
    return value[index];
}

const uint32 = new Uint32Array([0, 0xFFFFFFFF, 7]);
for (let i = 0; i < 1000; i++) {
    loadUint32Element(uint32, i % uint32.length);
}

ArkTools.arkSteedCompileSync(loadUint32Element);
print(loadUint32Element(uint32, 1));

function loadFloat64Element(value, index) {
    return value[index];
}

const float64 = new Float64Array([1.25, -3.5, 9.75]);
for (let i = 0; i < 1000; i++) {
    loadFloat64Element(float64, i % float64.length);
}

ArkTools.arkSteedCompileSync(loadFloat64Element);
print(loadFloat64Element(float64, 1));

function loadOffHeapUint8Element(value, index) {
    return value[index];
}

const offHeapUint8 = new Uint8Array(5000);
offHeapUint8[4097] = 123;
for (let i = 0; i < 1000; i++) {
    loadOffHeapUint8Element(offHeapUint8, 4097);
}

ArkTools.arkSteedCompileSync(loadOffHeapUint8Element);
print(loadOffHeapUint8Element(offHeapUint8, 4097));

function loadDetachedUint8Element(value, index) {
    return value[index];
}

const detachedUint8 = new Uint8Array([11, 22, 33]);
for (let i = 0; i < 1000; i++) {
    loadDetachedUint8Element(detachedUint8, i % detachedUint8.length);
}

ArkTools.arkSteedCompileSync(loadDetachedUint8Element);
ArkTools.arrayBufferDetach(detachedUint8.buffer);
try {
    loadDetachedUint8Element(detachedUint8, 1);
    print(false);
} catch (error) {
    print(error instanceof TypeError);
}

function loadBigInt64Element(value, index) {
    return value[index];
}

const bigint64 = new BigInt64Array([3n, 7n]);
for (let i = 0; i < 1000; i++) {
    loadBigInt64Element(bigint64, i % bigint64.length);
}

ArkTools.arkSteedCompileSync(loadBigInt64Element);
print(loadBigInt64Element(bigint64, 1) === 7n);
