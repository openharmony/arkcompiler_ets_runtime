/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:dataview
 * @tc.desc:test dataview
 * @tc.type: FUNC
 * @tc.require: issue#I7NUZM
 */
const buffer = new ArrayBuffer(16);
const view = new DataView(buffer);
view.setInt32({}, 0x1337, {});
print(view.getInt32({}, {}));

try {
    var buffer1 = new ArrayBuffer(64);
    var dataview = new DataView(buffer1, 8, 24);
    dataview.setInt32(0, 1n);
} catch(e) {
    print(e)
}

const buf = new ArrayBuffer(16);
const first = new DataView(buf, 0, 8);
const second = new DataView(buf, 8);
// test setInt32
second.setInt32(0, NaN);
print(second.getInt32(0));
second.setInt32(0, 13.54);
print(second.getInt32(0));
second.setInt32(0, -413.54);
print(second.getInt32(0));
second.setInt32(1, 2147483648);
print(second.getInt32(1));
second.setInt32(1, 13.54);
print(second.getInt32(0));
second.setInt32(0, Infinity);
print(second.getInt32(0));
second.setInt32(0, 27, true);
print(second.getInt32(0));

// test setFloat32
second.setFloat32(0, NaN);
print(second.getInt32(0));
second.setFloat32(0, 13.54);
print(second.getInt32(0));
second.setFloat32(0, -413.54);
print(second.getInt32(0));
second.setFloat32(1, 2147483648);
print(second.getInt32(1));
second.setFloat32(1, 13.54);
print(second.getInt32(0));
second.setFloat32(0, Infinity);
print(second.getInt32(0));
second.setFloat32(0, 27, true);
print(second.getInt32(0));

// test setFloat64
second.setFloat64(0, NaN);
print(second.getInt32(0));
second.setFloat64(0, 13.54);
print(second.getInt32(0));
second.setFloat64(0, -413.54);
print(second.getInt32(0));
second.setFloat64(0, 2147483648);
print(second.getInt32(1));
second.setFloat64(0, 13.54);
print(second.getInt32(0));
second.setFloat64(0, Infinity);
print(second.getInt32(0));
second.setFloat64(0, 27, true);
print(second.getInt32(0));

let ab = new ArrayBuffer(0x100);
try {
    let dv1 = new DataView(ab, 0x10, 0xfffffff8);
} catch(e) {
    print(e)
}
try {
    let dv2 = new DataView(ab, -1, 0xfffffff8);
} catch(e) {
    print(e)
}
try {
    let dv3 = new DataView(ab, 2**53, 0xfffffff8);
} catch(e) {
    print(e)
}
try {
    let dv4 = new DataView(ab, 0x10, -1);
} catch(e) {
    print(e)
}
try {
    let dv5 = new DataView(ab, 0x10, 2**53);
} catch(e) {
    print(e)
}

try {
    var dv6 = new DataView(ab, 64, 14);
    dv6.setFloat64(0x7fffffff, +254, true);
} catch(e) {
    print(e)
}

// ========== Additional comprehensive tests for DataView Get methods ==========

print("=== Testing GetInt8 ===");
const buf1 = new ArrayBuffer(16);
const dv1 = new DataView(buf1);
// Set some values
dv1.setInt8(0, 127);      // max positive int8
dv1.setInt8(1, -128);     // min negative int8
dv1.setInt8(2, 0);        // zero
dv1.setInt8(3, -1);       // -1
dv1.setInt8(4, 42);       // positive value
// Read back
print(dv1.getInt8(0));    // 127
print(dv1.getInt8(1));    // -128
print(dv1.getInt8(2));    // 0
print(dv1.getInt8(3));    // -1
print(dv1.getInt8(4));    // 42

print("=== Testing GetUint8 ===");
const buf2 = new ArrayBuffer(16);
const dv2 = new DataView(buf2);
dv2.setUint8(0, 255);     // max uint8
dv2.setUint8(1, 0);       // min uint8
dv2.setUint8(2, 128);     // mid value
dv2.setUint8(3, 200);     // another value
print(dv2.getUint8(0));   // 255
print(dv2.getUint8(1));   // 0
print(dv2.getUint8(2));   // 128
print(dv2.getUint8(3));   // 200

print("=== Testing GetInt16 with endianness ===");
const buf3 = new ArrayBuffer(16);
const dv3 = new DataView(buf3);
dv3.setInt16(0, 0x1234, true);   // little-endian
dv3.setInt16(2, 0x1234, false);  // big-endian
dv3.setInt16(4, -12345, true);   // negative, little-endian
dv3.setInt16(6, -12345, false);  // negative, big-endian
print(dv3.getInt16(0, true));    // 4660 (0x1234)
print(dv3.getInt16(2, false));   // 4660 (0x1234)
print(dv3.getInt16(4, true));    // -12345
print(dv3.getInt16(6, false));   // -12345
// Test reading with wrong endianness
print(dv3.getInt16(0, false));   // different result
print(dv3.getInt16(2, true));    // different result

print("=== Testing GetUint16 with endianness ===");
const buf4 = new ArrayBuffer(16);
const dv4 = new DataView(buf4);
dv4.setUint16(0, 0xABCD, true);   // little-endian
dv4.setUint16(2, 0xABCD, false);  // big-endian
dv4.setUint16(4, 65535, true);    // max value, little-endian
dv4.setUint16(6, 0, false);       // min value, big-endian
print(dv4.getUint16(0, true));    // 43981 (0xABCD)
print(dv4.getUint16(2, false));   // 43981 (0xABCD)
print(dv4.getUint16(4, true));    // 65535
print(dv4.getUint16(6, false));   // 0

print("=== Testing GetInt32 with endianness ===");
const buf5 = new ArrayBuffer(16);
const dv5 = new DataView(buf5);
dv5.setInt32(0, 0x12345678, true);    // little-endian
dv5.setInt32(4, 0x12345678, false);   // big-endian
dv5.setInt32(8, -1234567890, true);   // negative, little-endian
print(dv5.getInt32(0, true));         // 305419896 (0x12345678)
print(dv5.getInt32(4, false));        // 305419896 (0x12345678)
print(dv5.getInt32(8, true));         // -1234567890
// Test reading with wrong endianness
print(dv5.getInt32(0, false));        // different result
print(dv5.getInt32(4, true));         // different result

print("=== Testing GetUint32 with endianness ===");
const buf6 = new ArrayBuffer(16);
const dv6_test = new DataView(buf6);
dv6_test.setUint32(0, 0xFFFFFFFF, true);   // max value, little-endian
dv6_test.setUint32(4, 0x80000000, false);  // 2^31, big-endian
dv6_test.setUint32(8, 1234567890, true);   // large value
print(dv6_test.getUint32(0, true));        // 4294967295
print(dv6_test.getUint32(4, false));       // 2147483648
print(dv6_test.getUint32(8, true));        // 1234567890

print("=== Testing GetFloat32 with endianness ===");
const buf7 = new ArrayBuffer(16);
const dv7 = new DataView(buf7);
dv7.setFloat32(0, 3.14159, true);     // little-endian
dv7.setFloat32(4, 3.14159, false);    // big-endian
dv7.setFloat32(8, -123.456, true);    // negative
print(dv7.getFloat32(0, true));       // ~3.14159
print(dv7.getFloat32(4, false));      // ~3.14159
print(dv7.getFloat32(8, true));       // ~-123.456

print("=== Testing GetFloat64 with endianness ===");
const buf8 = new ArrayBuffer(24);
const dv8 = new DataView(buf8);
dv8.setFloat64(0, 3.141592653589793, true);    // PI, little-endian
dv8.setFloat64(8, 3.141592653589793, false);   // PI, big-endian
dv8.setFloat64(16, -987.654321, true);         // negative
print(dv8.getFloat64(0, true));                // PI
print(dv8.getFloat64(8, false));               // PI
print(dv8.getFloat64(16, true));               // -987.654321

print("=== Testing special float values ===");
const buf9 = new ArrayBuffer(32);
const dv9 = new DataView(buf9);
dv9.setFloat32(0, NaN);
dv9.setFloat32(4, Infinity);
dv9.setFloat32(8, -Infinity);
dv9.setFloat32(12, 0);
dv9.setFloat32(16, -0);
print(isNaN(dv9.getFloat32(0)));      // true
print(dv9.getFloat32(4));             // Infinity
print(dv9.getFloat32(8));             // -Infinity
print(dv9.getFloat32(12));            // 0
print(dv9.getFloat32(16));            // -0

print("=== Testing boundary values ===");
const buf10 = new ArrayBuffer(32);
const dv10 = new DataView(buf10);
// Int16 boundaries
dv10.setInt16(0, 32767, true);        // max int16
dv10.setInt16(2, -32768, true);       // min int16
print(dv10.getInt16(0, true));        // 32767
print(dv10.getInt16(2, true));        // -32768
// Uint16 boundaries
dv10.setUint16(4, 0, true);           // min uint16
dv10.setUint16(6, 65535, true);       // max uint16
print(dv10.getUint16(4, true));       // 0
print(dv10.getUint16(6, true));       // 65535
// Int32 boundaries
dv10.setInt32(8, 2147483647, true);   // max int32
dv10.setInt32(12, -2147483648, true); // min int32
print(dv10.getInt32(8, true));        // 2147483647
print(dv10.getInt32(12, true));       // -2147483648

print("=== Testing mixed operations ===");
const buf11 = new ArrayBuffer(16);
const dv11 = new DataView(buf11);
// Write as int32, read as different types
dv11.setInt32(0, 0x12345678, true);
print(dv11.getUint8(0));              // 0x78 = 120
print(dv11.getUint8(1));              // 0x56 = 86
print(dv11.getUint8(2));              // 0x34 = 52
print(dv11.getUint8(3));              // 0x12 = 18
print(dv11.getUint16(0, true));       // 0x5678 = 22136
print(dv11.getUint16(2, true));       // 0x1234 = 4660

print("=== Testing offset access ===");
const buf12 = new ArrayBuffer(32);
const dv12 = new DataView(buf12, 8, 16); // offset=8, length=16
dv12.setInt32(0, 100, true);
dv12.setInt32(4, 200, true);
dv12.setInt32(8, 300, true);
print(dv12.getInt32(0, true));        // 100
print(dv12.getInt32(4, true));        // 200
print(dv12.getInt32(8, true));        // 300

print("=== Testing all Int8 values (-128 to 127) ===");
const int8Buf = new ArrayBuffer(256);
const int8View = new DataView(int8Buf);
for (let i = -128; i <= 127; i++) {
    int8View.setInt8(i + 128, i);
}
for (let i = -128; i <= 127; i++) {
    const result = int8View.getInt8(i + 128);
    if (result !== i) {
        print("Int8 mismatch at " + i + ": expected " + i + ", got " + result);
    }
}
print("Int8 full range test passed");

print("=== Testing all Uint8 values (0 to 255) ===");
const uint8Buf = new ArrayBuffer(256);
const uint8View = new DataView(uint8Buf);
for (let i = 0; i <= 255; i++) {
    uint8View.setUint8(i, i);
}
for (let i = 0; i <= 255; i++) {
    const result = uint8View.getUint8(i);
    if (result !== i) {
        print("Uint8 mismatch at " + i + ": expected " + i + ", got " + result);
    }
}
print("Uint8 full range test passed");

print("=== Testing Int16 boundary and special values ===");
const int16TestBuf = new ArrayBuffer(256);
const int16TestView = new DataView(int16TestBuf);
const int16TestValues = [
    -32768, -32767, -32766, -32765,
    -16385, -16384, -16383,
    -8193, -8192, -8191,
    -2, -1, 0, 1, 2,
    8191, 8192, 8193,
    16383, 16384, 16385,
    32765, 32766, 32767
];
let offset = 0;
for (let val of int16TestValues) {
    int16TestView.setInt16(offset, val, true);
    int16TestView.setInt16(offset + 64, val, false);
    offset += 2;
}
offset = 0;
for (let val of int16TestValues) {
    const resultLE = int16TestView.getInt16(offset, true);
    const resultBE = int16TestView.getInt16(offset + 64, false);
    print("Int16 LE: " + resultLE + " (expected: " + val + ")");
    print("Int16 BE: " + resultBE + " (expected: " + val + ")");
    offset += 2;
}

print("=== Testing Uint16 boundary and special values ===");
const uint16TestBuf = new ArrayBuffer(256);
const uint16TestView = new DataView(uint16TestBuf);
const uint16TestValues = [
    0, 1, 2, 255, 256, 257,
    8191, 8192, 8193,
    16383, 16384, 16385,
    32767, 32768, 32769,
    49151, 49152, 49153,
    65533, 65534, 65535
];
offset = 0;
for (let val of uint16TestValues) {
    uint16TestView.setUint16(offset, val, true);
    uint16TestView.setUint16(offset + 64, val, false);
    offset += 2;
}
offset = 0;
for (let val of uint16TestValues) {
    const resultLE = uint16TestView.getUint16(offset, true);
    const resultBE = uint16TestView.getUint16(offset + 64, false);
    print("Uint16 LE: " + resultLE);
    print("Uint16 BE: " + resultBE);
    offset += 2;
}

print("=== Testing Int32 boundary and special values ===");
const int32TestBuf = new ArrayBuffer(512);
const int32TestView = new DataView(int32TestBuf);
const int32TestValues = [
    -2147483648, -2147483647, -2147483646,
    -1073741825, -1073741824, -1073741823,
    -536870913, -536870912, -536870911,
    -65537, -65536, -65535,
    -257, -256, -255, -2, -1, 0, 1, 2,
    255, 256, 257,
    65535, 65536, 65537,
    536870911, 536870912, 536870913,
    1073741823, 1073741824, 1073741825,
    2147483645, 2147483646, 2147483647
];
offset = 0;
for (let val of int32TestValues) {
    int32TestView.setInt32(offset, val, true);
    int32TestView.setInt32(offset + 256, val, false);
    offset += 4;
}
offset = 0;
for (let val of int32TestValues) {
    const resultLE = int32TestView.getInt32(offset, true);
    const resultBE = int32TestView.getInt32(offset + 256, false);
    print("Int32 LE: " + resultLE);
    print("Int32 BE: " + resultBE);
    offset += 4;
}

print("=== Testing Uint32 boundary and special values ===");
const uint32TestBuf = new ArrayBuffer(512);
const uint32TestView = new DataView(uint32TestBuf);
const uint32TestValues = [
    0, 1, 2, 255, 256, 257,
    65535, 65536, 65537,
    16777215, 16777216, 16777217,
    536870911, 536870912, 536870913,
    1073741823, 1073741824, 1073741825,
    2147483647, 2147483648, 2147483649,
    3221225471, 3221225472, 3221225473,
    4294967293, 4294967294, 4294967295
];
offset = 0;
for (let val of uint32TestValues) {
    uint32TestView.setUint32(offset, val, true);
    uint32TestView.setUint32(offset + 256, val, false);
    offset += 4;
}
offset = 0;
for (let val of uint32TestValues) {
    const resultLE = uint32TestView.getUint32(offset, true);
    const resultBE = uint32TestView.getUint32(offset + 256, false);
    print("Uint32 LE: " + resultLE);
    print("Uint32 BE: " + resultBE);
    offset += 4;
}

print("=== Testing Float32 special and boundary values ===");
const float32TestBuf = new ArrayBuffer(512);
const float32TestView = new DataView(float32TestBuf);
const float32TestValues = [
    0, -0, 1, -1, 2, -2,
    0.5, -0.5, 0.25, -0.25,
    3.14159265, -3.14159265,
    2.718281828, -2.718281828,
    1.41421356, -1.41421356,
    100, -100, 1000, -1000,
    0.1, 0.01, 0.001, 0.0001,
    1.234567, 9.876543,
    123.456, -123.456,
    1e10, -1e10, 1e-10, -1e-10,
    NaN, Infinity, -Infinity
];
offset = 0;
for (let val of float32TestValues) {
    float32TestView.setFloat32(offset, val, true);
    float32TestView.setFloat32(offset + 256, val, false);
    offset += 4;
}
offset = 0;
for (let val of float32TestValues) {
    const resultLE = float32TestView.getFloat32(offset, true);
    const resultBE = float32TestView.getFloat32(offset + 256, false);
    print("Float32 LE: " + resultLE);
    print("Float32 BE: " + resultBE);
    offset += 4;
}

print("=== Testing Float64 special and boundary values ===");
const float64TestBuf = new ArrayBuffer(1024);
const float64TestView = new DataView(float64TestBuf);
const float64TestValues = [
    0, -0, 1, -1, 2, -2,
    0.5, -0.5, 0.25, -0.25,
    Math.PI, -Math.PI,
    Math.E, -Math.E,
    Math.SQRT2, -Math.SQRT2,
    Math.SQRT1_2, -Math.SQRT1_2,
    Math.LN2, Math.LN10,
    Math.LOG2E, Math.LOG10E,
    100, -100, 1000, -1000, 10000, -10000,
    0.1, 0.01, 0.001, 0.0001, 0.00001,
    1.23456789012345, 9.87654321098765,
    123.456789, -123.456789,
    1e10, -1e10, 1e20, -1e20,
    1e-10, -1e-10, 1e-20, -1e-20,
    1.7976931348623157e+308,
    2.2250738585072014e-308,
    NaN, Infinity, -Infinity
];
offset = 0;
for (let val of float64TestValues) {
    float64TestView.setFloat64(offset, val, true);
    float64TestView.setFloat64(offset + 512, val, false);
    offset += 8;
}
offset = 0;
for (let val of float64TestValues) {
    const resultLE = float64TestView.getFloat64(offset, true);
    const resultBE = float64TestView.getFloat64(offset + 512, false);
    print("Float64 LE: " + resultLE);
    print("Float64 BE: " + resultBE);
    offset += 8;
}

print("=== Testing cross-type reads (write one type, read another) ===");
const crossBuf = new ArrayBuffer(64);
const crossView = new DataView(crossBuf);

// Write Int32, read as various types
crossView.setInt32(0, 0x12345678, true);
print("Wrote Int32 0x12345678 (little-endian):");
print("  as Uint8[0]: " + crossView.getUint8(0));
print("  as Uint8[1]: " + crossView.getUint8(1));
print("  as Uint8[2]: " + crossView.getUint8(2));
print("  as Uint8[3]: " + crossView.getUint8(3));
print("  as Int8[0]: " + crossView.getInt8(0));
print("  as Int8[1]: " + crossView.getInt8(1));
print("  as Int8[2]: " + crossView.getInt8(2));
print("  as Int8[3]: " + crossView.getInt8(3));
print("  as Uint16[0] LE: " + crossView.getUint16(0, true));
print("  as Uint16[0] BE: " + crossView.getUint16(0, false));
print("  as Uint16[2] LE: " + crossView.getUint16(2, true));
print("  as Uint16[2] BE: " + crossView.getUint16(2, false));
print("  as Int32 LE: " + crossView.getInt32(0, true));
print("  as Int32 BE: " + crossView.getInt32(0, false));
print("  as Uint32 LE: " + crossView.getUint32(0, true));
print("  as Uint32 BE: " + crossView.getUint32(0, false));

// Write Float32, read as Int32
crossView.setFloat32(8, 1.5, true);
print("Wrote Float32 1.5 (little-endian):");
print("  as Int32 LE: " + crossView.getInt32(8, true));
print("  as Uint32 LE: " + crossView.getUint32(8, true));

// Write Float64, read as Int32s
crossView.setFloat64(16, Math.PI, true);
print("Wrote Float64 PI (little-endian):");
print("  as Int32[0] LE: " + crossView.getInt32(16, true));
print("  as Int32[4] LE: " + crossView.getInt32(20, true));
print("  as Uint32[0] LE: " + crossView.getUint32(16, true));
print("  as Uint32[4] LE: " + crossView.getUint32(20, true));

print("=== Testing sequential writes and reads ===");
const seqBuf = new ArrayBuffer(256);
const seqView = new DataView(seqBuf);
let seqOffset = 0;

// Write sequence
for (let i = 0; i < 16; i++) {
    seqView.setInt8(seqOffset++, i);
    seqView.setUint8(seqOffset++, i * 10);
    seqView.setInt16(seqOffset, i * 100, true);
    seqOffset += 2;
    seqView.setUint16(seqOffset, i * 1000, true);
    seqOffset += 2;
    seqView.setInt32(seqOffset, i * 10000, true);
    seqOffset += 4;
    seqView.setUint32(seqOffset, i * 100000, true);
    seqOffset += 4;
}

// Read back sequence
seqOffset = 0;
for (let i = 0; i < 16; i++) {
    const i8 = seqView.getInt8(seqOffset++);
    const ui8 = seqView.getUint8(seqOffset++);
    const i16 = seqView.getInt16(seqOffset, true);
    seqOffset += 2;
    const ui16 = seqView.getUint16(seqOffset, true);
    seqOffset += 2;
    const i32 = seqView.getInt32(seqOffset, true);
    seqOffset += 4;
    const ui32 = seqView.getUint32(seqOffset, true);
    seqOffset += 4;
    print("Seq[" + i + "]: i8=" + i8 + " ui8=" + ui8 + " i16=" + i16 + " ui16=" + ui16 + " i32=" + i32 + " ui32=" + ui32);
}

print("=== Testing endianness conversion patterns ===");
const endianBuf = new ArrayBuffer(128);
const endianView = new DataView(endianBuf);

// Pattern 1: Write little-endian, read both
const pattern1Values = [0x0001, 0x0102, 0x1234, 0xABCD, 0xFFFF];
offset = 0;
for (let val of pattern1Values) {
    endianView.setUint16(offset, val, true);
    offset += 2;
}
offset = 0;
for (let i = 0; i < pattern1Values.length; i++) {
    const le = endianView.getUint16(offset, true);
    const be = endianView.getUint16(offset, false);
    print("Pattern1 0x" + pattern1Values[i].toString(16) + ": LE=" + le + " BE=" + be);
    offset += 2;
}

// Pattern 2: 32-bit patterns
const pattern2Values = [0x00000001, 0x01020304, 0x12345678, 0xABCDEF00, 0xFFFFFFFF];
offset = 32;
for (let val of pattern2Values) {
    endianView.setUint32(offset, val, true);
    offset += 4;
}
offset = 32;
for (let i = 0; i < pattern2Values.length; i++) {
    const le = endianView.getUint32(offset, true);
    const be = endianView.getUint32(offset, false);
    print("Pattern2 0x" + pattern2Values[i].toString(16) + ": LE=" + le + " BE=" + be);
    offset += 4;
}

print("=== Testing aligned vs unaligned access ===");
const alignBuf = new ArrayBuffer(64);
const alignView = new DataView(alignBuf);

// Aligned writes (offsets 0, 4, 8, 12, ...)
for (let i = 0; i < 8; i++) {
    alignView.setInt32(i * 4, i * 1111, true);
}
// Unaligned writes (offsets 1, 5, 9, 13, ...)
for (let i = 0; i < 7; i++) {
    alignView.setInt32(i * 4 + 1 + 32, i * 2222, true);
}
// Read aligned
for (let i = 0; i < 8; i++) {
    print("Aligned[" + (i*4) + "]: " + alignView.getInt32(i * 4, true));
}
// Read unaligned
for (let i = 0; i < 7; i++) {
    print("Unaligned[" + (i*4+1+32) + "]: " + alignView.getInt32(i * 4 + 1 + 32, true));
}

print("=== Testing with DataView at different offsets ===");
const offsetBuf = new ArrayBuffer(128);
for (let i = 0; i < 128; i++) {
    const view = new DataView(offsetBuf);
    view.setUint8(i, i);
}

// Create views at different offsets
const view0 = new DataView(offsetBuf, 0, 32);
const view16 = new DataView(offsetBuf, 16, 32);
const view32 = new DataView(offsetBuf, 32, 32);
const view48 = new DataView(offsetBuf, 48, 32);

view0.setInt32(0, 1000, true);
view16.setInt32(0, 2000, true);
view32.setInt32(0, 3000, true);
view48.setInt32(0, 4000, true);

print("View offset 0, pos 0: " + view0.getInt32(0, true));
print("View offset 16, pos 0: " + view16.getInt32(0, true));
print("View offset 32, pos 0: " + view32.getInt32(0, true));
print("View offset 48, pos 0: " + view48.getInt32(0, true));

// Verify they wrote to different places
const mainView = new DataView(offsetBuf);
print("Main buffer at 0: " + mainView.getInt32(0, true));
print("Main buffer at 16: " + mainView.getInt32(16, true));
print("Main buffer at 32: " + mainView.getInt32(32, true));
print("Main buffer at 48: " + mainView.getInt32(48, true));

print("=== Testing negative values across all integer types ===");
const negBuf = new ArrayBuffer(256);
const negView = new DataView(negBuf);

const negativeTests = [
    { val: -1, name: "minus_one" },
    { val: -2, name: "minus_two" },
    { val: -127, name: "minus_127" },
    { val: -128, name: "int8_min" },
    { val: -255, name: "minus_255" },
    { val: -256, name: "minus_256" },
    { val: -32767, name: "minus_32767" },
    { val: -32768, name: "int16_min" },
    { val: -65535, name: "minus_65535" },
    { val: -65536, name: "minus_65536" },
    { val: -2147483647, name: "minus_2147483647" },
    { val: -2147483648, name: "int32_min" }
];

offset = 0;
for (let test of negativeTests) {
    negView.setInt8(offset, test.val);
    negView.setInt16(offset + 64, test.val, true);
    negView.setInt32(offset + 128, test.val, true);
    offset += 4;
}

offset = 0;
for (let test of negativeTests) {
    const i8 = negView.getInt8(offset);
    const i16 = negView.getInt16(offset + 64, true);
    const i32 = negView.getInt32(offset + 128, true);
    print("Negative " + test.name + ": i8=" + i8 + " i16=" + i16 + " i32=" + i32);
    offset += 4;
}

print("=== Testing power-of-two values ===");
const powBuf = new ArrayBuffer(256);
const powView = new DataView(powBuf);

const powerTests = [];
for (let i = 0; i < 16; i++) {
    powerTests.push(1 << i);
}

offset = 0;
for (let val of powerTests) {
    powView.setInt16(offset, val, true);
    powView.setInt32(offset + 64, val, true);
    powView.setUint32(offset + 128, val, true);
    offset += 4;
}

offset = 0;
for (let val of powerTests) {
    const i16 = powView.getInt16(offset, true);
    const i32 = powView.getInt32(offset + 64, true);
    const ui32 = powView.getUint32(offset + 128, true);
    print("2^n " + val + ": i16=" + i16 + " i32=" + i32 + " ui32=" + ui32);
    offset += 4;
}

print("=== Testing byte patterns (0x00, 0xFF, 0xAA, 0x55) ===");
const patternBuf = new ArrayBuffer(64);
const patternView = new DataView(patternBuf);

const bytePatterns = [
    { pattern: 0x00, name: "all_zeros" },
    { pattern: 0xFF, name: "all_ones" },
    { pattern: 0xAA, name: "alternating_10" },
    { pattern: 0x55, name: "alternating_01" },
    { pattern: 0xF0, name: "high_nibble" },
    { pattern: 0x0F, name: "low_nibble" }
];

offset = 0;
for (let test of bytePatterns) {
    // Fill 4 bytes with pattern
    for (let i = 0; i < 4; i++) {
        patternView.setUint8(offset + i, test.pattern);
    }
    offset += 4;
}

offset = 0;
for (let test of bytePatterns) {
    const i32_le = patternView.getInt32(offset, true);
    const i32_be = patternView.getInt32(offset, false);
    const ui32_le = patternView.getUint32(offset, true);
    const ui32_be = patternView.getUint32(offset, false);
    print("Pattern " + test.name + " (0x" + test.pattern.toString(16) + "):");
    print("  Int32 LE: " + i32_le + ", BE: " + i32_be);
    print("  Uint32 LE: " + ui32_le + ", BE: " + ui32_be);
    offset += 4;
}

print("=== Testing maximum and minimum representable values ===");
const minmaxBuf = new ArrayBuffer(128);
const minmaxView = new DataView(minmaxBuf);

// Int8
minmaxView.setInt8(0, 127);
minmaxView.setInt8(1, -128);
print("Int8 max: " + minmaxView.getInt8(0));
print("Int8 min: " + minmaxView.getInt8(1));

// Uint8
minmaxView.setUint8(2, 255);
minmaxView.setUint8(3, 0);
print("Uint8 max: " + minmaxView.getUint8(2));
print("Uint8 min: " + minmaxView.getUint8(3));

// Int16
minmaxView.setInt16(4, 32767, true);
minmaxView.setInt16(6, -32768, true);
print("Int16 max: " + minmaxView.getInt16(4, true));
print("Int16 min: " + minmaxView.getInt16(6, true));

// Uint16
minmaxView.setUint16(8, 65535, true);
minmaxView.setUint16(10, 0, true);
print("Uint16 max: " + minmaxView.getUint16(8, true));
print("Uint16 min: " + minmaxView.getUint16(10, true));

// Int32
minmaxView.setInt32(12, 2147483647, true);
minmaxView.setInt32(16, -2147483648, true);
print("Int32 max: " + minmaxView.getInt32(12, true));
print("Int32 min: " + minmaxView.getInt32(16, true));

// Uint32
minmaxView.setUint32(20, 4294967295, true);
minmaxView.setUint32(24, 0, true);
print("Uint32 max: " + minmaxView.getUint32(20, true));
print("Uint32 min: " + minmaxView.getUint32(24, true));

print("=== Testing Float32 precision boundaries ===");
const f32PrecBuf = new ArrayBuffer(128);
const f32PrecView = new DataView(f32PrecBuf);

const f32PrecTests = [
    1.0, 1.5, 1.25, 1.125, 1.0625,
    0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
    1e-5, 1e-6, 1e-7, 1e-8, 1e-9,
    1e5, 1e6, 1e7, 1e8, 1e9
];

offset = 0;
for (let val of f32PrecTests) {
    f32PrecView.setFloat32(offset, val, true);
    offset += 4;
}

offset = 0;
for (let val of f32PrecTests) {
    const result = f32PrecView.getFloat32(offset, true);
    print("Float32 " + val + " => " + result);
    offset += 4;
}

print("=== Testing Float64 precision boundaries ===");
const f64PrecBuf = new ArrayBuffer(256);
const f64PrecView = new DataView(f64PrecBuf);

const f64PrecTests = [
    1.0, 1.5, 1.25, 1.125, 1.0625, 1.03125,
    0.1, 0.01, 0.001, 0.0001, 0.00001,
    1e-10, 1e-20, 1e-30, 1e-40, 1e-50,
    1e10, 1e20, 1e30, 1e40, 1e50
];

offset = 0;
for (let val of f64PrecTests) {
    f64PrecView.setFloat64(offset, val, true);
    offset += 8;
}

offset = 0;
for (let val of f64PrecTests) {
    const result = f64PrecView.getFloat64(offset, true);
    print("Float64 " + val + " => " + result);
    offset += 8;
}

print("=== Testing consecutive overlapping writes ===");
const overlapBuf = new ArrayBuffer(64);
const overlapView = new DataView(overlapBuf);

// Write pattern
overlapView.setInt32(0, 0x11111111, true);
overlapView.setInt32(2, 0x22222222, true);  // overlaps bytes 2-5
overlapView.setInt32(4, 0x33333333, true);  // overlaps bytes 4-7

print("Overlapping writes result:");
for (let i = 0; i < 8; i++) {
    print("  Byte[" + i + "]: 0x" + overlapView.getUint8(i).toString(16));
}
print("  Int32[0] LE: 0x" + overlapView.getInt32(0, true).toString(16));
print("  Int32[2] LE: 0x" + overlapView.getInt32(2, true).toString(16));
print("  Int32[4] LE: 0x" + overlapView.getInt32(4, true).toString(16));

print("=== Testing random value patterns ===");
const randomBuf = new ArrayBuffer(512);
const randomView = new DataView(randomBuf);

// Generate and test pseudo-random values
const randomSeeds = [12345, 67890, 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888];
offset = 0;
for (let seed of randomSeeds) {
    let val = seed;
    randomView.setInt32(offset, val, true);
    randomView.setInt32(offset + 4, val * 2, false);
    randomView.setUint32(offset + 8, val * 3, true);
    randomView.setUint32(offset + 12, val * 4, false);
    randomView.setFloat32(offset + 16, val / 100, true);
    randomView.setFloat64(offset + 20, val / 1000, true);
    offset += 28;
}

offset = 0;
for (let seed of randomSeeds) {
    const i32_le = randomView.getInt32(offset, true);
    const i32_be = randomView.getInt32(offset + 4, false);
    const ui32_le = randomView.getUint32(offset + 8, true);
    const ui32_be = randomView.getUint32(offset + 12, false);
    const f32 = randomView.getFloat32(offset + 16, true);
    const f64 = randomView.getFloat64(offset + 20, true);
    print("Seed " + seed + ": i32_le=" + i32_le + " i32_be=" + i32_be + " ui32_le=" + ui32_le + " ui32_be=" + ui32_be);
    print("  f32=" + f32 + " f64=" + f64);
    offset += 28;
}

print("=== Testing incremental byte fills ===");
const fillBuf = new ArrayBuffer(256);
const fillView = new DataView(fillBuf);

// Fill with incremental pattern
for (let i = 0; i < 256; i++) {
    fillView.setUint8(i, i);
}

// Read back as different types at various offsets
for (let i = 0; i < 10; i++) {
    const off = i * 16;
    const i8 = fillView.getInt8(off);
    const ui8 = fillView.getUint8(off);
    const i16_le = fillView.getInt16(off, true);
    const i16_be = fillView.getInt16(off, false);
    const ui16_le = fillView.getUint16(off, true);
    const ui16_be = fillView.getUint16(off, false);
    const i32_le = fillView.getInt32(off, true);
    const i32_be = fillView.getInt32(off, false);
    print("Offset " + off + ":");
    print("  i8=" + i8 + " ui8=" + ui8);
    print("  i16: LE=" + i16_le + " BE=" + i16_be);
    print("  ui16: LE=" + ui16_le + " BE=" + ui16_be);
    print("  i32: LE=" + i32_le + " BE=" + i32_be);
}

print("=== Testing bit manipulation results ===");
const bitBuf = new ArrayBuffer(128);
const bitView = new DataView(bitBuf);

// Test various bit patterns
const bitPatterns = [
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
];

offset = 0;
for (let i = 0; i < 16; i++) {
    bitView.setUint32(offset, bitPatterns[i], true);
    bitView.setUint32(offset + 64, bitPatterns[i], false);
    offset += 4;
}

offset = 0;
for (let i = 0; i < 16; i++) {
    const le = bitView.getUint32(offset, true);
    const be = bitView.getUint32(offset + 64, false);
    print("Bit pattern " + i + ": LE=0x" + le.toString(16) + " BE=0x" + be.toString(16));
    offset += 4;
}

print("=== Testing arithmetic progression ===");
const progBuf = new ArrayBuffer(256);
const progView = new DataView(progBuf);

// Arithmetic progression: start=10, diff=5
let progOffset = 0;
for (let i = 0; i < 20; i++) {
    const val = 10 + i * 5;
    progView.setInt16(progOffset, val, true);
    progView.setInt32(progOffset + 40, val * 100, true);
    progView.setFloat32(progOffset + 120, val / 10.0, true);
    progOffset += 2;
}

progOffset = 0;
for (let i = 0; i < 20; i++) {
    const i16 = progView.getInt16(progOffset, true);
    const i32 = progView.getInt32(progOffset + 40, true);
    const f32 = progView.getFloat32(progOffset + 120, true);
    print("Prog[" + i + "]: i16=" + i16 + " i32=" + i32 + " f32=" + f32);
    progOffset += 2;
}

print("=== Testing geometric progression ===");
const geomBuf = new ArrayBuffer(256);
const geomView = new DataView(geomBuf);

// Geometric progression: start=1, ratio=2
let geomOffset = 0;
for (let i = 0; i < 16; i++) {
    const val = 1 << i;  // 2^i
    geomView.setUint32(geomOffset, val, true);
    geomView.setFloat64(geomOffset + 64, val, true);
    geomOffset += 4;
}

geomOffset = 0;
for (let i = 0; i < 16; i++) {
    const ui32 = geomView.getUint32(geomOffset, true);
    const f64 = geomView.getFloat64(geomOffset + 64, true);
    print("Geom[" + i + "]: ui32=" + ui32 + " f64=" + f64);
    geomOffset += 4;
}

print("=== Testing alternating positive/negative values ===");
const altBuf = new ArrayBuffer(256);
const altView = new DataView(altBuf);

offset = 0;
for (let i = 0; i < 32; i++) {
    const val = (i % 2 === 0) ? (i * 100) : -(i * 100);
    altView.setInt32(offset, val, true);
    offset += 4;
}

offset = 0;
for (let i = 0; i < 32; i++) {
    const val = altView.getInt32(offset, true);
    print("Alt[" + i + "]: " + val);
    offset += 4;
}

print("=== Testing Fibonacci-like sequence ===");
const fibBuf = new ArrayBuffer(256);
const fibView = new DataView(fibBuf);

let fib1 = 1, fib2 = 1;
offset = 0;
fibView.setInt32(offset, fib1, true);
fibView.setInt32(offset + 4, fib2, true);
offset = 8;

for (let i = 2; i < 20; i++) {
    const next = fib1 + fib2;
    fibView.setInt32(offset, next, true);
    fib1 = fib2;
    fib2 = next;
    offset += 4;
}

offset = 0;
for (let i = 0; i < 20; i++) {
    const val = fibView.getInt32(offset, true);
    print("Fib[" + i + "]: " + val);
    offset += 4;
}

print("=== Testing prime number storage ===");
const primeBuf = new ArrayBuffer(256);
const primeView = new DataView(primeBuf);

const primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71];
offset = 0;
for (let p of primes) {
    primeView.setUint16(offset, p, true);
    primeView.setUint32(offset + 40, p * p, true);
    offset += 2;
}

offset = 0;
for (let i = 0; i < primes.length; i++) {
    const p = primeView.getUint16(offset, true);
    const sq = primeView.getUint32(offset + 40, true);
    print("Prime[" + i + "]: " + p + ", square=" + sq);
    offset += 2;
}

print("=== Testing factorial approximations ===");
const factBuf = new ArrayBuffer(256);
const factView = new DataView(factBuf);

let factorial = 1;
offset = 0;
for (let i = 1; i <= 12; i++) {
    factorial *= i;
    factView.setUint32(offset, factorial, true);
    factView.setFloat64(offset + 48, factorial, true);
    offset += 4;
}

offset = 0;
for (let i = 1; i <= 12; i++) {
    const ui32 = factView.getUint32(offset, true);
    const f64 = factView.getFloat64(offset + 48, true);
    print("Factorial[" + i + "]: ui32=" + ui32 + " f64=" + f64);
    offset += 4;
}

print("=== Testing scientific notation values ===");
const sciBuf = new ArrayBuffer(512);
const sciView = new DataView(sciBuf);

const sciValues = [
    1e-50, 1e-40, 1e-30, 1e-20, 1e-10,
    1e-5, 1e-4, 1e-3, 1e-2, 1e-1,
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5,
    1e10, 1e20, 1e30, 1e40, 1e50
];

offset = 0;
for (let val of sciValues) {
    sciView.setFloat64(offset, val, true);
    offset += 8;
}

offset = 0;
for (let val of sciValues) {
    const result = sciView.getFloat64(offset, true);
    print("Scientific: " + val + " => " + result);
    offset += 8;
}

print("=== Testing mathematical constants ===");
const mathBuf = new ArrayBuffer(256);
const mathView = new DataView(mathBuf);

const mathConstants = [
    { name: "PI", val: Math.PI },
    { name: "E", val: Math.E },
    { name: "LN2", val: Math.LN2 },
    { name: "LN10", val: Math.LN10 },
    { name: "LOG2E", val: Math.LOG2E },
    { name: "LOG10E", val: Math.LOG10E },
    { name: "SQRT1_2", val: Math.SQRT1_2 },
    { name: "SQRT2", val: Math.SQRT2 }
];

offset = 0;
for (let c of mathConstants) {
    mathView.setFloat64(offset, c.val, true);
    mathView.setFloat64(offset + 64, c.val, false);
    offset += 8;
}

offset = 0;
for (let c of mathConstants) {
    const le = mathView.getFloat64(offset, true);
    const be = mathView.getFloat64(offset + 64, false);
    print("Constant " + c.name + ": LE=" + le + " BE=" + be);
    offset += 8;
}

print("=== Testing trigonometric values ===");
const trigBuf = new ArrayBuffer(512);
const trigView = new DataView(trigBuf);

offset = 0;
for (let deg = 0; deg <= 360; deg += 30) {
    const rad = deg * Math.PI / 180;
    const sinVal = Math.sin(rad);
    const cosVal = Math.cos(rad);
    const tanVal = Math.tan(rad);
    trigView.setFloat64(offset, sinVal, true);
    trigView.setFloat64(offset + 8, cosVal, true);
    trigView.setFloat64(offset + 16, tanVal, true);
    offset += 24;
}

offset = 0;
for (let deg = 0; deg <= 360; deg += 30) {
    const sinVal = trigView.getFloat64(offset, true);
    const cosVal = trigView.getFloat64(offset + 8, true);
    const tanVal = trigView.getFloat64(offset + 16, true);
    print("Angle " + deg + "deg: sin=" + sinVal + " cos=" + cosVal + " tan=" + tanVal);
    offset += 24;
}

print("=== Testing square roots ===");
const sqrtBuf = new ArrayBuffer(256);
const sqrtView = new DataView(sqrtBuf);

offset = 0;
for (let i = 1; i <= 25; i++) {
    const sqrtVal = Math.sqrt(i);
    sqrtView.setFloat64(offset, sqrtVal, true);
    offset += 8;
}

offset = 0;
for (let i = 1; i <= 25; i++) {
    const result = sqrtView.getFloat64(offset, true);
    print("sqrt(" + i + ") = " + result);
    offset += 8;
}

print("=== Testing logarithmic values ===");
const logBuf = new ArrayBuffer(512);
const logView = new DataView(logBuf);

offset = 0;
for (let i = 1; i <= 20; i++) {
    const lnVal = Math.log(i);
    const log10Val = Math.log10(i);
    const log2Val = Math.log2(i);
    logView.setFloat64(offset, lnVal, true);
    logView.setFloat64(offset + 8, log10Val, true);
    logView.setFloat64(offset + 16, log2Val, true);
    offset += 24;
}

offset = 0;
for (let i = 1; i <= 20; i++) {
    const ln = logView.getFloat64(offset, true);
    const log10 = logView.getFloat64(offset + 8, true);
    const log2 = logView.getFloat64(offset + 16, true);
    print("log(" + i + "): ln=" + ln + " log10=" + log10 + " log2=" + log2);
    offset += 24;
}

print("=== Testing exponential values ===");
const expBuf = new ArrayBuffer(256);
const expView = new DataView(expBuf);

offset = 0;
for (let i = 0; i <= 20; i++) {
    const expVal = Math.exp(i);
    expView.setFloat64(offset, expVal, true);
    offset += 8;
}

offset = 0;
for (let i = 0; i <= 20; i++) {
    const result = expView.getFloat64(offset, true);
    print("exp(" + i + ") = " + result);
    offset += 8;
}

print("=== Testing power function values ===");
const powBuf2 = new ArrayBuffer(512);
const powView2 = new DataView(powBuf2);

offset = 0;
for (let base = 2; base <= 5; base++) {
    for (let exp = 0; exp <= 10; exp++) {
        const powVal = Math.pow(base, exp);
        powView2.setFloat64(offset, powVal, true);
        offset += 8;
    }
}

offset = 0;
for (let base = 2; base <= 5; base++) {
    for (let exp = 0; exp <= 10; exp++) {
        const result = powView2.getFloat64(offset, true);
        print(base + "^" + exp + " = " + result);
        offset += 8;
    }
}

print("=== Testing fractional values ===");
const fracBuf = new ArrayBuffer(512);
const fracView = new DataView(fracBuf);

offset = 0;
for (let num = 1; num <= 10; num++) {
    for (let denom = 2; denom <= 5; denom++) {
        const frac = num / denom;
        fracView.setFloat64(offset, frac, true);
        offset += 8;
    }
}

offset = 0;
for (let num = 1; num <= 10; num++) {
    for (let denom = 2; denom <= 5; denom++) {
        const result = fracView.getFloat64(offset, true);
        print(num + "/" + denom + " = " + result);
        offset += 8;
    }
}

print("=== Testing percentage values ===");
const pctBuf = new ArrayBuffer(256);
const pctView = new DataView(pctBuf);

offset = 0;
for (let pct = 0; pct <= 100; pct += 5) {
    const val = pct / 100.0;
    pctView.setFloat32(offset, val, true);
    pctView.setFloat64(offset + 104, val, true);
    offset += 1;
}

offset = 0;
for (let pct = 0; pct <= 100; pct += 5) {
    const f32 = pctView.getFloat32(offset, true);
    const f64 = pctView.getFloat64(offset + 104, true);
    print(pct + "%: f32=" + f32 + " f64=" + f64);
    offset += 1;
}

print("=== Testing byte reversal patterns ===");
const revBuf = new ArrayBuffer(128);
const revView = new DataView(revBuf);

offset = 0;
for (let i = 0; i < 16; i++) {
    const val = (i << 24) | ((i+1) << 16) | ((i+2) << 8) | (i+3);
    revView.setUint32(offset, val, true);
    revView.setUint32(offset + 64, val, false);
    offset += 4;
}

offset = 0;
for (let i = 0; i < 16; i++) {
    const le = revView.getUint32(offset, true);
    const be = revView.getUint32(offset + 64, false);
    print("Reversal[" + i + "]: LE=0x" + le.toString(16) + " BE=0x" + be.toString(16));
    offset += 4;
}

print("=== Testing checksums and hashes (mock) ===");
const hashBuf = new ArrayBuffer(256);
const hashView = new DataView(hashBuf);

offset = 0;
for (let i = 0; i < 32; i++) {
    // Simple mock hash: multiply by prime and take modulo
    const hash = (i * 31 + 17) % 256;
    hashView.setUint8(offset, hash);
    hashView.setUint32(offset + 32, hash * 65537, true);
    offset += 1;
}

offset = 0;
for (let i = 0; i < 32; i++) {
    const h8 = hashView.getUint8(offset);
    const h32 = hashView.getUint32(offset + 32, true);
    print("Hash[" + i + "]: u8=" + h8 + " u32=" + h32);
    offset += 1;
}

print("=== Testing cumulative sums ===");
const cumBuf = new ArrayBuffer(256);
const cumView = new DataView(cumBuf);

let cumSum = 0;
offset = 0;
for (let i = 1; i <= 50; i++) {
    cumSum += i;
    cumView.setInt32(offset, cumSum, true);
    offset += 4;
}

offset = 0;
for (let i = 1; i <= 50; i++) {
    const result = cumView.getInt32(offset, true);
    print("CumSum[" + i + "]: " + result);
    offset += 4;
}

print("=== Testing alternating bit patterns ===");
const altBitBuf = new ArrayBuffer(128);
const altBitView = new DataView(altBitBuf);

const altPatterns = [
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xF0F0F0F0, 0x0F0F0F0F, 0xFF00FF00, 0x00FF00FF
];

offset = 0;
for (let pat of altPatterns) {
    altBitView.setUint32(offset, pat, true);
    altBitView.setUint32(offset + 32, pat, false);
    offset += 4;
}

offset = 0;
for (let i = 0; i < altPatterns.length; i++) {
    const le = altBitView.getUint32(offset, true);
    const be = altBitView.getUint32(offset + 32, false);
    print("AltBit[" + i + "]: LE=0x" + le.toString(16) + " BE=0x" + be.toString(16));
    offset += 4;
}

print("=== Testing combined read/write stress patterns ===");
const stressBuf = new ArrayBuffer(1024);
const stressView = new DataView(stressBuf);

// Stress test: many small operations
offset = 0;
for (let i = 0; i < 64; i++) {
    stressView.setInt8(offset, i - 32);
    stressView.setUint8(offset + 64, i);
    stressView.setInt16(offset + 128, (i - 32) * 100, true);
    stressView.setUint16(offset + 256, i * 100, true);
    stressView.setInt32(offset + 384, (i - 32) * 10000, true);
    stressView.setUint32(offset + 512, i * 10000, true);
    stressView.setFloat32(offset + 640, (i - 32) / 10.0, true);
    stressView.setFloat64(offset + 768, (i - 32) / 100.0, true);
    offset += 1;
}

// Verify stress test results
offset = 0;
for (let i = 0; i < 64; i++) {
    const i8 = stressView.getInt8(offset);
    const ui8 = stressView.getUint8(offset + 64);
    const i16 = stressView.getInt16(offset + 128, true);
    const ui16 = stressView.getUint16(offset + 256, true);
    if (i % 8 === 0) {  // Print every 8th to save space
        print("Stress[" + i + "]: i8=" + i8 + " ui8=" + ui8 + " i16=" + i16 + " ui16=" + ui16);
    }
    offset += 1;
}

print("=== Testing mixed endianness in same buffer ===");
const mixBuf = new ArrayBuffer(512);
const mixView = new DataView(mixBuf);

offset = 0;
for (let i = 0; i < 16; i++) {
    mixView.setInt32(offset, i * 1000, true);        // LE
    mixView.setInt32(offset + 64, i * 2000, false);  // BE
    mixView.setFloat64(offset + 128, i / 10.0, true);   // LE
    mixView.setFloat64(offset + 192, i / 20.0, false);  // BE
    offset += 4;
}

offset = 0;
for (let i = 0; i < 16; i++) {
    const i32_le = mixView.getInt32(offset, true);
    const i32_be = mixView.getInt32(offset + 64, false);
    const f64_le = mixView.getFloat64(offset + 128, true);
    const f64_be = mixView.getFloat64(offset + 192, false);
    print("Mix[" + i + "]: i32_le=" + i32_le + " i32_be=" + i32_be + " f64_le=" + f64_le + " f64_be=" + f64_be);
    offset += 4;
}

print("=== Testing buffer reuse patterns ===");
const reuseBuf = new ArrayBuffer(128);
const reuseView = new DataView(reuseBuf);

// Write pass 1
for (let i = 0; i < 32; i++) {
    reuseView.setInt32(i * 4, i * 100, true);
}
// Read pass 1
offset = 0;
for (let i = 0; i < 8; i++) {
    print("Reuse pass1[" + i + "]: " + reuseView.getInt32(offset, true));
    offset += 4;
}

// Overwrite with pass 2
for (let i = 0; i < 32; i++) {
    reuseView.setFloat32(i * 4, i / 2.0, true);
}
// Read pass 2
offset = 0;
for (let i = 0; i < 8; i++) {
    print("Reuse pass2[" + i + "]: " + reuseView.getFloat32(offset, true));
    offset += 4;
}

print("=== Testing descending value patterns ===");
const descBuf = new ArrayBuffer(256);
const descView = new DataView(descBuf);

offset = 0;
for (let i = 100; i > 0; i -= 5) {
    descView.setInt16(offset, i, true);
    descView.setInt32(offset + 50, i * 100, true);
    offset += 2;
}

offset = 0;
for (let i = 100; i > 0; i -= 5) {
    const i16 = descView.getInt16(offset, true);
    const i32 = descView.getInt32(offset + 50, true);
    print("Desc: i16=" + i16 + " i32=" + i32);
    offset += 2;
}

print("=== Testing value doubling sequence ===");
const doubleBuf = new ArrayBuffer(256);
const doubleView = new DataView(doubleBuf);

let doubleVal = 1;
offset = 0;
for (let i = 0; i < 20; i++) {
    doubleView.setFloat64(offset, doubleVal, true);
    doubleVal *= 2;
    offset += 8;
}

offset = 0;
for (let i = 0; i < 20; i++) {
    const result = doubleView.getFloat64(offset, true);
    print("Double[" + i + "]: " + result);
    offset += 8;
}

print("=== Testing value halving sequence ===");
const halveBuf = new ArrayBuffer(256);
const halveView = new DataView(halveBuf);

let halveVal = 1024;
offset = 0;
for (let i = 0; i < 20; i++) {
    halveView.setFloat64(offset, halveVal, true);
    halveVal /= 2;
    offset += 8;
}

offset = 0;
for (let i = 0; i < 20; i++) {
    const result = halveView.getFloat64(offset, true);
    print("Halve[" + i + "]: " + result);
    offset += 8;
}

print("=== Testing negation patterns ===");
const negPatBuf = new ArrayBuffer(256);
const negPatView = new DataView(negPatBuf);

offset = 0;
for (let i = 1; i <= 32; i++) {
    negPatView.setInt32(offset, i, true);
    negPatView.setInt32(offset + 128, -i, true);
    offset += 4;
}

offset = 0;
for (let i = 1; i <= 32; i++) {
    const pos = negPatView.getInt32(offset, true);
    const neg = negPatView.getInt32(offset + 128, true);
    if (i % 4 === 1) {  // Print every 4th
        print("NegPat[" + i + "]: pos=" + pos + " neg=" + neg);
    }
    offset += 4;
}

print("=== Testing reciprocal values ===");
const recipBuf = new ArrayBuffer(256);
const recipView = new DataView(recipBuf);

offset = 0;
for (let i = 1; i <= 25; i++) {
    const recip = 1.0 / i;
    recipView.setFloat64(offset, recip, true);
    offset += 8;
}

offset = 0;
for (let i = 1; i <= 25; i++) {
    const result = recipView.getFloat64(offset, true);
    print("1/" + i + " = " + result);
    offset += 8;
}

print("=== Testing RangeError cases ===");

// Create a small buffer for testing boundary conditions
const testBuf = new ArrayBuffer(16);
const testView = new DataView(testBuf);

print("--- Testing offset out of range ---");

// Test getInt8 out of range
try {
    testView.getInt8(16); // buffer size is 16, so offset 16 is out of range
} catch (e) {
    print("getInt8(16): " + e.name + " - " + e.message);
}

try {
    testView.getInt8(-1); // negative offset
} catch (e) {
    print("getInt8(-1): " + e.name + " - " + e.message);
}

// Test getUint8 out of range
try {
    testView.getUint8(16);
} catch (e) {
    print("getUint8(16): " + e.name + " - " + e.message);
}

// Test getInt16 out of range
try {
    testView.getInt16(15); // needs 2 bytes, but only 1 byte available
} catch (e) {
    print("getInt16(15): " + e.name + " - " + e.message);
}

try {
    testView.getInt16(16); // completely out of range
} catch (e) {
    print("getInt16(16): " + e.name + " - " + e.message);
}

// Test getUint16 out of range
try {
    testView.getUint16(15);
} catch (e) {
    print("getUint16(15): " + e.name + " - " + e.message);
}

// Test getInt32 out of range
try {
    testView.getInt32(13); // needs 4 bytes, but only 3 bytes available
} catch (e) {
    print("getInt32(13): " + e.name + " - " + e.message);
}

try {
    testView.getInt32(16);
} catch (e) {
    print("getInt32(16): " + e.name + " - " + e.message);
}

// Test getUint32 out of range
try {
    testView.getUint32(13);
} catch (e) {
    print("getUint32(13): " + e.name + " - " + e.message);
}

// Test getFloat32 out of range
try {
    testView.getFloat32(13);
} catch (e) {
    print("getFloat32(13): " + e.name + " - " + e.message);
}

// Test getFloat64 out of range
try {
    testView.getFloat64(9); // needs 8 bytes, but only 7 bytes available
} catch (e) {
    print("getFloat64(9): " + e.name + " - " + e.message);
}

try {
    testView.getFloat64(16);
} catch (e) {
    print("getFloat64(16): " + e.name + " - " + e.message);
}

print("--- Testing setXXX out of range ---");

// Test setInt8 out of range
try {
    testView.setInt8(16, 100);
} catch (e) {
    print("setInt8(16, 100): " + e.name + " - " + e.message);
}

try {
    testView.setInt8(-1, 100);
} catch (e) {
    print("setInt8(-1, 100): " + e.name + " - " + e.message);
}

// Test setUint8 out of range
try {
    testView.setUint8(16, 200);
} catch (e) {
    print("setUint8(16, 200): " + e.name + " - " + e.message);
}

// Test setInt16 out of range
try {
    testView.setInt16(15, 1000);
} catch (e) {
    print("setInt16(15, 1000): " + e.name + " - " + e.message);
}

// Test setUint16 out of range
try {
    testView.setUint16(15, 2000);
} catch (e) {
    print("setUint16(15, 2000): " + e.name + " - " + e.message);
}

// Test setInt32 out of range
try {
    testView.setInt32(13, 100000);
} catch (e) {
    print("setInt32(13, 100000): " + e.name + " - " + e.message);
}

// Test setUint32 out of range
try {
    testView.setUint32(13, 200000);
} catch (e) {
    print("setUint32(13, 200000): " + e.name + " - " + e.message);
}

// Test setFloat32 out of range
try {
    testView.setFloat32(13, 3.14);
} catch (e) {
    print("setFloat32(13, 3.14): " + e.name + " - " + e.message);
}

// Test setFloat64 out of range
try {
    testView.setFloat64(9, 2.718);
} catch (e) {
    print("setFloat64(9, 2.718): " + e.name + " - " + e.message);
}

print("=== Testing invalid parameters ===");

print("--- Testing invalid offset types ---");

// Test with non-numeric offsets
try {
    testView.getInt8("invalid");
} catch (e) {
    print("getInt8('invalid'): " + e.name + " - " + e.message);
}

try {
    testView.getInt16(null);
} catch (e) {
    print("getInt16(null): " + e.name + " - " + e.message);
}

try {
    testView.getInt32(undefined);
} catch (e) {
    print("getInt32(undefined): " + e.name + " - " + e.message);
}

try {
    testView.getFloat32({});
} catch (e) {
    print("getFloat32({}): " + e.name + " - " + e.message);
}

try {
    testView.getFloat64([]);
} catch (e) {
    print("getFloat64([]): " + e.name + " - " + e.message);
}

// Test setters with invalid offsets
try {
    testView.setInt8("bad", 100);
} catch (e) {
    print("setInt8('bad', 100): " + e.name + " - " + e.message);
}

try {
    testView.setFloat64(Symbol("test"), 3.14);
} catch (e) {
    print("setFloat64(Symbol, 3.14): " + e.name + " - " + e.message);
}

print("--- Testing invalid value types for setters ---");

// Test with non-numeric values
try {
    testView.setInt8(0, "not a number");
} catch (e) {
    print("setInt8(0, 'not a number'): " + e.name + " - " + e.message);
}

try {
    testView.setInt16(0, {});
} catch (e) {
    print("setInt16(0, {}): " + e.name + " - " + e.message);
}

try {
    testView.setInt32(0, [1, 2, 3]);
} catch (e) {
    print("setInt32(0, [1,2,3]): " + e.name + " - " + e.message);
}

try {
    testView.setFloat32(0, Symbol("float"));
} catch (e) {
    print("setFloat32(0, Symbol): " + e.name + " - " + e.message);
}

print("--- Testing BigInt values (should cause TypeError) ---");

try {
    testView.setInt8(0, 1n); // BigInt
} catch (e) {
    print("setInt8(0, 1n): " + e.name + " - " + e.message);
}

try {
    testView.setInt32(0, 123n);
} catch (e) {
    print("setInt32(0, 123n): " + e.name + " - " + e.message);
}

try {
    testView.setFloat64(0, 456n);
} catch (e) {
    print("setFloat64(0, 456n): " + e.name + " - " + e.message);
}

print("--- Testing extremely large offsets ---");

try {
    testView.getInt8(Number.MAX_SAFE_INTEGER);
} catch (e) {
    print("getInt8(MAX_SAFE_INTEGER): " + e.name + " - " + e.message);
}

try {
    testView.setInt32(0xFFFFFFFF, 100);
} catch (e) {
    print("setInt32(0xFFFFFFFF, 100): " + e.name + " - " + e.message);
}

try {
    testView.getFloat64(Infinity);
} catch (e) {
    print("getFloat64(Infinity): " + e.name + " - " + e.message);
}

print("--- Testing NaN as offset ---");

try {
    testView.getInt8(NaN);
} catch (e) {
    print("getInt8(NaN): " + e.name + " - " + e.message);
}

try {
    testView.setInt16(NaN, 100);
} catch (e) {
    print("setInt16(NaN, 100): " + e.name + " - " + e.message);
}

print("--- Testing fractional offsets ---");

try {
    testView.getInt8(1.5);
} catch (e) {
    print("getInt8(1.5): " + e.name + " - " + e.message);
}

try {
    testView.setInt32(2.9, 100);
} catch (e) {
    print("setInt32(2.9, 100): " + e.name + " - " + e.message);
}

print("=== Testing DataView constructor errors ===");

print("--- Testing invalid ArrayBuffer ---");

try {
    new DataView(null);
} catch (e) {
    print("new DataView(null): " + e.name + " - " + e.message);
}

try {
    new DataView(undefined);
} catch (e) {
    print("new DataView(undefined): " + e.name + " - " + e.message);
}

try {
    new DataView("not a buffer");
} catch (e) {
    print("new DataView('not a buffer'): " + e.name + " - " + e.message);
}

try {
    new DataView({});
} catch (e) {
    print("new DataView({}): " + e.name + " - " + e.message);
}

print("--- Testing invalid offset/length in constructor ---");

const constructorBuf = new ArrayBuffer(32);

try {
    new DataView(constructorBuf, -1);
} catch (e) {
    print("new DataView(buf, -1): " + e.name + " - " + e.message);
}

try {
    new DataView(constructorBuf, 33); // offset > buffer size
} catch (e) {
    print("new DataView(buf, 33): " + e.name + " - " + e.message);
}

try {
    new DataView(constructorBuf, 16, -1); // negative length
} catch (e) {
    print("new DataView(buf, 16, -1): " + e.name + " - " + e.message);
}

try {
    new DataView(constructorBuf, 16, 17); // offset + length > buffer size
} catch (e) {
    print("new DataView(buf, 16, 17): " + e.name + " - " + e.message);
}

try {
    new DataView(constructorBuf, 0, 33); // length > buffer size
} catch (e) {
    print("new DataView(buf, 0, 33): " + e.name + " - " + e.message);
}

print("=== Testing boundary conditions ===");

// Test exactly at the boundary (should work)
const boundaryBuf = new ArrayBuffer(8);
const boundaryView = new DataView(boundaryBuf);

print("--- Testing valid boundary cases ---");
try {
    boundaryView.setInt8(7, 100);  // last valid position for int8
    print("setInt8(7, 100): success - " + boundaryView.getInt8(7));
} catch (e) {
    print("setInt8(7, 100): " + e.name + " - " + e.message);
}

try {
    boundaryView.setInt16(6, 1000, true); // last valid position for int16
    print("setInt16(6, 1000): success - " + boundaryView.getInt16(6, true));
} catch (e) {
    print("setInt16(6, 1000): " + e.name + " - " + e.message);
}

try {
    boundaryView.setInt32(4, 100000, true); // last valid position for int32
    print("setInt32(4, 100000): success - " + boundaryView.getInt32(4, true));
} catch (e) {
    print("setInt32(4, 100000): " + e.name + " - " + e.message);
}

try {
    boundaryView.setFloat64(0, 3.14159, true); // uses all 8 bytes
    print("setFloat64(0, 3.14159): success - " + boundaryView.getFloat64(0, true));
} catch (e) {
    print("setFloat64(0, 3.14159): " + e.name + " - " + e.message);
}

print("--- Testing just over the boundary ---");

try {
    boundaryView.setInt8(8, 100); // one byte too far
} catch (e) {
    print("setInt8(8, 100): " + e.name + " - " + e.message);
}

try {
    boundaryView.setInt16(7, 1000, true); // would need bytes 7 and 8
} catch (e) {
    print("setInt16(7, 1000): " + e.name + " - " + e.message);
}

try {
    boundaryView.setInt32(5, 100000, true); // would need bytes 5-8
} catch (e) {
    print("setInt32(5, 100000): " + e.name + " - " + e.message);
}

try {
    boundaryView.setFloat64(1, 2.718, true); // would need bytes 1-8
} catch (e) {
    print("setFloat64(1, 2.718): " + e.name + " - " + e.message);
}

print("=== Testing with detached ArrayBuffer ===");

const detachBuf = new ArrayBuffer(16);
const detachView = new DataView(detachBuf);

// Note: In most environments, you can't actually detach an ArrayBuffer directly
// This is typically done by transferring it, but we'll simulate the attempt

print("--- Attempting operations on potentially detached buffer ---");

try {
    detachView.getInt8(0);
    print("Operation on detached buffer succeeded (buffer not actually detached)");
} catch (e) {
    print("getInt8 on detached buffer: " + e.name + " - " + e.message);
}

print("=== Testing with zero-length ArrayBuffer ===");

const zeroBuf = new ArrayBuffer(0);
const zeroView = new DataView(zeroBuf);

try {
    zeroView.getInt8(0);
} catch (e) {
    print("getInt8(0) on zero-length buffer: " + e.name + " - " + e.message);
}

try {
    zeroView.setInt8(0, 100);
} catch (e) {
    print("setInt8(0, 100) on zero-length buffer: " + e.name + " - " + e.message);
}

print("=== Testing special number values as parameters ===");

const specialBuf = new ArrayBuffer(16);
const specialView = new DataView(specialBuf);

print("--- Testing Infinity and -Infinity as values ---");
try {
    specialView.setFloat32(0, Infinity);
    print("setFloat32(0, Infinity): success - " + specialView.getFloat32(0));
} catch (e) {
    print("setFloat32(0, Infinity): " + e.name + " - " + e.message);
}

try {
    specialView.setFloat64(8, -Infinity);
    print("setFloat64(8, -Infinity): success - " + specialView.getFloat64(8));
} catch (e) {
    print("setFloat64(8, -Infinity): " + e.name + " - " + e.message);
}

print("--- Testing NaN as value ---");
try {
    specialView.setFloat32(0, NaN);
    print("setFloat32(0, NaN): success - " + (isNaN(specialView.getFloat32(0)) ? "NaN" : specialView.getFloat32(0)));
} catch (e) {
    print("setFloat32(0, NaN): " + e.name + " - " + e.message);
}

print("DataView RangeError and invalid parameter tests completed!");