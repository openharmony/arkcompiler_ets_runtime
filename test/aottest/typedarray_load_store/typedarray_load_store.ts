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




/** if you failed on this test, it's most likely that you accidentally
 *  set TypedArray.isOnHeap to true with this TypedArray construded from ArrayBuffer. */

let buffer = new ArrayBuffer(120); 

function GenerateFakeRandomInteger(): Int32Array {
    let resource: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
    return resource;
}

function GenerateFakeRandomFloat(): Float64Array {
    let resource: Float64Array = new Float64Array([12.2, 43.5, 56.2, 76.6, 89.7, 54.9, 45.2, 32.5, 35.6, 47.2, 46.6, 44.3, 21.2, 37.6, 84.57]);
    return resource;
}

function GenerateFakeRandomIndex(): Int32Array {
    let resource: Int32Array = new Int32Array([3, 14, 44, 25, 91, 38, 82, 88, 64, 81, 70, 90, 33, 63, 70]);
    return resource;
}

/** Test fake random load and store of Int32Array. */
function IntegerArray(res: Int32Array) {
    let count: number = 10;
    let integerIndexes: Int32Array = GenerateFakeRandomIndex();
    let resources: Int32Array = GenerateFakeRandomInteger();
    let resLength: number = res.length;
    let num: number = 1;
    let length: number = resources.length - 1;
    for (let i = 0; i < count; i++) {
        num += integerIndexes[i % num & length];
        res[i % resLength] += resources[i % num & length];
    }
    let tmp: number = 0;
    for (let i = 0; i < resLength; i++) {
        tmp += res[i];
    }
    print(tmp);
}

(() => {
    /** TypedArray constructed from ArrayBuffer with byteOffset = 20 and length = 4. */
    let arr1 = new Int32Array(buffer, 20, 4); 

    IntegerArray(arr1);

    /** TypedArray constructed from ArrayBuffer with byteOffset = 28 and length = 5, intersecting with arr1. */
    let arr2 = new Int32Array(buffer, 28, 5); 

    IntegerArray(arr2);

    /** TypedArray constructed from ArrayBuffer with byteOffset = 0 and length = 5. */
    let arr3 = new Int32Array(buffer, 0, 5);

    IntegerArray(arr3);

    /** TypedArray constructed from another TypedArray(constructed from ArrayBuffer). */
    let arr4 = new Int32Array(arr2); 

    IntegerArray(arr4);

    /** TypedArray constructed from another TypedArray(constructed from TypedArray). */
    let arr5 = new Int32Array(arr4);

    IntegerArray(arr5);
})();

/** Test fake random load and store of Float64Array. */
function FloatArray(res: Float64Array) {
    let count: number = 10;
    let resources: Float64Array = GenerateFakeRandomFloat();
    let integerIndexes: Int32Array = GenerateFakeRandomIndex();
    let resLength: number = res.length;
    let num: number = 1;
    let length: number = resources.length - 1;
    for (let i = 0; i < count; i++) {
        num += integerIndexes[i % num & length];
        res[i % resLength] += resources[i % num & length];
    }
    let tmp: number = 0.0;
    for (let i = 0; i < resLength; i++) {
        tmp += res[i];
    }
    /** use toFixed() to avoid accuracy problems */
    print(tmp.toFixed(2));
}

(() => {
    /** TypedArray constructed from ArrayBuffer with byteOffset = 16 and length = 4. */
    let arr1 = new Float64Array(buffer, 16, 4); 

    FloatArray(arr1);

    /** TypedArray constructed from ArrayBuffer with byteOffset = 24 and length = 5, intersecting with arr1. */
    let arr2 = new Float64Array(buffer, 24, 5); 
    
    FloatArray(arr2);

    /** TypedArray constructed from ArrayBuffer with byteOffset = 0 and length = 5, intersecting with arr1 and arr2. */
    let arr3 = new Float64Array(buffer, 0, 5);
    
    FloatArray(arr3);

    /** TypedArray constructed from another TypedArray(constructed from ArrayBuffer). */
    let arr4 = new Float64Array(arr2); 

    FloatArray(arr4);

    /** TypedArray constructed from another TypedArray(constructed from TypedArray). */
    let arr5 = new Float64Array(arr4);

    FloatArray(arr5);
})();
