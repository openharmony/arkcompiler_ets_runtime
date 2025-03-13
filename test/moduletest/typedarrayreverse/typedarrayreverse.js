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
 * @tc.name:typedarrayreverse
 * @tc.desc:test TypedArray.reverse
 * @tc.type: FUNC
 * @tc.issue: I8NI6U
 */
// 1. Uint8Array 测试用例
let uint8Array = new Uint8Array([10, 255, 0, 128, 200]);
assert_equal("Before reverse (Uint8Array): "+uint8Array,"Before reverse (Uint8Array): 10,255,0,128,200");
uint8Array.reverse();  // 反转数组
assert_equal("After reverse (Uint8Array): "+uint8Array,"After reverse (Uint8Array): 200,128,0,255,10");

// 2. Int8Array 测试用例
let int8Array = new Int8Array([10, -128, 0, 100, -50]);
assert_equal("Before reverse (Int8Array): "+int8Array,"Before reverse (Int8Array): 10,-128,0,100,-50");
int8Array.reverse();  // 反转数组
assert_equal("After reverse (Int8Array): "+int8Array,"After reverse (Int8Array): -50,100,0,-128,10");

// 3. Uint16Array 测试用例
let uint16Array = new Uint16Array([65535, 0, 256, 1000, 5000]);
assert_equal("Before reverse (Uint16Array): "+uint16Array,"Before reverse (Uint16Array): 65535,0,256,1000,5000");
uint16Array.reverse();  // 反转数组
assert_equal("After reverse (Uint16Array): "+uint16Array,"After reverse (Uint16Array): 5000,1000,256,0,65535");

// 4. Int16Array 测试用例
let int16Array = new Int16Array([30000, -30000, 0, 5000, -10000]);
assert_equal("Before reverse (Int16Array): "+int16Array,"Before reverse (Int16Array): 30000,-30000,0,5000,-10000");
int16Array.reverse();  // 反转数组
assert_equal("After reverse (Int16Array): "+int16Array,"After reverse (Int16Array): -10000,5000,0,-30000,30000");

// 5. Uint32Array 测试用例
let uint32Array = new Uint32Array([4000000000, 1000000, 255, 0, 500000000]);
assert_equal("Before reverse (Uint32Array): "+uint32Array,"Before reverse (Uint32Array): 4000000000,1000000,255,0,500000000");
uint32Array.reverse();  // 反转数组
assert_equal("After reverse (Uint32Array): "+uint32Array,"After reverse (Uint32Array): 500000000,0,255,1000000,4000000000");

// 6. Int32Array 测试用例
let int32Array = new Int32Array([2147483647, -2147483648, 1000, -5000, 0]);
assert_equal("Before reverse (Int32Array): "+int32Array,"Before reverse (Int32Array): 2147483647,-2147483648,1000,-5000,0");
int32Array.reverse();  // 反转数组
assert_equal("After reverse (Int32Array): "+int32Array,"After reverse (Int32Array): 0,-5000,1000,-2147483648,2147483647");

// 7. Float32Array 测试用例
let float32Array = new Float32Array([3.14, -1.5, 0.5, -3.14, 2.71]);
assert_equal("Before reverse (Float32Array): "+float32Array,"Before reverse (Float32Array): 3.140000104904175,-1.5,0.5,-3.140000104904175,2.7100000381469727");
float32Array.reverse();  // 反转数组
assert_equal("After reverse (Float32Array): "+float32Array,"After reverse (Float32Array): 2.7100000381469727,-3.140000104904175,0.5,-1.5,3.140000104904175");

// 8. Float64Array 测试用例
let float64Array = new Float64Array([1.234, -5.678, 9.01, 3.14159, 2.718]);
assert_equal("Before reverse (Float64Array): "+float64Array,"Before reverse (Float64Array): 1.234,-5.678,9.01,3.14159,2.718");
float64Array.reverse();  // 反转数组
assert_equal("After reverse (Float64Array): "+float64Array,"After reverse (Float64Array): 2.718,3.14159,9.01,-5.678,1.234");

// 9. BigUint64Array 测试用例
let bigUint64Array = new BigUint64Array([BigInt(100000000000), BigInt(255), BigInt(5000000000), BigInt(100000000)]);
assert_equal("Before reverse (BigUint64Array): "+bigUint64Array,"Before reverse (BigUint64Array): 100000000000,255,5000000000,100000000");
bigUint64Array.reverse();  // 反转数组
assert_equal("After reverse (BigUint64Array): "+bigUint64Array,"After reverse (BigUint64Array): 100000000,5000000000,255,100000000000");

// 10. BigInt64Array 测试用例
let bigInt64Array = new BigInt64Array([BigInt(5000000000), BigInt(-5000000000), BigInt(0), BigInt(2000000000)]);
assert_equal("Before reverse (BigInt64Array): "+bigInt64Array,"Before reverse (BigInt64Array): 5000000000,-5000000000,0,2000000000");
bigInt64Array.reverse();  // 反转数组
assert_equal("After reverse (BigInt64Array): "+bigInt64Array,"After reverse (BigInt64Array): 2000000000,0,-5000000000,5000000000");

{
    let arr1 = new Int16Array(0);
    new Int8Array(arr1);
    arr1.reverse();
    assert_equal(arr1.length, 0);
}

test_end();