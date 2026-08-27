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
 * @tc.name:typedarraystore
 * @tc.desc:test typed array indexed store fast path conversion semantics per element kind
 * @tc.type: FUNC
 * @tc.require:
 */

// per-kind conversions over edge values, hot loop first to warm the store IC
var kinds = [
    ["Int8", Int8Array], ["Uint8", Uint8Array], ["Uint8C", Uint8ClampedArray],
    ["Int16", Int16Array], ["Uint16", Uint16Array], ["Int32", Int32Array],
    ["Uint32", Uint32Array], ["Float32", Float32Array], ["Float64", Float64Array]
];
var vals = [0, -0, 1.5, 2.5, -1.5, 127.9, 128, -129, 254.5, 255.5, 255.999, 256, -1,
            65536, 2147483648, 4294967296, NaN, Infinity, -Infinity, 0.4999];
for (var k = 0; k < kinds.length; k++) {
    var C = kinds[k][1];
    var w = new C(64);
    for (var r = 0; r < 8; r++) { for (var i = 0; i < 64; i++) { w[i] = i + r; } }
    var a = new C(vals.length);
    for (var i2 = 0; i2 < vals.length; i2++) { a[i2] = vals[i2]; }
    var out = [];
    for (var j = 0; j < vals.length; j++) { out.push(Object.is(a[j], -0) ? "-0" : String(a[j])); }
    print(kinds[k][0] + " = " + out.join(","));
}

// checksums after hot store loops
for (var k2 = 0; k2 < kinds.length; k2++) {
    var C2 = kinds[k2][1];
    var b = new C2(512);
    for (var r2 = 0; r2 < 4; r2++) { for (var i3 = 0; i3 < 512; i3++) { b[i3] = i3 * 3 + r2; } }
    var sum = 0;
    for (var j2 = 0; j2 < 512; j2++) { sum += b[j2]; }
    print(kinds[k2][0] + " checksum = " + sum);
}

// out-of-bounds, negative and non-canonical keys are ignored or generic
var o = new Int32Array(4);
o[4] = 99; o[-1] = 99; o[4294967295] = 99; o[1.5] = 77; o["02"] = 55;
print("edges = " + o.join(",") + " frac " + o[1.5] + " strkey " + o["02"]);

// side-effecting store value coerces exactly once, in order
var log = [];
var s = new Int16Array(2);
s[0] = { valueOf: function () { log.push("a"); return 70000; } };
print("coerce = " + s[0] + " order " + log.join(","));

// throwing valueOf propagates, element keeps prior value
var t = new Int32Array(1);
t[0] = 5;
try { t[0] = { valueOf: function () { throw new Error("boom"); } }; print("bad"); }
catch (e) { print("caught " + e.message + " kept " + t[0]); }

// float rounding and NaN canonicalization
var f32 = new Float32Array(2);
f32[0] = 1.0000001; f32[1] = NaN;
print("f32 = " + f32[0] + "," + (f32[1] !== f32[1]));

// bigint kinds reject numbers, accept bigints, wrap mod 2^64
try {
    var g = new BigInt64Array(2);
    g[0] = 5n; g[1] = -5n;
    var u = new BigUint64Array(1);
    u[0] = 18446744073709551617n;
    var thrown = "none";
    try { g[0] = 5; } catch (e2) { thrown = e2.name; }
    print("bigint = " + g.join(",") + " wrap " + u[0] + " numstore " + thrown);
} catch (e3) { print("bigint unsupported " + e3.name); }

// Uint8Clamped out-of-int32-range doubles must saturate via ToUint8Clamp
// (regression: the inline float->int conversion is target-dependent for these)
var c8 = new Uint8ClampedArray(4);
c8[0] = 2147483648;
c8[1] = -2147483649;
c8[2] = Infinity;
c8[3] = -Infinity;
var m8 = new Uint8ClampedArray([1]).map(function (x) { return 2147483648; });
print("clamped big = " + c8.join(",") + " map " + m8[0]);

print("typedarraystore suite end");
