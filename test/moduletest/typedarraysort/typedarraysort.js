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
