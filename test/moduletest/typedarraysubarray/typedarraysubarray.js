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

const typedArraySubArrayConstructors = [
  Float64Array, Float32Array, Int32Array, Int16Array, Int8Array, Uint32Array, Uint16Array, Uint8Array,
  Uint8ClampedArray
];

typedArraySubArrayConstructors.forEach(function (ctor, i) {
    testTypeArrayAt1(ctor)
});

function testTypeArrayAt1(ctor) {
    let result = [];
    let obj = new ctor([10, 11, 12, 13, 14]);
    let result1 = obj.subarray(0, 5);
    assert_equal(result1.toString(),"10,11,12,13,14");
    result1 = obj.subarray(-1, 1);
    assert_equal(result1.length,0);
    result1 = obj.subarray(2, 10);
    assert_equal(result1.toString(),"12,13,14");
    result1 = obj.subarray(2, -10);
    assert_equal(result1.length,0);
}

var v0 = new Uint8Array(64);
try {
  v0 = v0.subarray(64);
} catch (e) {
  assert_unreachable();
}
try {
  v0 = v0.subarray(64);
} catch (e) {
  assert_unreachable();
}
assert_equal(v0.toString(),"");

test_end();