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
print("Before reverse (Uint8Array):", uint8Array);
uint8Array.reverse();  // 反转数组
print("After reverse (Uint8Array):", uint8Array);

// 2. Int8Array 测试用例
let int8Array = new Int8Array([10, -128, 0, 100, -50]);
print("Before reverse (Int8Array):", int8Array);
int8Array.reverse();  // 反转数组
print("After reverse (Int8Array):", int8Array);

// 3. Uint16Array 测试用例
let uint16Array = new Uint16Array([65535, 0, 256, 1000, 5000]);
print("Before reverse (Uint16Array):", uint16Array);
uint16Array.reverse();  // 反转数组
print("After reverse (Uint16Array):", uint16Array);

// 4. Int16Array 测试用例
let int16Array = new Int16Array([30000, -30000, 0, 5000, -10000]);
print("Before reverse (Int16Array):", int16Array);
int16Array.reverse();  // 反转数组
print("After reverse (Int16Array):", int16Array);

// 5. Uint32Array 测试用例
let uint32Array = new Uint32Array([4000000000, 1000000, 255, 0, 500000000]);
print("Before reverse (Uint32Array):", uint32Array);
uint32Array.reverse();  // 反转数组
print("After reverse (Uint32Array):", uint32Array);

// 6. Int32Array 测试用例
let int32Array = new Int32Array([2147483647, -2147483648, 1000, -5000, 0]);
print("Before reverse (Int32Array):", int32Array);
int32Array.reverse();  // 反转数组
print("After reverse (Int32Array):", int32Array);

// 7. Float32Array 测试用例
let float32Array = new Float32Array([3.14, -1.5, 0.5, -3.14, 2.71]);
print("Before reverse (Float32Array):", float32Array);
float32Array.reverse();  // 反转数组
print("After reverse (Float32Array):", float32Array);

// 8. Float64Array 测试用例
let float64Array = new Float64Array([1.234, -5.678, 9.01, 3.14159, 2.718]);
print("Before reverse (Float64Array):", float64Array);
float64Array.reverse();  // 反转数组
print("After reverse (Float64Array):", float64Array);

// 9. BigUint64Array 测试用例
let bigUint64Array = new BigUint64Array([BigInt(100000000000), BigInt(255), BigInt(5000000000), BigInt(100000000)]);
print("Before reverse (BigUint64Array):", bigUint64Array);
bigUint64Array.reverse();  // 反转数组
print("After reverse (BigUint64Array):", bigUint64Array);

// 10. BigInt64Array 测试用例
let bigInt64Array = new BigInt64Array([BigInt(5000000000), BigInt(-5000000000), BigInt(0), BigInt(2000000000)]);
print("Before reverse (BigInt64Array):", bigInt64Array);
bigInt64Array.reverse();  // 反转数组
print("After reverse (BigInt64Array):", bigInt64Array);
