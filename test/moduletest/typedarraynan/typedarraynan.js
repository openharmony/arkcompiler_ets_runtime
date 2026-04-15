/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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
 * @tc.name:typedarraynan
 * @tc.desc:test TypedArray with NaN
 * @tc.type: FUNC
 * @tc.require: issueI7X3AY
 */

-4n ^-4n;
8>>8;
const o17 = {
    "maxByteLength":8,
};
o17.b = 8;
o17.b = o17;
const v18 = new SharedArrayBuffer(8,o17);
const v19 = new Uint32Array(v18,4);
v19[0]=-5;
const v20 = new Float32Array(v18);
try {
    v20.toReversed();
} catch(err) {
}
v20.set(v19);
assert_equal(v19[0],4294967291);
assert_equal(v20[0],4294967296);
assert_equal(v20[1].toString(),"NaN");

var buffer = new ArrayBuffer(8);
var array1 = new Int32Array(buffer);
array1[0] = -5;
array1[1] = -5;
var array2 = new Float64Array(buffer);
assert_equal(array2[0].toString(),"NaN");

array2[0] = 9007199254740991;
assert_equal(array1[0],-1);
assert_equal(array1[1],1128267775);
assert_equal(array2[0],9007199254740991);

array2[0] = NaN;
assert_equal(array1[0],0);
assert_equal(array1[1],2146959360);
assert_equal(array2[0].toString(),"NaN");

function add(arr, idx) {
    arr[idx] += 0;
    return "add PASS";
}
function sub(arr, idx) {
    arr[idx] -= 0;
    return "sub PASS";
}
function mul(arr, idx) {
    arr[idx] *= 1;
    return "mul PASS";
}
function div(arr, idx) {
    arr[idx] /= 1;
    return "div PASS";
}
function shl(arr, idx) {
    arr[idx] <<= 0;
    return "shl PASS";
}
function shr(arr, idx) {
    arr[idx] >>= 0;
    return "shr PASS";
}
function xor(arr, idx) {
    arr[idx] ^= 0;
    return "xor PASS";
}
function and(arr, idx) {
    arr[idx] &= 1;
    return "and PASS";
}
function or(arr, idx) {
    arr[idx] |= 0;
    return "or PASS";
}
function not(arr, idx) {
    arr[idx] = ~arr[idx];
    return "not PASS";
}
function add2(arr, idx) {
    arr[idx] += {
        toString: function () {
            return 0;
        },
    };
    return "add PASS";
}
function sub2(arr, idx) {
    arr[idx] -= {
        toString: function () {
            return 0;
        },
    };
    return "sub PASS";
}
function mul2(arr, idx) {
    arr[idx] *= {
        toString: function () {
            return 1;
        },
    };
    return "mul PASS";
}
function div2(arr, idx) {
    arr[idx] /= {
        toString: function () {
            return 1;
        },
    };
    return "div PASS";
}
function shl2(arr, idx) {
    arr[idx] <<= {
        toString: function () {
            return 0;
        },
    };
    return "shl PASS";
}
function shr2(arr, idx) {
    arr[idx] >>= {
        toString: function () {
            return 0;
        },
    };
    return "shr PASS";
}
function xor2(arr, idx) {
    arr[idx] ^= {
        toString: function () {
            return 0;
        },
    };
    return "xor PASS";
}
function and2(arr, idx) {
    arr[idx] &= {
        toString: function () {
            return 1;
        },
    };
    return "and PASS";
}
function or2(arr, idx) {
    arr[idx] |= {
        toString: function () {
            return 0;
        },
    };
    return "or PASS";
}
function not2(arr, idx) {
    arr[idx] = ~{
        toString: function () {
            return arr[idx];
        },
    };
    return "not PASS";
}

var buffer = new ArrayBuffer(16);
var int32arr = new Int32Array(buffer);
int32arr[3] = 0xfff7abcd;
var float64arr = new Float64Array(buffer);
assert_equal(add(float64arr, 1), "add PASS");
assert_equal(sub(float64arr, 1), "sub PASS");
assert_equal(div(float64arr, 1), "div PASS");
assert_equal(mul(float64arr, 1), "mul PASS");
assert_equal(xor(float64arr, 1), "xor PASS");
assert_equal(and(float64arr, 1), "and PASS");
assert_equal(or(float64arr, 1), "or PASS");
assert_equal(not(float64arr, 1), "not PASS");
assert_equal(shl(float64arr, 1), "shl PASS");
assert_equal(shr(float64arr, 1), "shr PASS");
assert_equal(add2(float64arr, 1), "add PASS");
assert_equal(sub2(float64arr, 1), "sub PASS");
assert_equal(div2(float64arr, 1), "div PASS");
assert_equal(mul2(float64arr, 1), "mul PASS");
assert_equal(shl2(float64arr, 1), "shl PASS");
assert_equal(shr2(float64arr, 1), "shr PASS");
assert_equal(xor2(float64arr, 1), "xor PASS");
assert_equal(and2(float64arr, 1), "and PASS");
assert_equal(or2(float64arr, 1), "or PASS");
assert_equal(not2(float64arr, 1), "not PASS");

test_end();