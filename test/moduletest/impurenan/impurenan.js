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

/*
 * @tc.name:impurenan
 * @tc.desc:test impure NaN purification in TypedArray, DataView and ArrayBuffer
 * @tc.type: FUNC
 * @tc.require: issueIBDK44
 */

// ==================== Float64Array store NaN -> canonical 0x7FF8000000000000 ====================
{
    // Write NaN via Float64Array, verify canonical NaN bits via Int32Array overlay
    let buf = new ArrayBuffer(8);
    let f64 = new Float64Array(buf);
    let i32 = new Int32Array(buf);
    f64[0] = NaN;
    assert_equal(Number.isNaN(f64[0]), true);
    // canonical NaN: low 32 bits = 0x00000000, high 32 bits = 0x7FF80000
    assert_equal(i32[0], 0);
    assert_equal(i32[1], 2146959360);
}

// ==================== Float64Array impure NaN -> read purifies to canonical ====================
{
    // Construct impure NaN (signaling NaN) via Int32Array, then read via Float64Array
    let buf = new ArrayBuffer(8);
    let i32 = new Int32Array(buf);
    let f64 = new Float64Array(buf);
    // signaling NaN: 0xFFF7000000000001
    i32[0] = 0x00000001;
    i32[1] = 0xFFF70000;
    assert_equal(Number.isNaN(f64[0]), true);
    assert_equal(f64[0].toString(), "NaN");
}

// ==================== Float64Array impure NaN -> write NaN purifies buffer ====================
{
    // Write impure NaN (signaling NaN: 0xFFF7000000000001) via Int32, then overwrite with NaN via Float64Array
    // Little-endian: i32[0] = low word, i32[1] = high word (contains exponent)
    // For NaN, i32[1] must have exponent bits (bits 30-20) all set = 0x7FF
    let buf = new ArrayBuffer(8);
    let i32 = new Int32Array(buf);
    let f64 = new Float64Array(buf);
    i32[0] = 0x00000001; // mantissa_low
    i32[1] = 0xFFF70000; // sign=1, exponent=0x7FF, mantissa_high with quiet bit=0 -> signaling NaN
    assert_equal(Number.isNaN(f64[0]), true);

    f64[0] = NaN;
    assert_equal(Number.isNaN(f64[0]), true);
    // After store, buffer should have canonical NaN bits
    assert_equal(i32[0], 0);
    assert_equal(i32[1], 2146959360);
}

// ==================== Float32Array store NaN -> canonical 0x7FC00000 ====================
{
    let buf = new ArrayBuffer(8);
    let f32 = new Float32Array(buf);
    let i32 = new Int32Array(buf);
    f32[0] = NaN;
    assert_equal(Number.isNaN(f32[0]), true);
    // canonical float32 NaN: 0x7FC00000 = 2143289344
    assert_equal(i32[0], 2143289344);
}

// ==================== Float32Array impure NaN -> read purifies ====================
{
    let buf = new ArrayBuffer(8);
    let i32 = new Int32Array(buf);
    let f32 = new Float32Array(buf);
    // signaling NaN for float32: 0x7FBFFFFF
    i32[0] = 0x7FBFFFFF;
    assert_equal(Number.isNaN(f32[0]), true);
    assert_equal(f32[0].toString(), "NaN");
}

// ==================== Float32Array impure NaN -> write NaN purifies buffer ====================
{
    let buf = new ArrayBuffer(8);
    let u32 = new Uint32Array(buf);
    let f32 = new Float32Array(buf);
    // NaN with payload: 0x7F800001
    u32[0] = 0x7F800001;
    assert_equal(Number.isNaN(f32[0]), true);

    f32[0] = NaN;
    assert_equal(Number.isNaN(f32[0]), true);
    // After store, buffer should have canonical NaN bits
    assert_equal(u32[0], 2143289344);
}

// ==================== DataView setFloat64 NaN -> canonical 0x7FF8000000000000 ====================
{
    let buf = new ArrayBuffer(8);
    let dv = new DataView(buf);
    let i32 = new Int32Array(buf);
    dv.setFloat64(0, NaN, true);
    assert_equal(Number.isNaN(dv.getFloat64(0, true)), true);
    // canonical NaN bits (little-endian)
    assert_equal(i32[0], 0);
    assert_equal(i32[1], 2146959360);
}

// ==================== DataView setFloat64 NaN big-endian -> canonical ====================
{
    let buf = new ArrayBuffer(8);
    let dv = new DataView(buf);
    let i32 = new Int32Array(buf);
    dv.setFloat64(0, NaN, false);
    assert_equal(Number.isNaN(dv.getFloat64(0, false)), true);
    // DataView writes BE bytes: 7F F8 00 00 | 00 00 00 00
    // Int32Array reads in native LE: i32[0] = LE(7F F8 00 00) = 0x0000F87F = 63615
    assert_equal(i32[0], 63615);
    assert_equal(i32[1], 0);
}

// ==================== DataView setFloat32 NaN -> canonical 0x7FC00000 ====================
{
    let buf = new ArrayBuffer(4);
    let dv = new DataView(buf);
    let i32 = new Int32Array(buf);
    dv.setFloat32(0, NaN, true);
    assert_equal(Number.isNaN(dv.getFloat32(0, true)), true);
    assert_equal(i32[0], 2143289344);
}

// ==================== DataView setFloat32 NaN big-endian -> canonical ====================
{
    let buf = new ArrayBuffer(4);
    let dv = new DataView(buf);
    let i32 = new Int32Array(buf);
    dv.setFloat32(0, NaN, false);
    assert_equal(Number.isNaN(dv.getFloat32(0, false)), true);
    // DataView writes BE bytes: 7F C0 00 00
    // Int32Array reads in native LE: i32[0] = LE(7F C0 00 00) = 0x0000C07F = 49279
    assert_equal(i32[0], 49279);
}

// ==================== DataView getFloat64 reads impure NaN correctly ====================
{
    let buf = new ArrayBuffer(8);
    let i32 = new Int32Array(buf);
    let dv = new DataView(buf);
    // signaling NaN: 0x7FF0000000000001
    i32[0] = 0x00000001;
    i32[1] = 0x7FF00000;
    assert_equal(Number.isNaN(dv.getFloat64(0, true)), true);
    assert_equal(dv.getFloat64(0, true).toString(), "NaN");
}

// ==================== DataView getFloat32 reads impure NaN correctly ====================
{
    let buf = new ArrayBuffer(4);
    let i32 = new Int32Array(buf);
    let dv = new DataView(buf);
    i32[0] = 0x7FBFFFFF;
    assert_equal(Number.isNaN(dv.getFloat32(0, true)), true);
}

// ==================== DataView setInt32 with NaN -> 0 ====================
{
    let buf = new ArrayBuffer(4);
    let dv = new DataView(buf);
    dv.setInt32(0, NaN, true);
    assert_equal(dv.getInt32(0, true), 0);
}

// ==================== GetValueFromBuffer impure NaN (overflow double) ====================
{
    let v0 = new ArrayBuffer(8);
    let v1 = new Int32Array(v0);
    v1[0] = 0xcafe0000;
    v1[1] = 0xffff0000;
    let v2 = new Float64Array(v0);
    Array.prototype.push.apply(v0, v2);
    assert_equal(Number.isNaN(v2[0]), true);
}

// ==================== Float64Array.set() with NaN ====================
{
    let src = new Float64Array([1.0, NaN, 3.0]);
    let dst = new Float64Array(3);
    dst.set(src);
    assert_equal(dst[0], 1.0);
    assert_equal(Number.isNaN(dst[1]), true);
    assert_equal(dst[2], 3.0);
}

// ==================== Float32Array.set() with NaN ====================
{
    let src = new Float32Array([1.0, NaN, 3.0]);
    let dst = new Float32Array(3);
    dst.set(src);
    assert_equal(dst[0], 1.0);
    assert_equal(Number.isNaN(dst[1]), true);
    assert_equal(dst[2], 3.0);
}

// ==================== SharedArrayBuffer Float64Array NaN ====================
{
    let sab = new SharedArrayBuffer(16);
    let f64 = new Float64Array(sab);
    let i32 = new Int32Array(sab);
    f64[0] = NaN;
    assert_equal(Number.isNaN(f64[0]), true);
    assert_equal(i32[0], 0);
    assert_equal(i32[1], 2146959360);
}

// ==================== SharedArrayBuffer Float32Array NaN ====================
{
    let sab = new SharedArrayBuffer(16);
    let f32 = new Float32Array(sab);
    let i32 = new Int32Array(sab);
    f32[0] = NaN;
    assert_equal(Number.isNaN(f32[0]), true);
    assert_equal(i32[0], 2143289344);
}

// ==================== SharedArrayBuffer impure NaN read ====================
{
    let sab = new SharedArrayBuffer(16);
    let i32 = new Int32Array(sab);
    let f64 = new Float64Array(sab);
    // Little-endian: i32[0] = low word (mantissa_low), i32[1] = high word (sign+exponent)
    // signaling NaN: 0xFFF7000000000001
    i32[0] = 0x00000001;
    i32[1] = 0xFFF70000;
    assert_equal(Number.isNaN(f64[0]), true);
    assert_equal(f64[0].toString(), "NaN");
}

// ==================== Float64Array NaN via arithmetic -> canonical ====================
{
    let buf = new ArrayBuffer(16);
    let f64 = new Float64Array(buf);
    let i32 = new Int32Array(buf);

    f64[0] = 0/0;
    assert_equal(Number.isNaN(f64[0]), true);
    assert_equal(i32[0], 0);
    assert_equal(i32[1], 2146959360);

    f64[1] = Infinity - Infinity;
    assert_equal(Number.isNaN(f64[1]), true);

    f64[2] = Math.sqrt(-1);
    assert_equal(Number.isNaN(f64[2]), false);
}

// ==================== Float32Array NaN via arithmetic -> canonical ====================
{
    let buf = new ArrayBuffer(12);
    let f32 = new Float32Array(buf);
    let i32 = new Int32Array(buf);

    f32[0] = 0/0;
    assert_equal(Number.isNaN(f32[0]), true);
    assert_equal(i32[0], 2143289344);

    f32[1] = Infinity - Infinity;
    assert_equal(Number.isNaN(f32[1]), true);

    f32[2] = Math.sqrt(-1);
    assert_equal(Number.isNaN(f32[2]), true);
}

// ==================== Cross-type read: Int32Array -> Float64Array (impure NaN) ====================
{
    let buf = new ArrayBuffer(8);
    let i32 = new Int32Array(buf);
    let f64 = new Float64Array(buf);
    i32[0] = -5;
    i32[1] = -5;
    assert_equal(Number.isNaN(f64[0]), true);
    assert_equal(f64[0].toString(), "NaN");
}

// ==================== Float64Array toReversed with NaN ====================
{
    let f64 = new Float64Array([1.0, NaN, 3.0]);
    let reversed = f64.toReversed();
    assert_equal(reversed[0], 3.0);
    assert_equal(Number.isNaN(reversed[1]), true);
    assert_equal(reversed[2], 1.0);
}

// ==================== Float32Array toReversed with NaN ====================
{
    let f32 = new Float32Array([1.0, NaN, 3.0]);
    let reversed = f32.toReversed();
    assert_equal(reversed[0], 3.0);
    assert_equal(Number.isNaN(reversed[1]), true);
    assert_equal(reversed[2], 1.0);
}

// ==================== Float64Array.from / Float64Array.of with NaN ====================
{
    let f64_from = Float64Array.from([1.0, NaN, 3.0]);
    assert_equal(f64_from[0], 1.0);
    assert_equal(Number.isNaN(f64_from[1]), true);
    assert_equal(f64_from[2], 3.0);

    let f64_of = Float64Array.of(NaN, 1.0, NaN);
    assert_equal(Number.isNaN(f64_of[0]), true);
    assert_equal(f64_of[1], 1.0);
    assert_equal(Number.isNaN(f64_of[2]), true);
}

// ==================== Float32Array.from / Float32Array.of with NaN ====================
{
    let f32_from = Float32Array.from([1.0, NaN, 3.0]);
    assert_equal(f32_from[0], 1.0);
    assert_equal(Number.isNaN(f32_from[1]), true);
    assert_equal(f32_from[2], 3.0);

    let f32_of = Float32Array.of(NaN, 1.0, NaN);
    assert_equal(Number.isNaN(f32_of[0]), true);
    assert_equal(f32_of[1], 1.0);
    assert_equal(Number.isNaN(f32_of[2]), true);
}

// ==================== Float64Array fill with NaN -> canonical bits ====================
{
    let f64 = new Float64Array(3);
    let i32 = new Int32Array(f64.buffer);
    f64.fill(NaN);
    assert_equal(Number.isNaN(f64[0]), true);
    assert_equal(i32[0], 0);
    assert_equal(i32[1], 2146959360);
}

// ==================== Float32Array fill with NaN -> canonical bits ====================
{
    let f32 = new Float32Array(3);
    let i32 = new Int32Array(f32.buffer);
    f32.fill(NaN);
    assert_equal(Number.isNaN(f32[0]), true);
    assert_equal(i32[0], 2143289344);
}

// ==================== Float64Array subarray with NaN ====================
{
    let buf = new ArrayBuffer(32);
    let f64 = new Float64Array(buf);
    f64[0] = NaN;
    f64[1] = 1.5;
    f64[2] = NaN;
    f64[3] = 2.5;
    let sub = f64.subarray(1, 3);
    assert_equal(sub[0], 1.5);
    assert_equal(Number.isNaN(sub[1]), true);
}

// ==================== Float32Array slice with NaN ====================
{
    let buf = new ArrayBuffer(24);
    let f32 = new Float32Array(buf);
    f32[0] = 1.0;
    f32[1] = NaN;
    f32[2] = 3.0;
    let sliced = f32.slice(0, 3);
    assert_equal(sliced[0], 1.0);
    assert_equal(Number.isNaN(sliced[1]), true);
    assert_equal(sliced[2], 3.0);
}

// ==================== Multiple impure NaN patterns -> all read as NaN ====================
{
    let buf = new ArrayBuffer(32);
    let i32 = new Int32Array(buf);
    let f64 = new Float64Array(buf);

    // Pattern 1: canonical quiet NaN
    i32[0] = 0x00000000;
    i32[1] = 0x7FF80000;
    assert_equal(Number.isNaN(f64[0]), true);

    // Pattern 2: signaling NaN
    i32[2] = 0x00000001;
    i32[3] = 0x7FF00000;
    assert_equal(Number.isNaN(f64[1]), true);

    // Pattern 3: negative NaN
    i32[4] = 0x00000000;
    i32[5] = 0xFFF80000;
    assert_equal(Number.isNaN(f64[2]), true);

    // Pattern 4: NaN with max payload
    i32[6] = 0xFFFFFFFF;
    i32[7] = 0x7FFFFFFF;
    assert_equal(Number.isNaN(f64[3]), true);
}

// ==================== Multiple impure NaN patterns for Float32 -> all read as NaN ====================
{
    let buf = new ArrayBuffer(24);
    let i32 = new Int32Array(buf);
    let f32 = new Float32Array(buf);

    // canonical quiet NaN
    i32[0] = 0x7FC00000;
    assert_equal(Number.isNaN(f32[0]), true);

    // signaling NaN
    i32[1] = 0x7F800001;
    assert_equal(Number.isNaN(f32[1]), true);

    // negative NaN
    i32[2] = 0xFFC00000;
    assert_equal(Number.isNaN(f32[2]), true);

    // NaN with max payload
    i32[3] = 0x7FBFFFFF;
    assert_equal(Number.isNaN(f32[3]), true);

    // NaN with max payload (all bits set except sign+exp top)
    i32[4] = 0x7FFFFFFF;
    assert_equal(Number.isNaN(f32[4]), true);

    // negative NaN with max payload
    i32[5] = 0xFFFFFFFF;
    assert_equal(Number.isNaN(f32[5]), true);
}

// ==================== Float64Array map producing NaN -> canonical bits ====================
{
    let f64 = new Float64Array([1.0, -1.0, 4.0]);
    let mapped = f64.map(x => Math.sqrt(-Math.abs(x)));
    assert_equal(Number.isNaN(mapped[0]), true);
    assert_equal(Number.isNaN(mapped[1]), true);
    assert_equal(Number.isNaN(mapped[2]), true);
    // verify canonical bits
    let i32 = new Int32Array(mapped.buffer);
    assert_equal(i32[0], 0);
    assert_equal(i32[1], 2146959360);
}

// ==================== DataView setFloat64 NaN byte verification (little-endian) ====================
{
    let buf = new ArrayBuffer(8);
    let dv = new DataView(buf);
    let u8 = new Uint8Array(buf);
    dv.setFloat64(0, NaN, true);
    assert_equal(Number.isNaN(dv.getFloat64(0, true)), true);
    // canonical NaN LE bytes: 00 00 00 00 00 00 F8 7F
    assert_equal(u8[0], 0);
    assert_equal(u8[1], 0);
    assert_equal(u8[2], 0);
    assert_equal(u8[3], 0);
    assert_equal(u8[4], 0);
    assert_equal(u8[5], 0);
    assert_equal(u8[6], 248);
    assert_equal(u8[7], 127);
}

// ==================== DataView setFloat64 NaN byte verification (big-endian) ====================
{
    let buf = new ArrayBuffer(8);
    let dv = new DataView(buf);
    let u8 = new Uint8Array(buf);
    dv.setFloat64(0, NaN, false);
    assert_equal(Number.isNaN(dv.getFloat64(0, false)), true);
    // canonical NaN BE bytes: 7F F8 00 00 00 00 00 00
    assert_equal(u8[0], 127);
    assert_equal(u8[1], 248);
    assert_equal(u8[2], 0);
    assert_equal(u8[3], 0);
    assert_equal(u8[4], 0);
    assert_equal(u8[5], 0);
    assert_equal(u8[6], 0);
    assert_equal(u8[7], 0);
}

// ==================== DataView setFloat32 NaN byte verification (little-endian) ====================
{
    let buf = new ArrayBuffer(4);
    let dv = new DataView(buf);
    let u8 = new Uint8Array(buf);
    dv.setFloat32(0, NaN, true);
    assert_equal(Number.isNaN(dv.getFloat32(0, true)), true);
    // canonical NaN LE bytes: 00 00 C0 7F
    assert_equal(u8[0], 0);
    assert_equal(u8[1], 0);
    assert_equal(u8[2], 192);
    assert_equal(u8[3], 127);
}

// ==================== DataView setFloat32 NaN byte verification (big-endian) ====================
{
    let buf = new ArrayBuffer(4);
    let dv = new DataView(buf);
    let u8 = new Uint8Array(buf);
    dv.setFloat32(0, NaN, false);
    assert_equal(Number.isNaN(dv.getFloat32(0, false)), true);
    // canonical NaN BE bytes: 7F C0 00 00
    assert_equal(u8[0], 127);
    assert_equal(u8[1], 192);
    assert_equal(u8[2], 0);
    assert_equal(u8[3], 0);
}

test_end();
