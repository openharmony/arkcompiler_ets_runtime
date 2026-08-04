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

let int8 = new Int8Array(8);
let uint8 = new Uint8Array(8);
let clamped = new Uint8ClampedArray(8);
let int16 = new Int16Array(8);
let uint16 = new Uint16Array(8);
let int32 = new Int32Array(8);
let uint32 = new Uint32Array(8);
let float32 = new Float32Array(8);
let float64 = new Float64Array(8);
let offHeapInt8 = new Int8Array(5000);

function storeTypedArrays(i8Value, u8Value, clampedValue, i16Value, u16Value,
                          i32Value, u32Value, f32Value, f64Value, offHeapValue) {
    int8[1] = i8Value;
    uint8[1] = u8Value;
    clamped[1] = clampedValue;
    int16[1] = i16Value;
    uint16[1] = u16Value;
    int32[1] = i32Value;
    uint32[1] = u32Value;
    float32[1] = f32Value;
    float64[1] = f64Value;
    offHeapInt8[4097] = offHeapValue;
}

for (let i = 0; i < 20; i++) {
    storeTypedArrays(i, i, i, i, i, i, i, i, i, i);
}
ArkTools.arkSteedCompileSync(storeTypedArrays);

storeTypedArrays(-130, 258, 300, -32769, 65538,
                 2147483648, 4294967295, 1.25, Number.NaN, -131);
print(int8[1]);
print(uint8[1]);
print(clamped[1]);
storeTypedArrays(-130, 258, -20, -32769, 65538,
                 2147483648, 4294967295, 1.25, Number.NaN, -131);
print(clamped[1]);
print(int16[1]);
print(uint16[1]);
print(int32[1]);
print(uint32[1]);
print(float32[1]);
print(float64[1]);
print(offHeapInt8[4097]);

storeTypedArrays(Infinity, Infinity, Infinity, Infinity, Infinity,
                 Infinity, Infinity, Infinity, Infinity, Infinity);
print(uint32[1]);
storeTypedArrays(-Infinity, -Infinity, -Infinity, -Infinity, -Infinity,
                 -Infinity, -Infinity, -Infinity, -Infinity, -Infinity);
print(int8[1]);
storeTypedArrays(Number.NaN, Number.NaN, Number.NaN, Number.NaN, Number.NaN,
                 Number.NaN, Number.NaN, Number.NaN, Number.NaN, Number.NaN);
print(uint8[1]);
storeTypedArrays(0, 0, 1.5, 0, 0, 0, 0, 0, 0, 0);
print(clamped[1]);
storeTypedArrays(0, 0, 2.5, 0, 0, 0, 0, 0, 0, 0);
print(clamped[1]);
storeTypedArrays(0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0);
print(clamped[1]);
storeTypedArrays(0, 0, 0.5001, 0, 0, 0, 0, 0, 0, 0);
print(clamped[1]);
storeTypedArrays(0, 0, 254.5, 0, 0, 0, 0, 0, 0, 0);
print(clamped[1]);
storeTypedArrays(0, 0, 254.5001, 0, 0, 0, 0, 0, 0, 0);
print(clamped[1]);
storeTypedArrays(0, 0, Number.NaN, 0, 0, 0, 0, 0, 0, 0);
print(clamped[1]);
