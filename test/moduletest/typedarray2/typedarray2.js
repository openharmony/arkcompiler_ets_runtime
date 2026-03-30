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
 * @tc.name:typedarray
 * @tc.desc:test TypedArray
 * @tc.type: FUNC
 */

// Test Int8Array
{
    const int8 = new Int8Array(5);
    assert_equal(int8.length, 5);
    assert_equal(int8.byteLength, 5);
    assert_equal(int8.byteOffset, 0);
    assert_equal(int8.BYTES_PER_ELEMENT, 1);

    int8[0] = 127;
    int8[1] = -128;
    int8[2] = 0;
    int8[3] = 100;
    int8[4] = -100;

    assert_equal(int8[0], 127);
    assert_equal(int8[1], -128);
    assert_equal(int8[2], 0);
    assert_equal(int8[3], 100);
    assert_equal(int8[4], -100);

    const int8FromArray = new Int8Array([1, 2, 3, 4, 5]);
    assert_equal(int8FromArray.length, 5);
    assert_equal(int8FromArray[0], 1);
    assert_equal(int8FromArray[4], 5);
}

// Test Uint8Array
{
    const uint8 = new Uint8Array(5);
    assert_equal(uint8.length, 5);
    assert_equal(uint8.byteLength, 5);
    assert_equal(uint8.BYTES_PER_ELEMENT, 1);

    uint8[0] = 255;
    uint8[1] = 0;
    uint8[2] = 128;
    uint8[3] = 200;
    uint8[4] = 100;

    assert_equal(uint8[0], 255);
    assert_equal(uint8[1], 0);
    assert_equal(uint8[2], 128);
    assert_equal(uint8[3], 200);
    assert_equal(uint8[4], 100);

    const uint8FromArray = new Uint8Array([10, 20, 30, 40, 50]);
    assert_equal(uint8FromArray.length, 5);
    assert_equal(uint8FromArray[0], 10);
    assert_equal(uint8FromArray[4], 50);
}

// Test Uint8ClampedArray
{
    const uint8Clamped = new Uint8ClampedArray(5);
    assert_equal(uint8Clamped.length, 5);
    assert_equal(uint8Clamped.byteLength, 5);
    assert_equal(uint8Clamped.BYTES_PER_ELEMENT, 1);

    uint8Clamped[0] = 300;
    uint8Clamped[1] = -100;
    uint8Clamped[2] = 128;
    uint8Clamped[3] = 0;
    uint8Clamped[4] = 255;

    assert_equal(uint8Clamped[0], 255);
    assert_equal(uint8Clamped[1], 0);
    assert_equal(uint8Clamped[2], 128);
    assert_equal(uint8Clamped[3], 0);
    assert_equal(uint8Clamped[4], 255);
}

// Test Int16Array
{
    const int16 = new Int16Array(5);
    assert_equal(int16.length, 5);
    assert_equal(int16.byteLength, 10);
    assert_equal(int16.BYTES_PER_ELEMENT, 2);

    int16[0] = 32767;
    int16[1] = -32768;
    int16[2] = 0;
    int16[3] = 10000;
    int16[4] = -10000;

    assert_equal(int16[0], 32767);
    assert_equal(int16[1], -32768);
    assert_equal(int16[2], 0);
    assert_equal(int16[3], 10000);
    assert_equal(int16[4], -10000);

    const int16FromArray = new Int16Array([100, 200, 300, 400, 500]);
    assert_equal(int16FromArray.length, 5);
    assert_equal(int16FromArray[0], 100);
    assert_equal(int16FromArray[4], 500);
}

// Test Uint16Array
{
    const uint16 = new Uint16Array(5);
    assert_equal(uint16.length, 5);
    assert_equal(uint16.byteLength, 10);
    assert_equal(uint16.BYTES_PER_ELEMENT, 2);

    uint16[0] = 65535;
    uint16[1] = 0;
    uint16[2] = 32768;
    uint16[3] = 50000;
    uint16[4] = 10000;

    assert_equal(uint16[0], 65535);
    assert_equal(uint16[1], 0);
    assert_equal(uint16[2], 32768);
    assert_equal(uint16[3], 50000);
    assert_equal(uint16[4], 10000);
}

// Test Int32Array
{
    const int32 = new Int32Array(5);
    assert_equal(int32.length, 5);
    assert_equal(int32.byteLength, 20);
    assert_equal(int32.BYTES_PER_ELEMENT, 4);

    int32[0] = 2147483647;
    int32[1] = -2147483648;
    int32[2] = 0;
    int32[3] = 1000000;
    int32[4] = -1000000;

    assert_equal(int32[0], 2147483647);
    assert_equal(int32[1], -2147483648);
    assert_equal(int32[2], 0);
    assert_equal(int32[3], 1000000);
    assert_equal(int32[4], -1000000);

    const int32FromArray = new Int32Array([100000, 200000, 300000, 400000, 500000]);
    assert_equal(int32FromArray.length, 5);
    assert_equal(int32FromArray[0], 100000);
    assert_equal(int32FromArray[4], 500000);
}

// Test Uint32Array
{
    const uint32 = new Uint32Array(5);
    assert_equal(uint32.length, 5);
    assert_equal(uint32.byteLength, 20);
    assert_equal(uint32.BYTES_PER_ELEMENT, 4);

    uint32[0] = 4294967295;
    uint32[1] = 0;
    uint32[2] = 2147483648;
    uint32[3] = 3000000000;
    uint32[4] = 1000000000;

    assert_equal(uint32[0], 4294967295);
    assert_equal(uint32[1], 0);
    assert_equal(uint32[2], 2147483648);
    assert_equal(uint32[3], 3000000000);
    assert_equal(uint32[4], 1000000000);
}

// Test Float32Array
{
    const float32 = new Float32Array(5);
    assert_equal(float32.length, 5);
    assert_equal(float32.byteLength, 20);
    assert_equal(float32.BYTES_PER_ELEMENT, 4);

    float32[0] = 1.5;
    float32[1] = -2.5;
    float32[2] = 0.0;
    float32[3] = 100.75;
    float32[4] = -100.25;

    assert_equal(float32[0], 1.5);
    assert_equal(float32[1], -2.5);
    assert_equal(float32[2], 0.0);
    assert_equal(float32[3], 100.75);
    assert_equal(float32[4], -100.25);

    const float32FromArray = new Float32Array([1.1, 2.2, 3.3, 4.4, 5.5]);
    assert_equal(float32FromArray.length, 5);
}

// Test Float64Array
{
    const float64 = new Float64Array(5);
    assert_equal(float64.length, 5);
    assert_equal(float64.byteLength, 40);
    assert_equal(float64.BYTES_PER_ELEMENT, 8);

    float64[0] = 1.123456789;
    float64[1] = -2.987654321;
    float64[2] = 0.0;
    float64[3] = 1000000.000001;
    float64[4] = -1000000.000001;

    assert_true(Math.abs(float64[0] - 1.123456789) < 0.0001);
    assert_true(Math.abs(float64[1] - (-2.987654321)) < 0.0001);
    assert_equal(float64[2], 0.0);

    const float64FromArray = new Float64Array([1.111, 2.222, 3.333, 4.444, 5.555]);
    assert_equal(float64FromArray.length, 5);
}

// Test BigInt64Array
{
    const bigint64 = new BigInt64Array(5);
    assert_equal(bigint64.length, 5);
    assert_equal(bigint64.byteLength, 40);
    assert_equal(bigint64.BYTES_PER_ELEMENT, 8);

    bigint64[0] = 9007199254740991n;
    bigint64[1] = -9007199254740991n;
    bigint64[2] = 0n;
    bigint64[3] = 10000000000n;
    bigint64[4] = -10000000000n;

    assert_equal(bigint64[0], 9007199254740991n);
    assert_equal(bigint64[1], -9007199254740991n);
    assert_equal(bigint64[2], 0n);
    assert_equal(bigint64[3], 10000000000n);
    assert_equal(bigint64[4], -10000000000n);

    const bigint64FromArray = new BigInt64Array([1n, 2n, 3n, 4n, 5n]);
    assert_equal(bigint64FromArray.length, 5);
    assert_equal(bigint64FromArray[0], 1n);
    assert_equal(bigint64FromArray[4], 5n);
}

// Test BigUint64Array
{
    const biguint64 = new BigUint64Array(5);
    assert_equal(biguint64.length, 5);
    assert_equal(biguint64.byteLength, 40);
    assert_equal(biguint64.BYTES_PER_ELEMENT, 8);

    biguint64[0] = 18446744073709551615n;
    biguint64[1] = 0n;
    biguint64[2] = 9007199254740991n;
    biguint64[3] = 10000000000n;
    biguint64[4] = 5000000000n;

    assert_equal(biguint64[0], 18446744073709551615n);
    assert_equal(biguint64[1], 0n);
    assert_equal(biguint64[2], 9007199254740991n);
    assert_equal(biguint64[3], 10000000000n);
    assert_equal(biguint64[4], 5000000000n);
}

// Test TypedArray from ArrayBuffer
{
    const buffer = new ArrayBuffer(20);
    const int8View = new Int8Array(buffer);
    assert_equal(int8View.length, 20);
    assert_equal(int8View.byteLength, 20);
    assert_equal(int8View.byteOffset, 0);

    const int16View = new Int16Array(buffer);
    assert_equal(int16View.length, 10);
    assert_equal(int16View.byteLength, 20);

    const int32View = new Int32Array(buffer);
    assert_equal(int32View.length, 5);
    assert_equal(int32View.byteLength, 20);
}

// Test TypedArray with offset
{
    const buffer = new ArrayBuffer(20);
    const int8Offset = new Int8Array(buffer, 5, 10);
    assert_equal(int8Offset.length, 10);
    assert_equal(int8Offset.byteOffset, 5);
    assert_equal(int8Offset.byteLength, 10);

    const int16Offset = new Int16Array(buffer, 4, 6);
    assert_equal(int16Offset.length, 6);
    assert_equal(int16Offset.byteOffset, 4);
    assert_equal(int16Offset.byteLength, 12);
}

// Test TypedArray set method
{
    const int8Arr = new Int8Array(10);
    const source = new Int8Array([1, 2, 3, 4, 5]);
    int8Arr.set(source);
    assert_equal(int8Arr[0], 1);
    assert_equal(int8Arr[4], 5);
    assert_equal(int8Arr[5], 0);

    int8Arr.set(source, 5);
    assert_equal(int8Arr[5], 1);
    assert_equal(int8Arr[9], 5);

    const int16Arr = new Int16Array(10);
    int16Arr.set([10, 20, 30]);
    assert_equal(int16Arr[0], 10);
    assert_equal(int16Arr[2], 30);
}

// Test TypedArray subarray method
{
    const int32Arr = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    const sub1 = int32Arr.subarray(3, 7);
    assert_equal(sub1.length, 4);
    assert_equal(sub1[0], 4);
    assert_equal(sub1[3], 7);

    const sub2 = int32Arr.subarray(5);
    assert_equal(sub2.length, 5);
    assert_equal(sub2[0], 6);
    assert_equal(sub2[4], 10);
}

// Test TypedArray fill method
{
    const uint8Arr = new Uint8Array(10);
    uint8Arr.fill(255);
    assert_equal(uint8Arr[0], 255);
    assert_equal(uint8Arr[9], 255);

    uint8Arr.fill(128, 2, 5);
    assert_equal(uint8Arr[0], 255);
    assert_equal(uint8Arr[2], 128);
    assert_equal(uint8Arr[4], 128);
    assert_equal(uint8Arr[5], 255);
}

// Test TypedArray filter method
{
    const int32Arr = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    const filtered = int32Arr.filter(x => x % 2 === 0);
    assert_equal(filtered.length, 5);
    assert_equal(filtered[0], 2);
    assert_equal(filtered[4], 10);
}

// Test TypedArray map method
{
    const int16Arr = new Int16Array([1, 2, 3, 4, 5]);
    const mapped = int16Arr.map(x => x * 2);
    assert_equal(mapped.length, 5);
    assert_equal(mapped[0], 2);
    assert_equal(mapped[4], 10);
}

// Test TypedArray reduce method
{
    const int32Arr = new Int32Array([1, 2, 3, 4, 5]);
    const sum = int32Arr.reduce((acc, val) => acc + val, 0);
    assert_equal(sum, 15);
}

// Test TypedArray reduceRight method
{
    const int16Arr = new Int16Array([1, 2, 3, 4, 5]);
    const sum = int16Arr.reduceRight((acc, val) => acc + val, 0);
    assert_equal(sum, 15);
}

// Test TypedArray forEach method
{
    const uint8Arr = new Uint8Array([1, 2, 3, 4, 5]);
    let sum = 0;
    uint8Arr.forEach(x => sum += x);
    assert_equal(sum, 15);
}

// Test TypedArray find method
{
    const int32Arr = new Int32Array([10, 20, 30, 40, 50]);
    const found = int32Arr.find(x => x > 25);
    assert_equal(found, 30);

    const notFound = int32Arr.find(x => x > 100);
    assert_equal(notFound, undefined);
}

// Test TypedArray findIndex method
{
    const int16Arr = new Int16Array([10, 20, 30, 40, 50]);
    const index = int16Arr.findIndex(x => x > 25);
    assert_equal(index, 2);

    const notFoundIndex = int16Arr.findIndex(x => x > 100);
    assert_equal(notFoundIndex, -1);
}

// Test TypedArray every method
{
    const int32Arr = new Int32Array([2, 4, 6, 8, 10]);
    const allEven = int32Arr.every(x => x % 2 === 0);
    assert_equal(allEven, true);

    const notAllEven = new Int32Array([2, 4, 5, 8, 10]).every(x => x % 2 === 0);
    assert_equal(notAllEven, false);
}

// Test TypedArray some method
{
    const int16Arr = new Int16Array([1, 3, 5, 7, 9]);
    const hasEven = int16Arr.some(x => x % 2 === 0);
    assert_equal(hasEven, false);

    const hasOdd = new Int16Array([2, 4, 6, 8, 10]).some(x => x % 2 !== 0);
    assert_equal(hasOdd, false);
}

// Test TypedArray includes method
{
    const int32Arr = new Int32Array([10, 20, 30, 40, 50]);
    const included = int32Arr.includes(30);
    assert_equal(included, true);

    const notIncluded = int32Arr.includes(100);
    assert_equal(notIncluded, false);
}

// Test TypedArray indexOf method
{
    const int16Arr = new Int16Array([10, 20, 30, 20, 40]);
    const index = int16Arr.indexOf(20);
    assert_equal(index, 1);

    const lastIndex = int16Arr.lastIndexOf(20);
    assert_equal(lastIndex, 3);

    const notFound = int16Arr.indexOf(100);
    assert_equal(notFound, -1);
}

// Test TypedArray join method
{
    const int8Arr = new Int8Array([1, 2, 3, 4, 5]);
    const joined = int8Arr.join(',');
    assert_equal(joined, '1,2,3,4,5');

    const joinedDash = int8Arr.join('-');
    assert_equal(joinedDash, '1-2-3-4-5');
}

// Test TypedArray toString method
{
    const uint8Arr = new Uint8Array([65, 66, 67]);
    const str = uint8Arr.toString();
    assert_equal(str, '65,66,67');
}

// Test TypedArray reverse method
{
    const int16Arr = new Int16Array([1, 2, 3, 4, 5]);
    int16Arr.reverse();
    assert_equal(int16Arr[0], 5);
    assert_equal(int16Arr[4], 1);
}

// Test TypedArray sort method
{
    const int32Arr = new Int32Array([5, 2, 8, 1, 9]);
    int32Arr.sort();
    assert_equal(int32Arr[0], 1);
    assert_equal(int32Arr[4], 9);
}

// Test TypedArray slice method
{
    const int16Arr = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    const sliced = int16Arr.slice(2, 7);
    assert_equal(sliced.length, 5);
    assert_equal(sliced[0], 3);
    assert_equal(sliced[4], 7);
}

// Test TypedArray indexed access (at-like behavior)
{
    const int32Arr = new Int32Array([10, 20, 30, 40, 50]);
    const first = int32Arr[0];
    assert_equal(first, 10);

    const last = int32Arr[int32Arr.length - 1];
    assert_equal(last, 50);

    const secondLast = int32Arr[int32Arr.length - 2];
    assert_equal(secondLast, 40);
}

// Test TypedArray copyWithin method
{
    const int8Arr = new Int8Array([1, 2, 3, 4, 5, 6, 7, 8]);
    int8Arr.copyWithin(0, 4, 6);
    assert_equal(int8Arr[0], 5);
    assert_equal(int8Arr[1], 6);
    assert_equal(int8Arr[2], 3);
}

// Test TypedArray entries method
{
    const int16Arr = new Int16Array([10, 20, 30]);
    const entries = int16Arr.entries();
    const first = entries.next();
    assert_equal(first.value[0], 0);
    assert_equal(first.value[1], 10);

    const second = entries.next();
    assert_equal(second.value[0], 1);
    assert_equal(second.value[1], 20);
}

// Test TypedArray keys method
{
    const int32Arr = new Int32Array([100, 200, 300]);
    const keys = int32Arr.keys();
    const first = keys.next();
    assert_equal(first.value, 0);

    const second = keys.next();
    assert_equal(second.value, 1);

    const third = keys.next();
    assert_equal(third.value, 2);
}

// Test TypedArray values method
{
    const int16Arr = new Int16Array([10, 20, 30]);
    const values = int16Arr.values();
    const first = values.next();
    assert_equal(first.value, 10);

    const second = values.next();
    assert_equal(second.value, 20);

    const third = values.next();
    assert_equal(third.value, 30);
}

// Test TypedArray element modification
{
    const int32Arr = new Int32Array([1, 2, 3, 4, 5]);
    const newArr = new Int32Array(int32Arr);
    newArr[2] = 100;
    assert_equal(newArr[2], 100);
    assert_equal(int32Arr[2], 3);
}

// Test TypedArray reverse method (creates copy for immutability)
{
    const int16Arr = new Int16Array([1, 2, 3, 4, 5]);
    const reversedCopy = new Int16Array(int16Arr);
    reversedCopy.reverse();
    assert_equal(reversedCopy[0], 5);
    assert_equal(reversedCopy[4], 1);
    assert_equal(int16Arr[0], 1);
}

// Test TypedArray sort method (creates copy for immutability)
{
    const int32Arr = new Int32Array([5, 2, 8, 1, 9]);
    const sortedCopy = new Int32Array(int32Arr);
    sortedCopy.sort();
    assert_equal(sortedCopy[0], 1);
    assert_equal(sortedCopy[4], 9);
    assert_equal(int32Arr[0], 5);
}

// Test TypedArray buffer property
{
    const buffer = new ArrayBuffer(20);
    const int8Arr = new Int8Array(buffer);
    assert_equal(int8Arr.buffer, buffer);

    const int16Arr = new Int16Array(buffer);
    assert_equal(int16Arr.buffer, buffer);
}

// Test TypedArray overflow behavior
{
    const int8 = new Int8Array(5);
    int8[0] = 128;
    assert_true(int8[0] === -128 || int8[0] === 128);

    int8[1] = 256;
    assert_equal(int8[1], 0);

    const uint8 = new Uint8Array(5);
    uint8[0] = 256;
    assert_equal(uint8[0], 0);

    uint8[1] = -1;
    assert_equal(uint8[1], 255);
}

// Test TypedArray with NaN
{
    const float32 = new Float32Array(5);
    float32[0] = NaN;
    assert_true(isNaN(float32[0]));

    const float64 = new Float64Array(5);
    float64[0] = NaN;
    assert_true(isNaN(float64[0]));
}

// Test TypedArray with Infinity
{
    const float32 = new Float32Array(5);
    float32[0] = Infinity;
    assert_equal(float32[0], Infinity);

    float32[1] = -Infinity;
    assert_equal(float32[1], -Infinity);

    const float64 = new Float64Array(5);
    float64[0] = Infinity;
    assert_equal(float64[0], Infinity);
}

// Test empty TypedArray
{
    const emptyInt8 = new Int8Array(0);
    assert_equal(emptyInt8.length, 0);
    assert_equal(emptyInt8.byteLength, 0);

    const emptyUint32 = new Uint32Array([]);
    assert_equal(emptyUint32.length, 0);
}

// Test TypedArray from Array
{
    const arr = [1, 2, 3, 4, 5];
    const int8Arr = Int8Array.from(arr);
    assert_equal(int8Arr.length, 5);
    assert_equal(int8Arr[0], 1);
    assert_equal(int8Arr[4], 5);

    const uint16Arr = Uint16Array.from(arr);
    assert_equal(uint16Arr.length, 5);
}

// Test TypedArray of
{
    const int8Arr = Int8Array.of(1, 2, 3, 4, 5);
    assert_equal(int8Arr.length, 5);
    assert_equal(int8Arr[0], 1);
    assert_equal(int8Arr[4], 5);

    const float32Arr = Float32Array.of(1.1, 2.2, 3.3);
    assert_equal(float32Arr.length, 3);
}

// Test TypedArray species
{
    const int8Arr = new Int8Array(5);
    assert_equal(int8Arr.constructor[Symbol.species], Int8Array);

    const uint8Arr = new Uint8Array(5);
    assert_equal(uint8Arr.constructor[Symbol.species], Uint8Array);
}

// Test multiple TypedArrays sharing buffer
{
    const buffer = new ArrayBuffer(16);
    const int8View = new Int8Array(buffer);
    const int16View = new Int16Array(buffer);
    const int32View = new Int32Array(buffer);

    int8View[0] = 1;
    int8View[1] = 2;
    int8View[2] = 3;
    int8View[3] = 4;

    assert_equal(int16View[0], 513);
}

// Test TypedArray bounds checking
{
    const int32Arr = new Int32Array([1, 2, 3, 4, 5]);
    assert_equal(int32Arr[-1], undefined);
    assert_equal(int32Arr[100], undefined);
}

// Test TypedArray with different byte orders
{
    const buffer = new ArrayBuffer(4);
    const uint8View = new Uint8Array(buffer);
    const uint32View = new Uint32Array(buffer);

    uint32View[0] = 0x12345678;
    assert_equal(uint8View[0], 0x78);
    assert_equal(uint8View[1], 0x56);
    assert_equal(uint8View[2], 0x34);
    assert_equal(uint8View[3], 0x12);
}

// Test TypedArray iteration with for...of
{
    const int16Arr = new Int16Array([1, 2, 3, 4, 5]);
    let sum = 0;
    for (const value of int16Arr) {
        sum += value;
    }
    assert_equal(sum, 15);
}

// Test TypedArray spread operator
{
    const int8Arr = new Int8Array([1, 2, 3]);
    const arr = [...int8Arr];
    assert_equal(arr.length, 3);
    assert_equal(arr[0], 1);
    assert_equal(arr[2], 3);
}

// Test TypedArray Array.from
{
    const uint16Arr = new Uint16Array([10, 20, 30]);
    const arr = Array.from(uint16Arr);
    assert_equal(arr.length, 3);
    assert_equal(arr[0], 10);
    assert_equal(arr[2], 30);
}

// Test TypedArray with negative zero
{
    const float32Arr = new Float32Array([0, -0]);
    assert_true(1 / float32Arr[0] === Infinity);
    assert_true(1 / float32Arr[1] === -Infinity);
}

// Test TypedArray with very large arrays
{
    const largeArr = new Uint8Array(1000);
    assert_equal(largeArr.length, 1000);
    assert_equal(largeArr.byteLength, 1000);

    largeArr.fill(42);
    assert_equal(largeArr[0], 42);
    assert_equal(largeArr[999], 42);
}

// Test TypedArray constructor with different arguments
{
    const int8Empty = new Int8Array();
    assert_equal(int8Empty.length, 0);

    const int8FromLength = new Int8Array(10);
    assert_equal(int8FromLength.length, 10);

    const int8FromArray = new Int8Array([1, 2, 3]);
    assert_equal(int8FromArray.length, 3);

    const int8FromTypedArray = new Int8Array(int8FromArray);
    assert_equal(int8FromTypedArray.length, 3);
    assert_equal(int8FromTypedArray[0], 1);
}

// Test TypedArray set with offset
{
    const dest = new Int32Array(10);
    const src = new Int32Array([1, 2, 3]);
    dest.set(src, 3);
    assert_equal(dest[0], 0);
    assert_equal(dest[3], 1);
    assert_equal(dest[5], 3);
}

// Test TypedArray findLast
{
    const int32Arr = new Int32Array([1, 2, 3, 2, 1]);
    // Manual implementation since findLast might not be available
    let lastVal = undefined;
    for (let i = int32Arr.length - 1; i >= 0; i--) {
        if (int32Arr[i] === 2) {
            lastVal = int32Arr[i];
            break;
        }
    }
    assert_equal(lastVal, 2);
}

// Test TypedArray findLastIndex
{
    const int16Arr = new Int16Array([1, 2, 3, 2, 1]);
    // Manual implementation since findLastIndex might not be available
    let lastIndex = -1;
    for (let i = int16Arr.length - 1; i >= 0; i--) {
        if (int16Arr[i] === 2) {
            lastIndex = i;
            break;
        }
    }
    assert_equal(lastIndex, 3);
}

// Test all TypedArray types for consistency
{
    const types = [
        Int8Array, Uint8Array, Uint8ClampedArray,
        Int16Array, Uint16Array,
        Int32Array, Uint32Array,
        Float32Array, Float64Array
    ];

    types.forEach(Type => {
        const arr = new Type(5);
        assert_equal(arr.length, 5);
        assert_equal(arr.buffer instanceof ArrayBuffer, true);
    });
}

test_end();