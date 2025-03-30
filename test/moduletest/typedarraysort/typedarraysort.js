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
        assert_equal(ctr.name,"Int8Array");
        assert_equal(error instanceof TypeError, true);
    }
})

let arr1 = new Int16Array([5, 2, 7, 3, 8, NaN, 0, 4, 9]);
assert_equal(arr1.sort().toString(),"0,0,2,3,4,5,7,8,9");
let arr2 = new Uint32Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr2.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr3 = new Float32Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr3.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr4 = new Float64Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr4.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr5 = new Int32Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr5.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr6 = new Int8Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr6.sort().toString(), "0,1,2,3,4,5,7,8,9");
let arr7 = new Uint16Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr7.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr8 = new Uint8Array([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr8.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr9 = new Uint8ClampedArray([5, 2, 7, 3, 8, 1, 0, 4, 9]);
assert_equal(arr9.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr10 = new BigInt64Array([5n, 2n, 7n, 3n, 8n, 1n, 0n, 4n, 9n]);
assert_equal(arr9.sort().toString() , "0,1,2,3,4,5,7,8,9");
let arr11 = new BigUint64Array([5n, 2n, 7n, 3n, 8n, 1n, 0n, 4n, 9n]);
assert_equal(arr11.sort().toString() , "0,1,2,3,4,5,7,8,9");

// 1. Uint8Array 测试用例
let uint8Array = new Uint8Array([10, 255, 0, 128, 200]);
assert_equal("Before sort (Uint8Array): "+uint8Array,"Before sort (Uint8Array): 10,255,0,128,200");
uint8Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Uint8Array): "+uint8Array,"After sort (Uint8Array): 0,10,128,200,255");

// 2. Int8Array 测试用例
let int8Array = new Int8Array([10, -128, 0, 100, -50]);
assert_equal("Before sort (Int8Array): "+int8Array,"Before sort (Int8Array): 10,-128,0,100,-50");
int8Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Int8Array): "+int8Array,"After sort (Int8Array): -128,-50,0,10,100");

// 3. Uint16Array 测试用例
let uint16Array = new Uint16Array([65535, 0, 256, 1000, 5000]);
assert_equal("Before sort (Uint16Array): "+uint16Array,"Before sort (Uint16Array): 65535,0,256,1000,5000");
uint16Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Uint16Array): "+uint16Array,"After sort (Uint16Array): 0,256,1000,5000,65535");

// 4. Int16Array 测试用例
let int16Array = new Int16Array([30000, -30000, 0, 5000, -10000]);
assert_equal("Before sort (Int16Array): "+int16Array,"Before sort (Int16Array): 30000,-30000,0,5000,-10000");
int16Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Int16Array): "+int16Array,"After sort (Int16Array): -30000,-10000,0,5000,30000");

// 5. Uint32Array 测试用例
let uint32Array = new Uint32Array([4000000000, 1000000, 255, 0, 500000000]);
assert_equal("Before sort (Uint32Array): "+uint32Array,"Before sort (Uint32Array): 4000000000,1000000,255,0,500000000");
uint32Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Uint32Array): "+uint32Array,"After sort (Uint32Array): 0,255,1000000,500000000,4000000000");

// 6. Int32Array 测试用例
let int32Array = new Int32Array([2147483647, -2147483648, 1000, -5000, 0]);
assert_equal("Before sort (Int32Array): "+int32Array,"Before sort (Int32Array): 2147483647,-2147483648,1000,-5000,0");
int32Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Int32Array): "+int32Array,"After sort (Int32Array): -2147483648,-5000,0,1000,2147483647");

// 7. Float32Array 测试用例
let float32Array = new Float32Array([3.14, -1.5, 0.5, -3.14, 2.71]);
assert_equal("Before sort (Float32Array): "+float32Array,"Before sort (Float32Array): 3.140000104904175,-1.5,0.5,-3.140000104904175,2.7100000381469727");
float32Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Float32Array): "+float32Array,"After sort (Float32Array): -3.140000104904175,-1.5,0.5,2.7100000381469727,3.140000104904175");

// 8. Float64Array 测试用例
let float64Array = new Float64Array([1.234, -5.678, 9.01, 3.14159, 2.718]);
assert_equal("Before sort (Float64Array): "+float64Array,"Before sort (Float64Array): 1.234,-5.678,9.01,3.14159,2.718");
float64Array.sort();  // 默认按字符编码排序
assert_equal("After sort (Float64Array): "+float64Array,"After sort (Float64Array): -5.678,1.234,2.718,3.14159,9.01");

test_end();