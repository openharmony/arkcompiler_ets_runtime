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
 * @tc.name:typedarraysort
 * @tc.desc:test TypedArray.sort
 * @tc.type: FUNC
 * @tc.issue: I8NI6U
 */
[
Int8Array
].forEach((ctr)=>{
    let arr=new ctr();
    arr[0]=1;
    arr[1]=2;
    arr[2]=3;
    try {
        arr.sort(null);
    } catch (error) {
        print(ctr.name,error.name)   ;
    }
})

let arr1 = new Int16Array([5, 2, 7, 3, 8, NaN, 0, 4, 9]);
print(arr1.sort());
let arr2 = new Uint32Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr2.sort());
let arr3 = new Float32Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr3.sort());
let arr4 = new Float64Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr4.sort());
let arr5 = new Int32Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr5.sort());
let arr6 = new Int8Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr6.sort());
let arr7 = new Uint16Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr7.sort());
let arr8 = new Uint8Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr8.sort());
let arr9 = new Uint8ClampedArray([5, 2, 7, 3, 8, 1, 0, 4, 9]);
print(arr9.sort());
let arr10 = new BigInt64Array([5n, 2n, 7n, 3n, 8n, 1n, 0n, 4n, 9n]);
print(arr9.sort());
let arr11 = new BigUint64Array([5n, 2n, 7n, 3n, 8n, 1n, 0n, 4n, 9n]);
print(arr11.sort());

// 1. Uint8Array 测试用例
let uint8Array = new Uint8Array([10, 255, 0, 128, 200]);
print("Before sort (Uint8Array):", uint8Array);
uint8Array.sort();  // 默认按字符编码排序
print("After sort (Uint8Array):", uint8Array);

// 2. Int8Array 测试用例
let int8Array = new Int8Array([10, -128, 0, 100, -50]);
print("Before sort (Int8Array):", int8Array);
int8Array.sort();  // 默认按字符编码排序
print("After sort (Int8Array):", int8Array);

// 3. Uint16Array 测试用例
let uint16Array = new Uint16Array([65535, 0, 256, 1000, 5000]);
print("Before sort (Uint16Array):", uint16Array);
uint16Array.sort();  // 默认按字符编码排序
print("After sort (Uint16Array):", uint16Array);

// 4. Int16Array 测试用例
let int16Array = new Int16Array([30000, -30000, 0, 5000, -10000]);
print("Before sort (Int16Array):", int16Array);
int16Array.sort();  // 默认按字符编码排序
print("After sort (Int16Array):", int16Array);

// 5. Uint32Array 测试用例
let uint32Array = new Uint32Array([4000000000, 1000000, 255, 0, 500000000]);
print("Before sort (Uint32Array):", uint32Array);
uint32Array.sort();  // 默认按字符编码排序
print("After sort (Uint32Array):", uint32Array);

// 6. Int32Array 测试用例
let int32Array = new Int32Array([2147483647, -2147483648, 1000, -5000, 0]);
print("Before sort (Int32Array):", int32Array);
int32Array.sort();  // 默认按字符编码排序
print("After sort (Int32Array):", int32Array);

// 7. Float32Array 测试用例
let float32Array = new Float32Array([3.14, -1.5, 0.5, -3.14, 2.71]);
print("Before sort (Float32Array):", float32Array);
float32Array.sort();  // 默认按字符编码排序
print("After sort (Float32Array):", float32Array);

// 8. Float64Array 测试用例
let float64Array = new Float64Array([1.234, -5.678, 9.01, 3.14159, 2.718]);
print("Before sort (Float64Array):", float64Array);
float64Array.sort();  // 默认按字符编码排序
print("After sort (Float64Array):", float64Array);
