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
//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD loadUint8Element
//! HAS TypedArrayIntLoadElement
//! HAS CheckedTaggedIntToI32
//! HAS DeoptIfHClassMismatch
//! HAS_NOT HeapConstant
//! HAS LoadInt32Field
//! HAS DeoptIfInt32Condition
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

function loadInt8Element(value, index) {
    return value[index];
}

const int8 = new Int8Array([-128, -7, 127]);
for (let i = 0; i < 1000; i++) {
    loadInt8Element(int8, i % int8.length);
}

ArkTools.arkSteedCompileSync(loadInt8Element);
print(loadInt8Element(int8, 1));

function loadUint8ClampedElement(value, index) {
    return value[index];
}

const uint8Clamped = new Uint8ClampedArray([0, 129, 255]);
for (let i = 0; i < 1000; i++) {
    loadUint8ClampedElement(uint8Clamped, i % uint8Clamped.length);
}

ArkTools.arkSteedCompileSync(loadUint8ClampedElement);
print(loadUint8ClampedElement(uint8Clamped, 1));

function loadInt16Element(value, index) {
    return value[index];
}

const int16 = new Int16Array([-32768, -1234, 32767]);
for (let i = 0; i < 1000; i++) {
    loadInt16Element(int16, i % int16.length);
}

ArkTools.arkSteedCompileSync(loadInt16Element);
print(loadInt16Element(int16, 1));

function loadUint16Element(value, index) {
    return value[index];
}

const directUint16 = new Uint16Array([0, 60000, 65535]);
for (let i = 0; i < 1000; i++) {
    loadUint16Element(directUint16, i % directUint16.length);
}

ArkTools.arkSteedCompileSync(loadUint16Element);
print(loadUint16Element(directUint16, 1));

function loadInt32Element(value, index) {
    return value[index];
}

const int32 = new Int32Array([0, -2000000000, 2147483647]);
for (let i = 0; i < 1000; i++) {
    loadInt32Element(int32, i % int32.length);
}

ArkTools.arkSteedCompileSync(loadInt32Element);
print(loadInt32Element(int32, 1));

function loadUint32Element(value, index) {
    return value[index];
}

const uint32 = new Uint32Array([0, 0xFFFFFFFF, 7]);
for (let i = 0; i < 1000; i++) {
    loadUint32Element(uint32, i % uint32.length);
}

ArkTools.arkSteedCompileSync(loadUint32Element);
print(loadUint32Element(uint32, 1));

function loadFloat32Element(value, index) {
    return value[index];
}

const float32 = new Float32Array([1.5, -2.25, 9.75]);
for (let i = 0; i < 1000; i++) {
    loadFloat32Element(float32, i % float32.length);
}

ArkTools.arkSteedCompileSync(loadFloat32Element);
print(loadFloat32Element(float32, 1));

//! METHOD loadFloat64Element
//! HAS TypedArrayDoubleLoadElement
function loadFloat64Element(value, index) {
    return value[index];
}

const float64 = new Float64Array([1.25, -3.5, 9.75]);
for (let i = 0; i < 1000; i++) {
    loadFloat64Element(float64, i % float64.length);
}

ArkTools.arkSteedCompileSync(loadFloat64Element);
print(loadFloat64Element(float64, 1));

//! METHOD loadPolymorphicTypedElement
//! HAS DeoptIfHClassNotIn
//! HAS BranchIfHClassIn
//! HAS_NOT HeapConstant
function loadPolymorphicTypedElement(value, index) {
    return value[index];
}

const polyUint8 = new Uint8Array([21, 42]);
const polyFloat64 = new Float64Array([1.5, 6.25]);
for (let i = 0; i < 1000; i++) {
    loadPolymorphicTypedElement(i % 2 === 0 ? polyUint8 : polyFloat64, 1);
}

ArkTools.arkSteedCompileSync(loadPolymorphicTypedElement);
print(loadPolymorphicTypedElement(polyUint8, 1));
print(loadPolymorphicTypedElement(polyFloat64, 1));

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

//! METHOD loadBigInt64Element
//! HAS CallCommonStub GetPropertyByValue
function loadBigInt64Element(value, index) {
    return value[index];
}

const bigint64 = new BigInt64Array([3n, 7n]);
for (let i = 0; i < 1000; i++) {
    loadBigInt64Element(bigint64, i % bigint64.length);
}

ArkTools.arkSteedCompileSync(loadBigInt64Element);
print(loadBigInt64Element(bigint64, 1) === 7n);
