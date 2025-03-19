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

test_end();