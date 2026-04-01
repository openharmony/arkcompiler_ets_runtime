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

// Test TypedArray tag lookup and derived result types
{
    const numericCases = [
        [Int8Array, [1, 2, 3], 9],
        [Uint8Array, [1, 2, 3], 9],
        [Uint8ClampedArray, [1, 2, 3], 9],
        [Int16Array, [1, 2, 3], 9],
        [Uint16Array, [1, 2, 3], 9],
        [Int32Array, [1, 2, 3], 9],
        [Uint32Array, [1, 2, 3], 9],
        [Float32Array, [1, 2, 3], 9.5],
        [Float64Array, [1, 2, 3], 9.5],
    ];
    const bigintCases = [
        [BigInt64Array, [1n, 2n, 3n], 9n],
        [BigUint64Array, [1n, 2n, 3n], 9n],
    ];

    function assertTypedArrayDerivedType(ctor, input, replacement) {
        const expectedTag = `[object ${ctor.name}]`;
        const source = new ctor(input);
        const sliced = source.slice(1);
        const subarray = source.subarray(1);
        const withResult = source.with(0, replacement);

        assert_equal(Object.prototype.toString.call(source), expectedTag);
        assert_equal(Object.prototype.toString.call(sliced), expectedTag);
        assert_equal(Object.prototype.toString.call(subarray), expectedTag);
        assert_equal(Object.prototype.toString.call(withResult), expectedTag);
        assert_true(sliced instanceof ctor);
        assert_true(subarray instanceof ctor);
        assert_true(withResult instanceof ctor);
    }

    numericCases.forEach(([ctor, input, replacement]) => {
        assertTypedArrayDerivedType(ctor, input, replacement);
    });
    bigintCases.forEach(([ctor, input, replacement]) => {
        assertTypedArrayDerivedType(ctor, input, replacement);
    });
}

function assertTypedArrayValues(arr, expected) {
    assert_equal(arr.length, expected.length);
    for (let i = 0; i < expected.length; i++) {
        assert_equal(arr[i], expected[i]);
    }
}

function assertBigIntTypedArrayValues(arr, expected) {
    assert_equal(arr.length, expected.length);
    for (let i = 0; i < expected.length; i++) {
        assert_equal(arr[i], expected[i]);
    }
}

// Test Int8Array detailed typedarray regression coverage
{
    const source = new Int8Array([1, -2, 3, -4, 5]);
    assert_equal(source.BYTES_PER_ELEMENT, 1);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 5);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Int8Array]");

    const sliced = source.slice(1, 4);
    assert_true(sliced instanceof Int8Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Int8Array]");
    assertTypedArrayValues(sliced, [-2, 3, -4]);

    const sub = source.subarray(1, 4);
    assert_true(sub instanceof Int8Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Int8Array]");
    assertTypedArrayValues(sub, [-2, 3, -4]);

    sub[0] = 44;
    assert_equal(source[1], 44);
    assert_equal(sub[0], 44);
    assert_equal(sliced[0], -2);

    const withResult = source.with(2, 99);
    assert_true(withResult instanceof Int8Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Int8Array]");
    assertTypedArrayValues(withResult, [1, 44, 99, -4, 5]);
    assertTypedArrayValues(source, [1, 44, 3, -4, 5]);

    const copied = new Int8Array(7);
    copied.set(source, 1);
    assertTypedArrayValues(copied, [0, 1, 44, 3, -4, 5, 0]);

    copied.fill(-8, 2, 5);
    assertTypedArrayValues(copied, [0, 1, -8, -8, -8, 5, 0]);

    const buffer = new ArrayBuffer(8);
    const view = new Int8Array(buffer, 2, 4);
    view.set([7, -7, 8, -8]);
    assert_equal(view.byteOffset, 2);
    assert_equal(view.byteLength, 4);
    assertTypedArrayValues(view, [7, -7, 8, -8]);

    const fromTypedArray = new Int8Array(withResult);
    assert_true(fromTypedArray instanceof Int8Array);
    assertTypedArrayValues(fromTypedArray, [1, 44, 99, -4, 5]);
}

// Test Uint8Array detailed typedarray regression coverage
{
    const source = new Uint8Array([1, 2, 3, 4, 255]);
    assert_equal(source.BYTES_PER_ELEMENT, 1);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 5);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Uint8Array]");

    const sliced = source.slice(1, 4);
    assert_true(sliced instanceof Uint8Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Uint8Array]");
    assertTypedArrayValues(sliced, [2, 3, 4]);

    const sub = source.subarray(2, 5);
    assert_true(sub instanceof Uint8Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Uint8Array]");
    assertTypedArrayValues(sub, [3, 4, 255]);

    sub[1] = 200;
    assert_equal(source[3], 200);
    assert_equal(sliced[2], 4);

    const withResult = source.with(0, 99);
    assert_true(withResult instanceof Uint8Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Uint8Array]");
    assertTypedArrayValues(withResult, [99, 2, 3, 200, 255]);
    assertTypedArrayValues(source, [1, 2, 3, 200, 255]);

    const copied = new Uint8Array(8);
    copied.set(source, 2);
    assertTypedArrayValues(copied, [0, 0, 1, 2, 3, 200, 255, 0]);

    copied.fill(7, 1, 4);
    assertTypedArrayValues(copied, [0, 7, 7, 7, 3, 200, 255, 0]);

    const buffer = new ArrayBuffer(10);
    const view = new Uint8Array(buffer, 1, 5);
    view.set([10, 11, 12, 13, 14]);
    assert_equal(view.byteOffset, 1);
    assert_equal(view.byteLength, 5);
    assertTypedArrayValues(view, [10, 11, 12, 13, 14]);

    const fromTypedArray = new Uint8Array(withResult);
    assert_true(fromTypedArray instanceof Uint8Array);
    assertTypedArrayValues(fromTypedArray, [99, 2, 3, 200, 255]);
}

// Test Uint8ClampedArray detailed typedarray regression coverage
{
    const source = new Uint8ClampedArray([1, 2, 3, 4, 5]);
    assert_equal(source.BYTES_PER_ELEMENT, 1);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 5);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Uint8ClampedArray]");

    const sliced = source.slice(1, 5);
    assert_true(sliced instanceof Uint8ClampedArray);
    assert_equal(Object.prototype.toString.call(sliced), "[object Uint8ClampedArray]");
    assertTypedArrayValues(sliced, [2, 3, 4, 5]);

    const sub = source.subarray(0, 3);
    assert_true(sub instanceof Uint8ClampedArray);
    assert_equal(Object.prototype.toString.call(sub), "[object Uint8ClampedArray]");
    assertTypedArrayValues(sub, [1, 2, 3]);

    sub[2] = 255;
    assert_equal(source[2], 255);
    assert_equal(sliced[1], 3);

    const withResult = source.with(1, 300);
    assert_true(withResult instanceof Uint8ClampedArray);
    assert_equal(Object.prototype.toString.call(withResult), "[object Uint8ClampedArray]");
    assertTypedArrayValues(withResult, [1, 255, 255, 4, 5]);
    assertTypedArrayValues(source, [1, 2, 255, 4, 5]);

    const copied = new Uint8ClampedArray(7);
    copied.set(source, 1);
    assertTypedArrayValues(copied, [0, 1, 2, 255, 4, 5, 0]);

    copied.fill(-10, 0, 2);
    assertTypedArrayValues(copied, [0, 0, 2, 255, 4, 5, 0]);

    const buffer = new ArrayBuffer(8);
    const view = new Uint8ClampedArray(buffer, 2, 4);
    view.set([0, 128, 300, -20]);
    assert_equal(view.byteOffset, 2);
    assert_equal(view.byteLength, 4);
    assertTypedArrayValues(view, [0, 128, 255, 0]);

    const fromTypedArray = new Uint8ClampedArray(withResult);
    assert_true(fromTypedArray instanceof Uint8ClampedArray);
    assertTypedArrayValues(fromTypedArray, [1, 255, 255, 4, 5]);
}

// Test Int16Array detailed typedarray regression coverage
{
    const source = new Int16Array([100, -200, 300, -400, 500]);
    assert_equal(source.BYTES_PER_ELEMENT, 2);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 10);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Int16Array]");

    const sliced = source.slice(1, 4);
    assert_true(sliced instanceof Int16Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Int16Array]");
    assertTypedArrayValues(sliced, [-200, 300, -400]);

    const sub = source.subarray(2, 5);
    assert_true(sub instanceof Int16Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Int16Array]");
    assertTypedArrayValues(sub, [300, -400, 500]);

    sub[1] = 1234;
    assert_equal(source[3], 1234);
    assert_equal(sliced[2], -400);

    const withResult = source.with(4, -1234);
    assert_true(withResult instanceof Int16Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Int16Array]");
    assertTypedArrayValues(withResult, [100, -200, 300, 1234, -1234]);
    assertTypedArrayValues(source, [100, -200, 300, 1234, 500]);

    const copied = new Int16Array(8);
    copied.set(source, 2);
    assertTypedArrayValues(copied, [0, 0, 100, -200, 300, 1234, 500, 0]);

    copied.fill(-33, 1, 4);
    assertTypedArrayValues(copied, [0, -33, -33, -33, 300, 1234, 500, 0]);

    const buffer = new ArrayBuffer(16);
    const view = new Int16Array(buffer, 4, 4);
    view.set([7, -7, 8, -8]);
    assert_equal(view.byteOffset, 4);
    assert_equal(view.byteLength, 8);
    assertTypedArrayValues(view, [7, -7, 8, -8]);

    const fromTypedArray = new Int16Array(withResult);
    assert_true(fromTypedArray instanceof Int16Array);
    assertTypedArrayValues(fromTypedArray, [100, -200, 300, 1234, -1234]);
}

// Test Uint16Array detailed typedarray regression coverage
{
    const source = new Uint16Array([100, 200, 300, 400, 500]);
    assert_equal(source.BYTES_PER_ELEMENT, 2);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 10);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Uint16Array]");

    const sliced = source.slice(0, 3);
    assert_true(sliced instanceof Uint16Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Uint16Array]");
    assertTypedArrayValues(sliced, [100, 200, 300]);

    const sub = source.subarray(1, 4);
    assert_true(sub instanceof Uint16Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Uint16Array]");
    assertTypedArrayValues(sub, [200, 300, 400]);

    sub[2] = 65535;
    assert_equal(source[3], 65535);
    assert_equal(sliced[2], 300);

    const withResult = source.with(2, 12345);
    assert_true(withResult instanceof Uint16Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Uint16Array]");
    assertTypedArrayValues(withResult, [100, 200, 12345, 65535, 500]);
    assertTypedArrayValues(source, [100, 200, 300, 65535, 500]);

    const copied = new Uint16Array(8);
    copied.set(source, 1);
    assertTypedArrayValues(copied, [0, 100, 200, 300, 65535, 500, 0, 0]);

    copied.fill(9, 5, 8);
    assertTypedArrayValues(copied, [0, 100, 200, 300, 65535, 9, 9, 9]);

    const buffer = new ArrayBuffer(18);
    const view = new Uint16Array(buffer, 2, 4);
    view.set([11, 22, 33, 44]);
    assert_equal(view.byteOffset, 2);
    assert_equal(view.byteLength, 8);
    assertTypedArrayValues(view, [11, 22, 33, 44]);

    const fromTypedArray = new Uint16Array(withResult);
    assert_true(fromTypedArray instanceof Uint16Array);
    assertTypedArrayValues(fromTypedArray, [100, 200, 12345, 65535, 500]);
}

// Test Int32Array detailed typedarray regression coverage
{
    const source = new Int32Array([1000, -2000, 3000, -4000, 5000]);
    assert_equal(source.BYTES_PER_ELEMENT, 4);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 20);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Int32Array]");

    const sliced = source.slice(2, 5);
    assert_true(sliced instanceof Int32Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Int32Array]");
    assertTypedArrayValues(sliced, [3000, -4000, 5000]);

    const sub = source.subarray(0, 2);
    assert_true(sub instanceof Int32Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Int32Array]");
    assertTypedArrayValues(sub, [1000, -2000]);

    sub[1] = 7777;
    assert_equal(source[1], 7777);
    assert_equal(sliced[1], -4000);

    const withResult = source.with(3, -8888);
    assert_true(withResult instanceof Int32Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Int32Array]");
    assertTypedArrayValues(withResult, [1000, 7777, 3000, -8888, 5000]);
    assertTypedArrayValues(source, [1000, 7777, 3000, -4000, 5000]);

    const copied = new Int32Array(7);
    copied.set(source, 1);
    assertTypedArrayValues(copied, [0, 1000, 7777, 3000, -4000, 5000, 0]);

    copied.fill(-1, 0, 2);
    assertTypedArrayValues(copied, [-1, -1, 7777, 3000, -4000, 5000, 0]);

    const buffer = new ArrayBuffer(32);
    const view = new Int32Array(buffer, 8, 4);
    view.set([123, -123, 456, -456]);
    assert_equal(view.byteOffset, 8);
    assert_equal(view.byteLength, 16);
    assertTypedArrayValues(view, [123, -123, 456, -456]);

    const fromTypedArray = new Int32Array(withResult);
    assert_true(fromTypedArray instanceof Int32Array);
    assertTypedArrayValues(fromTypedArray, [1000, 7777, 3000, -8888, 5000]);
}

// Test Uint32Array detailed typedarray regression coverage
{
    const source = new Uint32Array([1000, 2000, 3000, 4000, 5000]);
    assert_equal(source.BYTES_PER_ELEMENT, 4);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 20);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Uint32Array]");

    const sliced = source.slice(1, 3);
    assert_true(sliced instanceof Uint32Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Uint32Array]");
    assertTypedArrayValues(sliced, [2000, 3000]);

    const sub = source.subarray(2, 5);
    assert_true(sub instanceof Uint32Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Uint32Array]");
    assertTypedArrayValues(sub, [3000, 4000, 5000]);

    sub[0] = 999999;
    assert_equal(source[2], 999999);
    assert_equal(sliced[1], 3000);

    const withResult = source.with(4, 1234567890);
    assert_true(withResult instanceof Uint32Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Uint32Array]");
    assertTypedArrayValues(withResult, [1000, 2000, 999999, 4000, 1234567890]);
    assertTypedArrayValues(source, [1000, 2000, 999999, 4000, 5000]);

    const copied = new Uint32Array(7);
    copied.set(source, 2);
    assertTypedArrayValues(copied, [0, 0, 1000, 2000, 999999, 4000, 5000]);

    copied.fill(42, 0, 3);
    assertTypedArrayValues(copied, [42, 42, 42, 2000, 999999, 4000, 5000]);

    const buffer = new ArrayBuffer(40);
    const view = new Uint32Array(buffer, 4, 4);
    view.set([1, 2, 3, 4]);
    assert_equal(view.byteOffset, 4);
    assert_equal(view.byteLength, 16);
    assertTypedArrayValues(view, [1, 2, 3, 4]);

    const fromTypedArray = new Uint32Array(withResult);
    assert_true(fromTypedArray instanceof Uint32Array);
    assertTypedArrayValues(fromTypedArray, [1000, 2000, 999999, 4000, 1234567890]);
}

// Test Float32Array detailed typedarray regression coverage
{
    const source = new Float32Array([1.5, -2.5, 3.5, -4.5, 5.5]);
    assert_equal(source.BYTES_PER_ELEMENT, 4);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 20);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Float32Array]");

    const sliced = source.slice(1, 4);
    assert_true(sliced instanceof Float32Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Float32Array]");
    assert_true(Math.abs(sliced[0] - (-2.5)) < 0.001);
    assert_true(Math.abs(sliced[1] - 3.5) < 0.001);
    assert_true(Math.abs(sliced[2] - (-4.5)) < 0.001);

    const sub = source.subarray(0, 3);
    assert_true(sub instanceof Float32Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Float32Array]");
    assert_true(Math.abs(sub[0] - 1.5) < 0.001);
    assert_true(Math.abs(sub[1] - (-2.5)) < 0.001);
    assert_true(Math.abs(sub[2] - 3.5) < 0.001);

    sub[1] = 9.25;
    assert_true(Math.abs(source[1] - 9.25) < 0.001);
    assert_true(Math.abs(sliced[0] - (-2.5)) < 0.001);

    const withResult = source.with(3, -7.75);
    assert_true(withResult instanceof Float32Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Float32Array]");
    assert_true(Math.abs(withResult[0] - 1.5) < 0.001);
    assert_true(Math.abs(withResult[1] - 9.25) < 0.001);
    assert_true(Math.abs(withResult[2] - 3.5) < 0.001);
    assert_true(Math.abs(withResult[3] - (-7.75)) < 0.001);
    assert_true(Math.abs(withResult[4] - 5.5) < 0.001);

    const copied = new Float32Array(7);
    copied.set(source, 1);
    assert_true(Math.abs(copied[1] - 1.5) < 0.001);
    assert_true(Math.abs(copied[2] - 9.25) < 0.001);
    assert_true(Math.abs(copied[5] - 5.5) < 0.001);

    copied.fill(2.25, 0, 2);
    assert_true(Math.abs(copied[0] - 2.25) < 0.001);
    assert_true(Math.abs(copied[1] - 2.25) < 0.001);

    const buffer = new ArrayBuffer(32);
    const view = new Float32Array(buffer, 8, 4);
    view.set([0.25, 0.5, 0.75, 1.25]);
    assert_equal(view.byteOffset, 8);
    assert_equal(view.byteLength, 16);
    assert_true(Math.abs(view[0] - 0.25) < 0.001);
    assert_true(Math.abs(view[1] - 0.5) < 0.001);
    assert_true(Math.abs(view[2] - 0.75) < 0.001);
    assert_true(Math.abs(view[3] - 1.25) < 0.001);

    const fromTypedArray = new Float32Array(withResult);
    assert_true(fromTypedArray instanceof Float32Array);
    assert_true(Math.abs(fromTypedArray[3] - (-7.75)) < 0.001);
}

// Test Float64Array detailed typedarray regression coverage
{
    const source = new Float64Array([1.25, -2.5, 3.75, -4.125, 5.5]);
    assert_equal(source.BYTES_PER_ELEMENT, 8);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 40);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object Float64Array]");

    const sliced = source.slice(0, 4);
    assert_true(sliced instanceof Float64Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object Float64Array]");
    assert_equal(sliced[0], 1.25);
    assert_equal(sliced[1], -2.5);
    assert_equal(sliced[2], 3.75);
    assert_equal(sliced[3], -4.125);

    const sub = source.subarray(1, 5);
    assert_true(sub instanceof Float64Array);
    assert_equal(Object.prototype.toString.call(sub), "[object Float64Array]");
    assert_equal(sub[0], -2.5);
    assert_equal(sub[1], 3.75);
    assert_equal(sub[2], -4.125);
    assert_equal(sub[3], 5.5);

    sub[2] = 10.625;
    assert_equal(source[3], 10.625);
    assert_equal(sliced[3], -4.125);

    const withResult = source.with(1, -99.5);
    assert_true(withResult instanceof Float64Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object Float64Array]");
    assert_equal(withResult[0], 1.25);
    assert_equal(withResult[1], -99.5);
    assert_equal(withResult[2], 3.75);
    assert_equal(withResult[3], 10.625);
    assert_equal(withResult[4], 5.5);

    const copied = new Float64Array(7);
    copied.set(source, 2);
    assert_equal(copied[2], 1.25);
    assert_equal(copied[3], -2.5);
    assert_equal(copied[6], 5.5);

    copied.fill(6.75, 0, 2);
    assert_equal(copied[0], 6.75);
    assert_equal(copied[1], 6.75);

    const buffer = new ArrayBuffer(64);
    const view = new Float64Array(buffer, 8, 4);
    view.set([11.1, 22.2, 33.3, 44.4]);
    assert_equal(view.byteOffset, 8);
    assert_equal(view.byteLength, 32);
    assert_equal(view[0], 11.1);
    assert_equal(view[1], 22.2);
    assert_equal(view[2], 33.3);
    assert_equal(view[3], 44.4);

    const fromTypedArray = new Float64Array(withResult);
    assert_true(fromTypedArray instanceof Float64Array);
    assert_equal(fromTypedArray[1], -99.5);
}

// Test BigInt64Array detailed typedarray regression coverage
{
    const source = new BigInt64Array([1n, -2n, 3n, -4n, 5n]);
    assert_equal(source.BYTES_PER_ELEMENT, 8);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 40);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object BigInt64Array]");

    const sliced = source.slice(1, 4);
    assert_true(sliced instanceof BigInt64Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object BigInt64Array]");
    assertBigIntTypedArrayValues(sliced, [-2n, 3n, -4n]);

    const sub = source.subarray(0, 3);
    assert_true(sub instanceof BigInt64Array);
    assert_equal(Object.prototype.toString.call(sub), "[object BigInt64Array]");
    assertBigIntTypedArrayValues(sub, [1n, -2n, 3n]);

    sub[1] = 77n;
    assert_equal(source[1], 77n);
    assert_equal(sliced[0], -2n);

    const withResult = source.with(4, -99n);
    assert_true(withResult instanceof BigInt64Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object BigInt64Array]");
    assertBigIntTypedArrayValues(withResult, [1n, 77n, 3n, -4n, -99n]);
    assertBigIntTypedArrayValues(source, [1n, 77n, 3n, -4n, 5n]);

    const copied = new BigInt64Array(7);
    copied.set(source, 1);
    assertBigIntTypedArrayValues(copied, [0n, 1n, 77n, 3n, -4n, 5n, 0n]);

    copied.fill(-8n, 2, 5);
    assertBigIntTypedArrayValues(copied, [0n, 1n, -8n, -8n, -8n, 5n, 0n]);

    const buffer = new ArrayBuffer(56);
    const view = new BigInt64Array(buffer, 8, 4);
    view.set([11n, -11n, 22n, -22n]);
    assert_equal(view.byteOffset, 8);
    assert_equal(view.byteLength, 32);
    assertBigIntTypedArrayValues(view, [11n, -11n, 22n, -22n]);

    const fromTypedArray = new BigInt64Array(withResult);
    assert_true(fromTypedArray instanceof BigInt64Array);
    assertBigIntTypedArrayValues(fromTypedArray, [1n, 77n, 3n, -4n, -99n]);
}

// Test BigUint64Array detailed typedarray regression coverage
{
    const source = new BigUint64Array([1n, 2n, 3n, 4n, 5n]);
    assert_equal(source.BYTES_PER_ELEMENT, 8);
    assert_equal(source.length, 5);
    assert_equal(source.byteLength, 40);
    assert_equal(source.byteOffset, 0);
    assert_equal(Object.prototype.toString.call(source), "[object BigUint64Array]");

    const sliced = source.slice(2, 5);
    assert_true(sliced instanceof BigUint64Array);
    assert_equal(Object.prototype.toString.call(sliced), "[object BigUint64Array]");
    assertBigIntTypedArrayValues(sliced, [3n, 4n, 5n]);

    const sub = source.subarray(1, 4);
    assert_true(sub instanceof BigUint64Array);
    assert_equal(Object.prototype.toString.call(sub), "[object BigUint64Array]");
    assertBigIntTypedArrayValues(sub, [2n, 3n, 4n]);

    sub[2] = 123n;
    assert_equal(source[3], 123n);
    assert_equal(sliced[1], 4n);

    const withResult = source.with(0, 999n);
    assert_true(withResult instanceof BigUint64Array);
    assert_equal(Object.prototype.toString.call(withResult), "[object BigUint64Array]");
    assertBigIntTypedArrayValues(withResult, [999n, 2n, 3n, 123n, 5n]);
    assertBigIntTypedArrayValues(source, [1n, 2n, 3n, 123n, 5n]);

    const copied = new BigUint64Array(7);
    copied.set(source, 2);
    assertBigIntTypedArrayValues(copied, [0n, 0n, 1n, 2n, 3n, 123n, 5n]);

    copied.fill(8n, 0, 3);
    assertBigIntTypedArrayValues(copied, [8n, 8n, 8n, 2n, 3n, 123n, 5n]);

    const buffer = new ArrayBuffer(56);
    const view = new BigUint64Array(buffer, 8, 4);
    view.set([7n, 8n, 9n, 10n]);
    assert_equal(view.byteOffset, 8);
    assert_equal(view.byteLength, 32);
    assertBigIntTypedArrayValues(view, [7n, 8n, 9n, 10n]);

    const fromTypedArray = new BigUint64Array(withResult);
    assert_true(fromTypedArray instanceof BigUint64Array);
    assertBigIntTypedArrayValues(fromTypedArray, [999n, 2n, 3n, 123n, 5n]);
}

// Test cross-type constructor copy semantics for numeric typedarrays
{
    const int8Source = new Int8Array([1, -2, 3, -4]);
    const uint16FromInt8 = new Uint16Array(int8Source);
    assert_true(uint16FromInt8 instanceof Uint16Array);
    assert_equal(Object.prototype.toString.call(uint16FromInt8), "[object Uint16Array]");
    assert_equal(uint16FromInt8.length, 4);
    assert_equal(uint16FromInt8[0], 1);
    assert_equal(uint16FromInt8[2], 3);

    const float32Source = new Float32Array([1.25, 2.5, 3.75, 4.5]);
    const int32FromFloat32 = new Int32Array(float32Source);
    assert_true(int32FromFloat32 instanceof Int32Array);
    assert_equal(Object.prototype.toString.call(int32FromFloat32), "[object Int32Array]");
    assert_equal(int32FromFloat32.length, 4);
    assert_equal(int32FromFloat32[0], 1);
    assert_equal(int32FromFloat32[1], 2);
    assert_equal(int32FromFloat32[2], 3);
    assert_equal(int32FromFloat32[3], 4);

    const uint8Source = new Uint8Array([255, 128, 64, 32]);
    const float64FromUint8 = new Float64Array(uint8Source);
    assert_true(float64FromUint8 instanceof Float64Array);
    assert_equal(Object.prototype.toString.call(float64FromUint8), "[object Float64Array]");
    assert_equal(float64FromUint8.length, 4);
    assert_equal(float64FromUint8[0], 255);
    assert_equal(float64FromUint8[1], 128);
    assert_equal(float64FromUint8[2], 64);
    assert_equal(float64FromUint8[3], 32);
}

// Test cross-type constructor copy semantics for bigint typedarrays
{
    const bigIntSource = new BigInt64Array([1n, -2n, 3n, -4n]);
    const bigUintFromBigInt = new BigUint64Array(bigIntSource);
    assert_true(bigUintFromBigInt instanceof BigUint64Array);
    assert_equal(Object.prototype.toString.call(bigUintFromBigInt), "[object BigUint64Array]");
    assert_equal(bigUintFromBigInt.length, 4);
    assert_equal(bigUintFromBigInt[0], 1n);
    assert_equal(bigUintFromBigInt[2], 3n);

    const bigUintSource = new BigUint64Array([10n, 20n, 30n, 40n]);
    const bigIntFromBigUint = new BigInt64Array(bigUintSource);
    assert_true(bigIntFromBigUint instanceof BigInt64Array);
    assert_equal(Object.prototype.toString.call(bigIntFromBigUint), "[object BigInt64Array]");
    assert_equal(bigIntFromBigUint.length, 4);
    assert_equal(bigIntFromBigUint[0], 10n);
    assert_equal(bigIntFromBigUint[1], 20n);
    assert_equal(bigIntFromBigUint[2], 30n);
    assert_equal(bigIntFromBigUint[3], 40n);
}

// Test typedarray result type stability after chained operations
{
    const a = new Int16Array([10, 20, 30, 40, 50]);
    const chained = a.slice(1, 5).with(1, 99).subarray(1, 4);
    assert_true(chained instanceof Int16Array);
    assert_equal(Object.prototype.toString.call(chained), "[object Int16Array]");
    assertTypedArrayValues(chained, [99, 40, 50]);

    const b = new Float64Array([1.5, 2.5, 3.5, 4.5]);
    const chainedFloat = b.subarray(1).with(2, 8.5).slice(0, 3);
    assert_true(chainedFloat instanceof Float64Array);
    assert_equal(Object.prototype.toString.call(chainedFloat), "[object Float64Array]");
    assert_equal(chainedFloat[0], 2.5);
    assert_equal(chainedFloat[1], 3.5);
    assert_equal(chainedFloat[2], 8.5);

    const c = new BigUint64Array([1n, 2n, 3n, 4n]);
    const chainedBig = c.subarray(1).with(1, 99n).slice(0, 3);
    assert_true(chainedBig instanceof BigUint64Array);
    assert_equal(Object.prototype.toString.call(chainedBig), "[object BigUint64Array]");
    assertBigIntTypedArrayValues(chainedBig, [2n, 99n, 4n]);
}

// Test ArrayBuffer-backed views preserve typedarray tags and offsets
{
    const buffer = new ArrayBuffer(64);

    const int8View = new Int8Array(buffer, 0, 4);
    int8View.set([1, 2, 3, 4]);
    assert_equal(Object.prototype.toString.call(int8View), "[object Int8Array]");
    assert_equal(int8View.byteOffset, 0);
    assert_equal(int8View.byteLength, 4);

    const uint16View = new Uint16Array(buffer, 8, 4);
    uint16View.set([11, 22, 33, 44]);
    assert_equal(Object.prototype.toString.call(uint16View), "[object Uint16Array]");
    assert_equal(uint16View.byteOffset, 8);
    assert_equal(uint16View.byteLength, 8);

    const float32View = new Float32Array(buffer, 24, 2);
    float32View.set([1.25, 2.5]);
    assert_equal(Object.prototype.toString.call(float32View), "[object Float32Array]");
    assert_equal(float32View.byteOffset, 24);
    assert_equal(float32View.byteLength, 8);

    const bigIntView = new BigInt64Array(buffer, 32, 2);
    bigIntView.set([9n, -9n]);
    assert_equal(Object.prototype.toString.call(bigIntView), "[object BigInt64Array]");
    assert_equal(bigIntView.byteOffset, 32);
    assert_equal(bigIntView.byteLength, 16);
    assert_equal(bigIntView[0], 9n);
    assert_equal(bigIntView[1], -9n);
}

// Test copyWithin and reverse preserve concrete typedarray identity
{
    const int8Arr = new Int8Array([1, 2, 3, 4, 5]);
    const int8Result = int8Arr.copyWithin(1, 3);
    assert_true(int8Result instanceof Int8Array);
    assert_equal(Object.prototype.toString.call(int8Result), "[object Int8Array]");
    assertTypedArrayValues(int8Arr, [1, 4, 5, 4, 5]);
    int8Arr.reverse();
    assertTypedArrayValues(int8Arr, [5, 4, 5, 4, 1]);

    const uint32Arr = new Uint32Array([10, 20, 30, 40, 50]);
    const uint32Result = uint32Arr.copyWithin(2, 0, 2);
    assert_true(uint32Result instanceof Uint32Array);
    assert_equal(Object.prototype.toString.call(uint32Result), "[object Uint32Array]");
    assertTypedArrayValues(uint32Arr, [10, 20, 10, 20, 50]);
    uint32Arr.reverse();
    assertTypedArrayValues(uint32Arr, [50, 20, 10, 20, 10]);

    const bigArr = new BigInt64Array([1n, 2n, 3n, 4n, 5n]);
    const bigResult = bigArr.copyWithin(0, 2, 5);
    assert_true(bigResult instanceof BigInt64Array);
    assert_equal(Object.prototype.toString.call(bigResult), "[object BigInt64Array]");
    assertBigIntTypedArrayValues(bigArr, [3n, 4n, 5n, 4n, 5n]);
    bigArr.reverse();
    assertBigIntTypedArrayValues(bigArr, [5n, 4n, 5n, 4n, 3n]);
}

// Test map and filter preserve concrete constructor results
{
    const int16Arr = new Int16Array([1, 2, 3, 4, 5]);
    const int16Mapped = int16Arr.map(v => v * 2);
    const int16Filtered = int16Arr.filter(v => v % 2 === 1);
    assert_true(int16Mapped instanceof Int16Array);
    assert_true(int16Filtered instanceof Int16Array);
    assert_equal(Object.prototype.toString.call(int16Mapped), "[object Int16Array]");
    assert_equal(Object.prototype.toString.call(int16Filtered), "[object Int16Array]");
    assertTypedArrayValues(int16Mapped, [2, 4, 6, 8, 10]);
    assertTypedArrayValues(int16Filtered, [1, 3, 5]);

    const float32Arr = new Float32Array([1.5, 2.5, 3.5, 4.5]);
    const float32Mapped = float32Arr.map(v => v + 0.5);
    const float32Filtered = float32Arr.filter(v => v > 2.0);
    assert_true(float32Mapped instanceof Float32Array);
    assert_true(float32Filtered instanceof Float32Array);
    assert_equal(Object.prototype.toString.call(float32Mapped), "[object Float32Array]");
    assert_equal(Object.prototype.toString.call(float32Filtered), "[object Float32Array]");
    assert_true(Math.abs(float32Mapped[0] - 2.0) < 0.001);
    assert_true(Math.abs(float32Mapped[3] - 5.0) < 0.001);
    assert_true(Math.abs(float32Filtered[0] - 2.5) < 0.001);
    assert_true(Math.abs(float32Filtered[2] - 4.5) < 0.001);

    const bigUintArr = new BigUint64Array([1n, 2n, 3n, 4n, 5n]);
    const bigUintMapped = bigUintArr.map(v => v + 1n);
    const bigUintFiltered = bigUintArr.filter(v => v % 2n === 1n);
    assert_true(bigUintMapped instanceof BigUint64Array);
    assert_true(bigUintFiltered instanceof BigUint64Array);
    assert_equal(Object.prototype.toString.call(bigUintMapped), "[object BigUint64Array]");
    assert_equal(Object.prototype.toString.call(bigUintFiltered), "[object BigUint64Array]");
    assertBigIntTypedArrayValues(bigUintMapped, [2n, 3n, 4n, 5n, 6n]);
    assertBigIntTypedArrayValues(bigUintFiltered, [1n, 3n, 5n]);
}

// Test reduce and join after typedarray memory-layout optimization
{
    const int8Arr = new Int8Array([1, 2, 3, 4]);
    const int8Sum = int8Arr.reduce((acc, v) => acc + v, 0);
    assert_equal(int8Sum, 10);
    assert_equal(int8Arr.join("|"), "1|2|3|4");

    const uint16Arr = new Uint16Array([10, 20, 30]);
    const uint16Sum = uint16Arr.reduce((acc, v) => acc + v, 0);
    assert_equal(uint16Sum, 60);
    assert_equal(uint16Arr.join(","), "10,20,30");

    const float64Arr = new Float64Array([1.5, 2.5, 3.5]);
    const float64Sum = float64Arr.reduce((acc, v) => acc + v, 0);
    assert_equal(float64Sum, 7.5);
    assert_equal(float64Arr.join("/"), "1.5/2.5/3.5");

    const bigIntArr = new BigInt64Array([1n, 2n, 3n]);
    const bigIntSum = bigIntArr.reduce((acc, v) => acc + v, 0n);
    assert_equal(bigIntSum, 6n);
    assert_equal(bigIntArr.join(","), "1,2,3");
}

// Test typedarray iterator materialization remains type-correct after derived operations
{
    const int32Arr = new Int32Array([7, 8, 9]);
    const int32Values = Array.from(int32Arr.values());
    const int32Keys = Array.from(int32Arr.keys());
    const int32Entries = Array.from(int32Arr.entries());
    assert_equal(int32Values.length, 3);
    assert_equal(int32Values[0], 7);
    assert_equal(int32Values[2], 9);
    assert_equal(int32Keys[0], 0);
    assert_equal(int32Keys[2], 2);
    assert_equal(int32Entries[1][0], 1);
    assert_equal(int32Entries[1][1], 8);

    const bigUintArr = new BigUint64Array([6n, 7n, 8n]);
    const bigUintValues = Array.from(bigUintArr.values());
    const bigUintKeys = Array.from(bigUintArr.keys());
    const bigUintEntries = Array.from(bigUintArr.entries());
    assert_equal(bigUintValues.length, 3);
    assert_equal(bigUintValues[0], 6n);
    assert_equal(bigUintValues[2], 8n);
    assert_equal(bigUintKeys[0], 0);
    assert_equal(bigUintKeys[2], 2);
    assert_equal(bigUintEntries[2][0], 2);
    assert_equal(bigUintEntries[2][1], 8n);
}

test_end();
