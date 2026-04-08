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
declare function print(arg: any, arg1?: any): string;

// ==================== Float64Array NaN store/read ====================
function testFloat64ArrayNaN() {
    // Test 1: Store NaN in Float64Array and read back
    let buf = new ArrayBuffer(16);
    let f64 = new Float64Array(buf);
    f64[0] = NaN;
    print(isNaN(f64[0]));
    print(f64[0].toString());

    // Test 2: Construct impure NaN via Int32Array overlay, read via Float64Array
    let i32 = new Int32Array(buf);
    i32[0] = 0xFFF7FFFF;
    i32[1] = 0xFFFFFFFF;
    print(isNaN(f64[0]));
    print(f64[0].toString());

    // Test 3: Write normal NaN and verify canonical representation
    f64[0] = NaN;
    print(isNaN(f64[0]));
    print(i32[0]); // 0 (low word of canonical NaN)
    print(i32[1]); // 2146959360 (0x7FF80000, high word of canonical NaN)
}

// ==================== Float32Array NaN store/read ====================
function testFloat32ArrayNaN() {
    // Test 4: Store NaN in Float32Array and read back
    let buf = new ArrayBuffer(8);
    let f32 = new Float32Array(buf);
    f32[0] = NaN;
    print(isNaN(f32[0]));
    print(f32[0].toString());

    // Test 5: Construct impure NaN via Int32Array overlay, read via Float32Array
    let i32 = new Int32Array(buf);
    i32[0] = 0x7FBFFFFF; // signaling NaN for float32
    print(isNaN(f32[0]));
    print(f32[0].toString());

    // Test 6: Store NaN and verify canonical representation
    f32[0] = NaN;
    print(isNaN(f32[0]));
    print(i32[0]); // 2143289344 (0x7FC00000)
}

// ==================== Float64Array store impure NaN then purify ====================
function testFloat64ArrayPurifyNaN() {
    let buf = new ArrayBuffer(8);
    let i32 = new Int32Array(buf);
    let f64 = new Float64Array(buf);
    // Create impure NaN (signaling NaN: 0xFFF7000000000001) via integer overlay
    // Little-endian: i32[0] = low word, i32[1] = high word (contains exponent)
    i32[0] = 0x00000001; // mantissa_low
    i32[1] = 0xFFF70000; // sign=1, exponent=0x7FF, quiet bit=0 -> signaling NaN
    print(isNaN(f64[0]));

    // Store NaN through Float64Array (should purify)
    f64[0] = NaN;
    print(isNaN(f64[0]));
    print(i32[0]); // 0
    print(i32[1]); // 2146959360
}

// ==================== Float32Array store impure NaN then purify ====================
function testFloat32ArrayPurifyNaN() {
    let buf = new ArrayBuffer(8);
    let u32 = new Uint32Array(buf);
    let f32 = new Float32Array(buf);
    u32[0] = 0x7F800001; // NaN with payload
    print(isNaN(f32[0]));

    f32[0] = NaN;
    print(isNaN(f32[0]));
    print(u32[0]); // 2143289344
}

// ==================== DataView setFloat64/getFloat64 NaN ====================
function testDataViewFloat64NaN() {
    let buf = new ArrayBuffer(16);
    let dv = new DataView(buf);
    let i32 = new Int32Array(buf);

    // Test 9: setFloat64 NaN little-endian
    dv.setFloat64(0, NaN, true);
    print(isNaN(dv.getFloat64(0, true)));

    // Test 10: setFloat64 NaN big-endian
    dv.setFloat64(0, NaN, false);
    print(isNaN(dv.getFloat64(0, false)));

    // Test 11: impure NaN via Int32 overlay, read via DataView getFloat64
    i32[0] = 0xFFF7FFFF;
    i32[1] = 0xFFFFFFFF;
    print(isNaN(dv.getFloat64(0, true)));
    print(dv.getFloat64(0, true).toString());
}

// ==================== DataView setFloat32/getFloat32 NaN ====================
function testDataViewFloat32NaN() {
    let buf = new ArrayBuffer(16);
    let dv = new DataView(buf);
    let i32 = new Int32Array(buf);

    // Test 12: setFloat32 NaN little-endian
    dv.setFloat32(0, NaN, true);
    print(isNaN(dv.getFloat32(0, true)));

    // Test 13: setFloat32 NaN big-endian
    dv.setFloat32(0, NaN, false);
    print(isNaN(dv.getFloat32(0, false)));

    // Test 14: impure NaN via Int32 overlay, read via DataView getFloat32
    i32[0] = 0x7FBFFFFF;
    print(isNaN(dv.getFloat32(0, true)));
}

// ==================== DataView setInt32 with NaN ====================
function testDataViewInt32NaN() {
    let buf = new ArrayBuffer(16);
    let dv = new DataView(buf);
    dv.setInt32(0, NaN, true);
    print(dv.getInt32(0, true)); // 0 (NaN -> ToInt32 -> 0)
}

// ==================== GetValueFromBuffer impure NaN ====================
function testGetValueFromBufferImpureNaN() {
    let v0 = new ArrayBuffer(8);
    let v1 = new Int32Array(v0);
    v1[0] = 0xcafe0000;
    v1[1] = 0xffff0000;
    let v2 = new Float64Array(v0);
    Array.prototype.push.apply(v0, v2);
    print(Number.isNaN(v2[0]));
}

// ==================== Float64Array.set with NaN ====================
function testFloat64ArraySetNaN() {
    let src = new Float64Array([1.0, NaN, 3.0]);
    let dst = new Float64Array(3);
    dst.set(src);
    print(dst[0]);
    print(isNaN(dst[1]));
    print(dst[2]);
}

// ==================== Float32Array.set with NaN ====================
function testFloat32ArraySetNaN() {
    let src = new Float32Array([1.0, NaN, 3.0]);
    let dst = new Float32Array(3);
    dst.set(src);
    print(dst[0]);
    print(isNaN(dst[1]));
    print(dst[2]);
}

// ==================== Float64Array NaN via arithmetic ====================
function testFloat64ArrayArithmeticNaN() {
    let buf = new ArrayBuffer(24);
    let f64 = new Float64Array(buf);
    let i32 = new Int32Array(buf);

    f64[0] = 0/0;
    print(isNaN(f64[0]));
    print(i32[0]); // 0 (canonical NaN low word)
    print(i32[1]); // 2146959360 (canonical NaN high word)

    f64[1] = Infinity - Infinity;
    print(isNaN(f64[1]));

    f64[2] = Math.sqrt(-1);
    print(isNaN(f64[2]));
}

// ==================== Float32Array NaN via arithmetic ====================
function testFloat32ArrayArithmeticNaN() {
    let buf = new ArrayBuffer(12);
    let f32 = new Float32Array(buf);
    let i32 = new Int32Array(buf);

    f32[0] = 0/0;
    print(isNaN(f32[0]));
    print(i32[0]); // 2143289344 (0x7FC00000)

    f32[1] = Infinity - Infinity;
    print(isNaN(f32[1]));

    f32[2] = Math.sqrt(-1);
    print(isNaN(f32[2]));
}

// ==================== Cross-type NaN read ====================
function testCrossTypeNaNRead() {
    // Int32Array -> Float64Array (impure NaN)
    let buf = new ArrayBuffer(8);
    let i32 = new Int32Array(buf);
    let f64 = new Float64Array(buf);
    i32[0] = -5;
    i32[1] = -5;
    print(isNaN(f64[0]));
    print(f64[0].toString());
}

// ==================== Float64Array store NaN verify canonical Int32 bits ====================
function testFloat64NaNCannonicalBits() {
    let buf = new ArrayBuffer(8);
    let f64 = new Float64Array(buf);
    let i32 = new Int32Array(buf);
    f64[0] = NaN;
    print(i32[0]); // 0
    print(i32[1]); // 2146959360
}

// ==================== Float32Array store NaN verify canonical Int32 bits ====================
function testFloat32NaNCannonicalBits() {
    let buf = new ArrayBuffer(8);
    let f32 = new Float32Array(buf);
    let i32 = new Int32Array(buf);
    f32[0] = NaN;
    print(i32[0]); // 2143289344
}

// ==================== DataView setFloat64 NaN verify byte order ====================
function testDataViewFloat64NaNBytes() {
    let buf = new ArrayBuffer(8);
    let dv = new DataView(buf);
    let u8 = new Uint8Array(buf);

    // little-endian
    dv.setFloat64(0, NaN, true);
    print(isNaN(dv.getFloat64(0, true)));
    print(u8[0]); // 0
    print(u8[1]); // 0
    print(u8[2]); // 0
    print(u8[3]); // 0
    print(u8[4]); // 0
    print(u8[5]); // 0
    print(u8[6]); // 248 (0xF8)
    print(u8[7]); // 127 (0x7F)
}

// ==================== DataView setFloat32 NaN verify byte order ====================
function testDataViewFloat32NaNBytes() {
    let buf = new ArrayBuffer(4);
    let dv = new DataView(buf);
    let u8 = new Uint8Array(buf);

    // little-endian
    dv.setFloat32(0, NaN, true);
    print(isNaN(dv.getFloat32(0, true)));
    print(u8[0]); // 0
    print(u8[1]); // 0
    print(u8[2]); // 192 (0xC0)
    print(u8[3]); // 127 (0x7F)
}

// ==================== Multiple NaN patterns in Float64Array ====================
function testMultipleNaNPatternsF64() {
    let buf = new ArrayBuffer(32);
    let i32 = new Int32Array(buf);
    let f64 = new Float64Array(buf);

    // canonical quiet NaN
    i32[0] = 0x00000000;
    i32[1] = 0x7FF80000;
    print(isNaN(f64[0]));

    // signaling NaN
    i32[2] = 0x00000001;
    i32[3] = 0x7FF00000;
    print(isNaN(f64[1]));

    // negative NaN
    i32[4] = 0x00000000;
    i32[5] = 0xFFF80000;
    print(isNaN(f64[2]));

    // NaN with max payload
    i32[6] = 0xFFFFFFFF;
    i32[7] = 0x7FFFFFFF;
    print(isNaN(f64[3]));
}

// ==================== Multiple NaN patterns in Float32Array ====================
function testMultipleNaNPatternsF32() {
    let buf = new ArrayBuffer(24);
    let i32 = new Int32Array(buf);
    let f32 = new Float32Array(buf);

    // canonical quiet NaN
    i32[0] = 0x7FC00000;
    print(isNaN(f32[0]));

    // signaling NaN
    i32[1] = 0x7F800001;
    print(isNaN(f32[1]));

    // negative NaN
    i32[2] = 0xFFC00000;
    print(isNaN(f32[2]));

    // NaN with payload
    i32[3] = 0x7FBFFFFF;
    print(isNaN(f32[3]));

    // NaN with max payload
    i32[4] = 0x7FFFFFFF;
    print(isNaN(f32[4]));

    // negative NaN with max payload
    i32[5] = 0xFFFFFFFF;
    print(isNaN(f32[5]));
}

// ==================== Float64Array toReversed with NaN ====================
function testFloat64ArrayToReversedNaN() {
    let f64 = new Float64Array([1.0, NaN, 3.0]);
    let reversed = f64.toReversed();
    print(reversed[0]); // 3
    print(isNaN(reversed[1])); // true
    print(reversed[2]); // 1
}

// ==================== Float32Array toReversed with NaN ====================
function testFloat32ArrayToReversedNaN() {
    let f32 = new Float32Array([1.0, NaN, 3.0]);
    let reversed = f32.toReversed();
    print(reversed[0]); // 3
    print(isNaN(reversed[1])); // true
    print(reversed[2]); // 1
}

// ==================== Float64Array.from/of with NaN ====================
function testFloat64ArrayFromOfNaN() {
    let f64_from = Float64Array.from([1.0, NaN, 3.0]);
    print(f64_from[0]); // 1
    print(isNaN(f64_from[1])); // true
    print(f64_from[2]); // 3

    let f64_of = Float64Array.of(NaN, 1.0, NaN);
    print(isNaN(f64_of[0])); // true
    print(f64_of[1]); // 1
    print(isNaN(f64_of[2])); // true
}

// ==================== Float32Array.from/of with NaN ====================
function testFloat32ArrayFromOfNaN() {
    let f32_from = Float32Array.from([1.0, NaN, 3.0]);
    print(f32_from[0]); // 1
    print(isNaN(f32_from[1])); // true
    print(f32_from[2]); // 3

    let f32_of = Float32Array.of(NaN, 1.0, NaN);
    print(isNaN(f32_of[0])); // true
    print(f32_of[1]); // 1
    print(isNaN(f32_of[2])); // true
}

// ==================== Float64Array fill with NaN ====================
function testFloat64ArrayFillNaN() {
    let f64 = new Float64Array(3);
    f64.fill(NaN);
    print(isNaN(f64[0]));
    print(isNaN(f64[1]));
    print(isNaN(f64[2]));
}

// ==================== Float32Array fill with NaN ====================
function testFloat32ArrayFillNaN() {
    let f32 = new Float32Array(3);
    f32.fill(NaN);
    print(isNaN(f32[0]));
    print(isNaN(f32[1]));
    print(isNaN(f32[2]));
}

// ==================== SharedArrayBuffer Float64Array NaN ====================
function testSharedFloat64ArrayNaN() {
    let sab = new SharedArrayBuffer(16);
    let f64 = new Float64Array(sab);
    let i32 = new Int32Array(sab);

    f64[0] = NaN;
    print(isNaN(f64[0]));

    // impure NaN (signaling NaN) via integer overlay: 0xFFF7000000000001
    i32[0] = 0x00000001; // mantissa_low
    i32[1] = 0xFFF70000; // sign=1, exponent=0x7FF, quiet bit=0 -> signaling NaN
    print(isNaN(f64[0]));
    print(f64[0].toString());
}

// ==================== SharedArrayBuffer Float32Array NaN ====================
function testSharedFloat32ArrayNaN() {
    let sab = new SharedArrayBuffer(16);
    let f32 = new Float32Array(sab);
    let i32 = new Int32Array(sab);

    f32[0] = NaN;
    print(isNaN(f32[0]));

    // impure NaN via integer overlay
    i32[0] = 0x7F800001;
    print(isNaN(f32[0]));
}

// ==================== Float64Array subarray with NaN ====================
function testFloat64ArraySubarrayNaN() {
    let buf = new ArrayBuffer(32);
    let f64 = new Float64Array(buf);
    f64[0] = NaN;
    f64[1] = 1.5;
    f64[2] = NaN;
    f64[3] = 2.5;

    let sub = f64.subarray(1, 3);
    print(sub[0]); // 1.5
    print(isNaN(sub[1])); // true
}

// ==================== Float32Array slice with NaN ====================
function testFloat32ArraySliceNaN() {
    let buf = new ArrayBuffer(24);
    let f32 = new Float32Array(buf);
    f32[0] = 1.0;
    f32[1] = NaN;
    f32[2] = 3.0;

    let sliced = f32.slice(0, 3);
    print(sliced[0]); // 1
    print(isNaN(sliced[1])); // true
    print(sliced[2]); // 3
}

// ==================== Run all tests ====================
testFloat64ArrayNaN();
testFloat32ArrayNaN();
testFloat64ArrayPurifyNaN();
testFloat32ArrayPurifyNaN();
testDataViewFloat64NaN();
testDataViewFloat32NaN();
testDataViewInt32NaN();
testGetValueFromBufferImpureNaN();
testFloat64ArraySetNaN();
testFloat32ArraySetNaN();
testFloat64ArrayArithmeticNaN();
testFloat32ArrayArithmeticNaN();
testCrossTypeNaNRead();
testFloat64NaNCannonicalBits();
testFloat32NaNCannonicalBits();
testDataViewFloat64NaNBytes();
testDataViewFloat32NaNBytes();
testMultipleNaNPatternsF64();
testMultipleNaNPatternsF32();
testFloat64ArrayToReversedNaN();
testFloat32ArrayToReversedNaN();
testFloat64ArrayFromOfNaN();
testFloat32ArrayFromOfNaN();
testFloat64ArrayFillNaN();
testFloat32ArrayFillNaN();
testSharedFloat64ArrayNaN();
testSharedFloat32ArrayNaN();
testFloat64ArraySubarrayNaN();
testFloat32ArraySliceNaN();
