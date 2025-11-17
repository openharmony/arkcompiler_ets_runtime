/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 * @tc.name: cppcallirbuiltins
 * @tc.desc: test cpp call ir builtins
 * @tc.type: FUNC
 */

/*******************************************************
 * Custom replace logic via Symbol.replace
 ******************************************************/
{
    function testSymbolReplace() {
        class ReplaceClass {
            constructor(value) {
                this.value = value;
            }
            [Symbol.replace](str) {
                return `s/${str}/${this.value}/g`;
            }
        }
        'hu'.replace(new ReplaceClass('an'));
    }

    testSymbolReplace();
}

/*******************************************************
 * Control derived-object species via Symbol.species
 ******************************************************/
{
    function testSymbolSpecies() {
        class Arr extends Array {
            static get [Symbol.species]() {
                return Array;
            }
        }
        let arr1 = new Arr(1, 2, 3, 4, 5);
        let mapped = arr1.map((x) => x * x);
        mapped instanceof Array;
    }

    testSymbolSpecies();
}

/*******************************************************
 * Seal object to prevent extensions
 ******************************************************/
{
    let obj = { a: 4 };
    function testObjectSeal() {
        Object.seal(obj);
        obj.a = 33;
    }
    function testObjectSealNumber() {
        Object.seal(1);
    }

    testObjectSeal();
    testObjectSealNumber();
}

/*******************************************************
 * Build Int8Array from iterable
 ******************************************************/
{
    function int8ArrayFromIterable() {
        const iterable = (function* () {
            yield* [1, 2, 3];
        })();
        const int8 = new Int8Array(iterable);
    }

    int8ArrayFromIterable();
}

/*******************************************************
 * In-place sort via Array.prototype.sort
 ******************************************************/
{
    let result;
    let array1 = [];
    let array2 = [];
    let array3 = [];
    let array4 = [];
    let arraySize = 100;
    for (let i = 0; i < arraySize; ++i) array1[i] = i;
    for (let i = 0; i < arraySize; ++i) array2[i] = i + 0.5;
    for (let i = 0; i < arraySize; i++) array3[i] = `string.${i}`;
    for (let i = 0; i < arraySize; i++) array4[i] = { ["obj" + i]: i };
    function smiSort() {
        result = null;
        result = array1.sort();
    }
    function doubleSort() {
        result = null;
        result = array2.sort();
    }
    function stringSort() {
        result = null;
        result = array3.sort();
    }
    function objectSort() {
        result = null;
        result = array4.sort();
    }

    smiSort();
    doubleSort();
    stringSort();
    objectSort();
}

/*******************************************************
 * Non-destructive sort via Array.prototype.toSorted
 ******************************************************/
{
    let result;
    let array1 = [];
    let array2 = [];
    let array3 = [];
    let array4 = [];
    let arraySize = 100;
    for (let i = 0; i < arraySize; ++i) array1[i] = i;
    for (let i = 0; i < arraySize; ++i) array2[i] = i + 0.5;
    for (let i = 0; i < arraySize; i++) array3[i] = `string.${i}`;
    for (let i = 0; i < arraySize; i++) array4[i] = { ["obj" + i]: i };
    function smiToSorted() {
        result = null;
        result = array1.toSorted();
    }
    function doubleToSorted() {
        result = null;
        result = array2.toSorted();
    }
    function stringToSorted() {
        result = null;
        result = array3.toSorted();
    }
    function objectToSorted() {
        result = null;
        result = array4.toSorted();
    }

    smiToSorted();
    doubleToSorted();
    stringToSorted();
    objectToSorted();
}

/*******************************************************
 * Set constructor with various iterable shapes
 ******************************************************/
{
    let set = new Set();
    function setConstructorSmi() {
        set = new Set([1, 2, 3, 4, 5]);
    }
    function setConstructorString() {
        set = new Set('1.5', 'for', 'hua', 'bar', 'faz');
    }
    function setConstructorObject() {
        set = new Set();
    }
    function setConstructorDouble() {
        set = new Set([1.5, 2.5, 3.5, 4.5, 5.5]);
    }

    setConstructorSmi();
    setConstructorString();
    setConstructorObject();
    setConstructorDouble();
}

/*******************************************************
 * Generic factory via TypedArray.from
 ******************************************************/
{
    function testTypedArrayFrom(e) {
        let s = new Set([-5, 10, 20, 30, -40, 50.5, -60.6]);
        let result = e.from(s);
    }

    function testTypedArrayFromInt16Array(e) {
        let s = new Set('12345');
        let result = e.from(s);
    }

    function testTypedArrayFromFloat32Array() {
        let result = Float32Array.from([1, 2, 3], (x) => x + x);
    }

    function testTypedArrayFromUint8Array() {
        let result = Uint8Array.from({ length: 5 }, (v, k) => k);
    }

    testTypedArrayFrom(Int8Array);
    testTypedArrayFromUint8Array();
    testTypedArrayFrom(Uint8ClampedArray);
    testTypedArrayFromInt16Array(Int16Array);
    testTypedArrayFrom(Uint16Array);
    testTypedArrayFrom(Int32Array);
    testTypedArrayFrom(Uint32Array);
    testTypedArrayFromFloat32Array();
    testTypedArrayFrom(Float64Array);
}

/*******************************************************
 * Reverse sparse array via Array.prototype.reverse
 ******************************************************/
{
    function testSparseReverse() {
        let a = [0,, 2,, 4];
        a.reverse();
    }
    testSparseReverse();
}

/*******************************************************
 * Map constructor with iterator closing
 ******************************************************/
{
    function testMapIteratorClose() {
        function* gen() {
            yield ['a', 1];
            yield ['b', 2];
            throw new Error('intentional');
        }
        try {
            new Map(gen());
        } catch (e) {
            // swallow
        }
    }
    testMapIteratorClose();
}

/*******************************************************
 * Edge cases for Number.prototype.toFixed
 ******************************************************/
{
    function testToFixedBoundary() {
        (1.23456789).toFixed(100);
        (987654321.123456789).toFixed(20);
    }
    testToFixedBoundary();
}

/*******************************************************
 * Date constructor with out-of-range month
 ******************************************************/
{
    function testDateOverflow() {
        new Date(2025, 12, 1);
        new Date(2025, -1, 1);
    }
    testDateOverflow();
}

/*******************************************************
 * Promise.allSettled with already resolved promises
 ******************************************************/
{
    function testPromiseAllSettledFastPath() {
        Promise.allSettled([
            Promise.resolve(1),
            Promise.resolve(2),
            Promise.resolve(3)
        ]);
    }
    testPromiseAllSettledFastPath();
}

/*******************************************************
 * Date.UTC with out-of-range fields
 ******************************************************/
{
    function testDateUTCOverflow() {
        new Date(Date.UTC(2025, 12, 1));
        new Date(Date.UTC(2025, -1, 1));
        new Date(Date.UTC(2025, 0, 32));
    }
    testDateUTCOverflow();
}

/*******************************************************
 * RegExp.prototype.exec with sticky flag
 ******************************************************/
{
    function testRegExpSticky() {
        const re = /foo/y;
        re.lastIndex = 0;
        re.exec('foofoofoo');
        re.exec('foofoofoo');
        re.exec('foofoofoo');
        re.exec('foofoofoo');
    }
    testRegExpSticky();
}

/*******************************************************
 * Array.prototype.includes on sparse array
 ******************************************************/
{
    function testSparseIncludes() {
        const a = [0, , 2];
        a.includes(undefined);
        a.includes(2);
        a.includes(1);
    }
    testSparseIncludes();
}

/*******************************************************
 * Object.is with edge-case values
 ******************************************************/
{
    function testObjectIs() {
        Object.is(-0, +0);
        Object.is(NaN, NaN);
        Object.is(42, 42);
    }
    testObjectIs();
}

/*******************************************************
 * BigInt truncation via asIntN / asUintN
 ******************************************************/
{
    function testBigIntAsIntN() {
        const big = 123456789012345678901234567890n;
        BigInt.asIntN(64, big);
        BigInt.asUintN(32, big);
    }
    testBigIntAsIntN();
}

/*******************************************************
 * WeakRef and FinalizationRegistry basic flow
 ******************************************************/
{
    function testWeakRef() {
        let obj = { id: 42 };
        const wr = new WeakRef(obj);
        const reg = new FinalizationRegistry((held) => {});
        reg.register(obj, "cleanup");
        obj = null;
        wr.deref();
    }
    testWeakRef();
}

/*******************************************************
 * TypedArray.prototype.set with offset
 ******************************************************/
{
    function testTypedArraySetOffset() {
        const src = new Uint8Array([10, 20, 30]);
        const dest = new Uint8Array(10);
        dest.set(src, 4);
    }
    testTypedArraySetOffset();
}

/*******************************************************
 * String.raw with escape sequences
 ******************************************************/
{
    function testStringRaw() {
        String.raw`C:\Development\profile\aboutme.html`;
        String.raw`line1\nline2`;
    }
    testStringRaw();
}

/*******************************************************
 * Math.hypot with overflow-prone values
 ******************************************************/
{
    function testMathHypot() {
        Math.hypot(1e200, 1e200);
        Math.hypot(3, 4);
    }
    testMathHypot();
}

/*******************************************************
 * Reflect.construct argument array
 ******************************************************/
{
    function testReflectConstruct() {
        function Base() {}
        function Derived() {}
        const instance = Reflect.construct(Base, [1, 2, 3], Derived);
        instance instanceof Derived;
    }
    testReflectConstruct();
}

/*******************************************************
 * decodeURI with incomplete escape triplet
 ******************************************************/
{
    function testDecodeURIError() {
        try {
            decodeURI('foo%2');
        } catch (e) {
            // expect URIError
        }
    }
    testDecodeURIError();
}

/*******************************************************
 * Proxy trap 'has' on with-in check
 ******************************************************/
{
    function testProxyHasTrap() {
        const target = { a: 1 };
        const proxy = new Proxy(target, {
            has(t, prop) {
                return prop === 'b';
            }
        });
        'a' in proxy;
        'b' in proxy;
    }
    testProxyHasTrap();
}

/*******************************************************
 * ArrayBuffer.prototype.resize (growable)
 ******************************************************/
{
    function testArrayBufferResize() {
        if (typeof ArrayBuffer.prototype.resize === 'function') {
            const ab = new ArrayBuffer(16, { maxByteLength: 128 });
            ab.resize(32);
            ab.byteLength;
        }
    }
    testArrayBufferResize();
}

/*******************************************************
 * Atomics.waitAsync non-blocking wait
 ******************************************************/
{
    async function testAtomicsWaitAsync() {
        const sab = new SharedArrayBuffer(4);
        const ia = new Int32Array(sab);
        const waiter = Atomics.waitAsync(ia, 0, 0);
        Atomics.notify(ia, 0, 1);
        await waiter.value;
    }
    testAtomicsWaitAsync();
}

/*******************************************************
 * JSON.stringify with BigInt throw
 ******************************************************/
{
    function testJSONBigIntThrow() {
        try {
            JSON.stringify({ val: 123n });
        } catch (e) {
            // expect TypeError
        }
    }
    testJSONBigIntThrow();
}

test_end();