/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name: isin
 * @tc.desc: test isin. Test whether the return value of IsIn is exception while input para is not a ECMA obj.
 * @tc.type: FUNC
 */
{
    try {
        1 in 0;
        let tmpArr = [0];
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}

{
    const v5 = [-3];
    const a8 = new Uint8Array(2);
    v5.__proto__ = a8;
    assert_equal(-3 in v5, false);
}

{
    const obj = { a: 1 };
    assert_equal('a' in obj, true);
    assert_equal('b' in obj, false);
}

{
    const arr = ['x'];
    assert_equal(0 in arr, true);
    assert_equal(1 in arr, false);
    arr[100] = 'y';
    assert_equal(100 in arr, true);
    assert_equal('length' in arr, true);
}

{
    const base = { foo: 1 };
    const derived = Object.create(base);
    assert_equal('foo' in derived, true);
    derived.foo = 2;
    assert_equal('foo' in derived, true);
}

{
    try {
        'x' in undefined;
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }

    try {
        'y' in null;
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}

{
    const sym = Symbol('s');
    const obj = { [sym]: 42 };
    assert_equal(sym in obj, true);
    assert_equal('s' in obj, false);
}

{
    const target = {};
    const proxy = new Proxy(target, {
        has(target, prop) {
            return prop === 'custom';
        }
    });
    assert_equal('custom' in proxy, true);
    assert_equal('another' in proxy, false);
}

{
    try {
        'x' in 42;
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }

    try {
        'length' in 'abc';
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}

// --- Section 1: Linear search path (1~9 properties) ---
// Test objects with 1 to 9 properties to exercise the linear search unrolled loop
{
    // 1 property
    let obj1 = { p0: 0 };
    assert_equal('p0' in obj1, true);
    assert_equal('missing' in obj1, false);
}

{
    // 2 properties
    let obj2 = { p0: 0, p1: 1 };
    assert_equal('p0' in obj2, true);
    assert_equal('p1' in obj2, true);
    assert_equal('p2' in obj2, false);
    assert_equal('missing' in obj2, false);
}

{
    // 3 properties
    let obj3 = { p0: 0, p1: 1, p2: 2 };
    assert_equal('p0' in obj3, true);
    assert_equal('p1' in obj3, true);
    assert_equal('p2' in obj3, true);
    assert_equal('p3' in obj3, false);
    assert_equal('xyz' in obj3, false);
}

{
    // 4 properties
    let obj4 = { p0: 0, p1: 1, p2: 2, p3: 3 };
    assert_equal('p0' in obj4, true);
    assert_equal('p1' in obj4, true);
    assert_equal('p2' in obj4, true);
    assert_equal('p3' in obj4, true);
    assert_equal('p4' in obj4, false);
}

{
    // 5 properties
    let obj5 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4 };
    assert_equal('p0' in obj5, true);
    assert_equal('p2' in obj5, true);
    assert_equal('p4' in obj5, true);
    assert_equal('p5' in obj5, false);
    assert_equal('notExist' in obj5, false);
}

{
    // 6 properties
    let obj6 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4, p5: 5 };
    assert_equal('p0' in obj6, true);
    assert_equal('p3' in obj6, true);
    assert_equal('p5' in obj6, true);
    assert_equal('p6' in obj6, false);
}

{
    // 7 properties
    let obj7 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4, p5: 5, p6: 6 };
    assert_equal('p0' in obj7, true);
    assert_equal('p6' in obj7, true);
    assert_equal('p7' in obj7, false);
}

{
    // 8 properties
    let obj8 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4, p5: 5, p6: 6, p7: 7 };
    assert_equal('p0' in obj8, true);
    assert_equal('p4' in obj8, true);
    assert_equal('p7' in obj8, true);
    assert_equal('p8' in obj8, false);
}

{
    // 9 properties (boundary of linear search)
    let obj9 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4, p5: 5, p6: 6, p7: 7, p8: 8 };
    assert_equal('p0' in obj9, true);
    assert_equal('p4' in obj9, true);
    assert_equal('p8' in obj9, true);
    assert_equal('p9' in obj9, false);
    assert_equal('notHere' in obj9, false);
}

// --- Section 2: Binary search path (>9 properties) ---
{
    // 10 properties (just above linear threshold)
    let obj10 = { a0: 0, a1: 1, a2: 2, a3: 3, a4: 4, a5: 5, a6: 6, a7: 7, a8: 8, a9: 9 };
    assert_equal('a0' in obj10, true);
    assert_equal('a5' in obj10, true);
    assert_equal('a9' in obj10, true);
    assert_equal('a10' in obj10, false);
    assert_equal('missing' in obj10, false);
}

{
    // 15 properties
    let obj15 = {};
    for (let i = 0; i < 15; i++) {
        obj15['prop' + i] = i;
    }
    for (let i = 0; i < 15; i++) {
        assert_equal(('prop' + i) in obj15, true);
    }
    assert_equal('prop15' in obj15, false);
    assert_equal('prop16' in obj15, false);
    assert_equal('nonExistent' in obj15, false);
}

{
    // 20 properties
    let obj20 = {};
    for (let i = 0; i < 20; i++) {
        obj20['key' + i] = i * 10;
    }
    for (let i = 0; i < 20; i++) {
        assert_equal(('key' + i) in obj20, true);
    }
    assert_equal('key20' in obj20, false);
    assert_equal('key99' in obj20, false);
}

{
    // 30 properties - larger object
    let obj30 = {};
    for (let i = 0; i < 30; i++) {
        obj30['field_' + i] = 'value' + i;
    }
    // Check first, middle, last
    assert_equal('field_0' in obj30, true);
    assert_equal('field_14' in obj30, true);
    assert_equal('field_29' in obj30, true);
    assert_equal('field_30' in obj30, false);
    assert_equal('field_100' in obj30, false);
}

{
    // 50 properties - stress test binary search
    let obj50 = {};
    for (let i = 0; i < 50; i++) {
        obj50['x' + i] = i;
    }
    for (let i = 0; i < 50; i++) {
        assert_equal(('x' + i) in obj50, true);
    }
    for (let i = 50; i < 60; i++) {
        assert_equal(('x' + i) in obj50, false);
    }
}

// --- Section 3: Cache hit path - repeated lookups on the same object ---
// After the first lookup the result should be cached, second lookup hits cache
{
    let cached = { alpha: 1, beta: 2, gamma: 3, delta: 4, epsilon: 5 };
    // First lookup - cache miss, populates cache
    assert_equal('alpha' in cached, true);
    assert_equal('beta' in cached, true);
    assert_equal('missing1' in cached, false);
    // Second lookup - should hit cache
    assert_equal('alpha' in cached, true);
    assert_equal('beta' in cached, true);
    assert_equal('missing1' in cached, false);
    // Third lookup - still cached
    assert_equal('gamma' in cached, true);
    assert_equal('gamma' in cached, true);
    assert_equal('delta' in cached, true);
    assert_equal('delta' in cached, true);
}

{
    // Cache with >9 props (binary search path caching)
    let bigCached = {};
    for (let i = 0; i < 20; i++) {
        bigCached['c' + i] = i;
    }
    // First pass - populates cache
    for (let i = 0; i < 20; i++) {
        assert_equal(('c' + i) in bigCached, true);
    }
    assert_equal('c20' in bigCached, false);
    assert_equal('cNone' in bigCached, false);
    // Second pass - hits cache
    for (let i = 0; i < 20; i++) {
        assert_equal(('c' + i) in bigCached, true);
    }
    assert_equal('c20' in bigCached, false);
    assert_equal('cNone' in bigCached, false);
}

// --- Section 4: NOT_FOUND caching for non-shared objects ---
// Repeated NOT_FOUND lookups should be cached for normal (non-shared) objects
{
    let normalObj = { a: 1, b: 2, c: 3 };
    // First NOT_FOUND - cache miss
    assert_equal('z' in normalObj, false);
    // Second NOT_FOUND - should hit cache
    assert_equal('z' in normalObj, false);
    // Different missing keys
    assert_equal('y' in normalObj, false);
    assert_equal('y' in normalObj, false);
    assert_equal('x' in normalObj, false);
    assert_equal('x' in normalObj, false);
}

{
    // NOT_FOUND caching with large object (binary search path)
    let largeObj = {};
    for (let i = 0; i < 25; i++) {
        largeObj['exist' + i] = i;
    }
    // NOT_FOUND lookups
    assert_equal('noSuchKey' in largeObj, false);
    assert_equal('noSuchKey' in largeObj, false);
    assert_equal('anotherMissing' in largeObj, false);
    assert_equal('anotherMissing' in largeObj, false);
    // Existing keys still work after NOT_FOUND
    assert_equal('exist0' in largeObj, true);
    assert_equal('exist12' in largeObj, true);
    assert_equal('exist24' in largeObj, true);
}

// --- Section 5: Multiple objects sharing same HClass (same shape) ---
// Properties cache is keyed by HClass, so objects with same shape share cache
{
    function makeObj(val) {
        let o = {};
        o.aa = val;
        o.bb = val + 1;
        o.cc = val + 2;
        return o;
    }
    let o1 = makeObj(10);
    let o2 = makeObj(20);
    let o3 = makeObj(30);
    // All share the same HClass
    assert_equal('aa' in o1, true);
    assert_equal('aa' in o2, true);
    assert_equal('aa' in o3, true);
    assert_equal('bb' in o1, true);
    assert_equal('cc' in o2, true);
    assert_equal('dd' in o1, false);
    assert_equal('dd' in o2, false);
    assert_equal('dd' in o3, false);
}

{
    // Same shape with >9 properties
    function makeLargeObj(prefix) {
        let o = {};
        for (let i = 0; i < 12; i++) {
            o['prop' + i] = prefix + i;
        }
        return o;
    }
    let la = makeLargeObj('a');
    let lb = makeLargeObj('b');
    for (let i = 0; i < 12; i++) {
        assert_equal(('prop' + i) in la, true);
        assert_equal(('prop' + i) in lb, true);
    }
    assert_equal('prop12' in la, false);
    assert_equal('prop12' in lb, false);
}

// --- Section 6: Property addition after cache population (cache invalidation) ---
{
    let evolving = { m0: 0, m1: 1, m2: 2 };
    // Populate cache
    assert_equal('m0' in evolving, true);
    assert_equal('m3' in evolving, false);
    // Add new property - HClass transition, old cache entry stale
    evolving.m3 = 3;
    assert_equal('m3' in evolving, true);
    assert_equal('m0' in evolving, true);
    assert_equal('m4' in evolving, false);
    // Add more
    evolving.m4 = 4;
    evolving.m5 = 5;
    assert_equal('m4' in evolving, true);
    assert_equal('m5' in evolving, true);
    assert_equal('m6' in evolving, false);
}

{
    // Evolving past linear search threshold
    let growing = {};
    for (let i = 0; i < 8; i++) {
        growing['g' + i] = i;
    }
    // In linear search range
    assert_equal('g0' in growing, true);
    assert_equal('g7' in growing, true);
    assert_equal('g8' in growing, false);
    // Grow past threshold
    growing.g8 = 8;
    growing.g9 = 9;
    growing.g10 = 10;
    // Now in binary search range
    assert_equal('g0' in growing, true);
    assert_equal('g8' in growing, true);
    assert_equal('g10' in growing, true);
    assert_equal('g11' in growing, false);
}

// --- Section 7: Property deletion and lookup ---
{
    let del = { d0: 0, d1: 1, d2: 2, d3: 3, d4: 4 };
    assert_equal('d2' in del, true);
    delete del.d2;
    assert_equal('d2' in del, false);
    assert_equal('d0' in del, true);
    assert_equal('d4' in del, true);
}

{
    let delLarge = {};
    for (let i = 0; i < 15; i++) {
        delLarge['dl' + i] = i;
    }
    assert_equal('dl7' in delLarge, true);
    assert_equal('dl14' in delLarge, true);
    delete delLarge.dl7;
    assert_equal('dl7' in delLarge, false);
    assert_equal('dl0' in delLarge, true);
    assert_equal('dl14' in delLarge, true);
}

// --- Section 8: Prototype chain lookup ---
{
    let proto = { inherited: 'yes', shared: true };
    let child = Object.create(proto);
    child.own = 42;
    assert_equal('own' in child, true);
    assert_equal('inherited' in child, true);
    assert_equal('shared' in child, true);
    assert_equal('missing' in child, false);
    // Shadow a prototype property
    child.inherited = 'overridden';
    assert_equal('inherited' in child, true);
}

{
    // Deep prototype chain
    let p1 = { level1: 1 };
    let p2 = Object.create(p1);
    p2.level2 = 2;
    let p3 = Object.create(p2);
    p3.level3 = 3;
    let p4 = Object.create(p3);
    p4.level4 = 4;

    assert_equal('level4' in p4, true);
    assert_equal('level3' in p4, true);
    assert_equal('level2' in p4, true);
    assert_equal('level1' in p4, true);
    assert_equal('level0' in p4, false);
}

{
    // Prototype with many properties
    let bigProto = {};
    for (let i = 0; i < 15; i++) {
        bigProto['bp' + i] = i;
    }
    let heir = Object.create(bigProto);
    heir.own1 = 'a';
    heir.own2 = 'b';
    for (let i = 0; i < 15; i++) {
        assert_equal(('bp' + i) in heir, true);
    }
    assert_equal('own1' in heir, true);
    assert_equal('own2' in heir, true);
    assert_equal('bp15' in heir, false);
}

// --- Section 9: Symbol property lookups ---
{
    let s1 = Symbol('sym1');
    let s2 = Symbol('sym2');
    let s3 = Symbol('sym3');
    let symObj = { [s1]: 1, [s2]: 2, [s3]: 3, normal: 4 };
    assert_equal(s1 in symObj, true);
    assert_equal(s2 in symObj, true);
    assert_equal(s3 in symObj, true);
    assert_equal('normal' in symObj, true);
    assert_equal(Symbol('other') in symObj, false);
    assert_equal('sym1' in symObj, false);
}

{
    // Symbol properties with many string properties
    let mixSym = Symbol('mix');
    let mixObj = {};
    for (let i = 0; i < 12; i++) {
        mixObj['str' + i] = i;
    }
    mixObj[mixSym] = 'symbolVal';
    assert_equal(mixSym in mixObj, true);
    for (let i = 0; i < 12; i++) {
        assert_equal(('str' + i) in mixObj, true);
    }
    assert_equal('str12' in mixObj, false);
}

// --- Section 10: Array index properties ---
{
    let arr = [10, 20, 30, 40, 50];
    assert_equal(0 in arr, true);
    assert_equal(4 in arr, true);
    assert_equal(5 in arr, false);
    assert_equal('length' in arr, true);
    assert_equal('0' in arr, true);
    assert_equal('4' in arr, true);
    assert_equal('5' in arr, false);
}

{
    // Sparse array
    let sparse = [];
    sparse[0] = 'a';
    sparse[100] = 'b';
    sparse[9999] = 'c';
    assert_equal(0 in sparse, true);
    assert_equal(1 in sparse, false);
    assert_equal(50 in sparse, false);
    assert_equal(100 in sparse, true);
    assert_equal(9999 in sparse, true);
    assert_equal(10000 in sparse, false);
}

// --- Section 11: TypedArray and special objects ---
{
    let buf = new ArrayBuffer(16);
    let i32 = new Int32Array(buf);
    assert_equal(0 in i32, true);
    assert_equal(3 in i32, true);
    assert_equal(4 in i32, false);
    assert_equal('length' in i32, true);
    assert_equal('byteLength' in i32, true);
}

{
    let f32 = new Float64Array(5);
    assert_equal(0 in f32, true);
    assert_equal(4 in f32, true);
    assert_equal(5 in f32, false);
}

{
    let u8 = new Uint8Array(10);
    for (let i = 0; i < 10; i++) {
        assert_equal(i in u8, true);
    }
    assert_equal(10 in u8, false);
    assert_equal(11 in u8, false);
}

// --- Section 12: Built-in object properties ---
{
    assert_equal('length' in [], true);
    try {
        'length' in '';  // string primitive is not object
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
    assert_equal('prototype' in Function, true);
    assert_equal('length' in Function, true);
    assert_equal('constructor' in {}, true);
    assert_equal('toString' in {}, true);
    assert_equal('hasOwnProperty' in {}, true);
    assert_equal('valueOf' in {}, true);
}

{
    let map = new Map();
    assert_equal('size' in map, true);
    assert_equal('get' in map, true);
    assert_equal('set' in map, true);
    assert_equal('has' in map, true);
    assert_equal('delete' in map, true);
    assert_equal('nonExistent' in map, false);
}

{
    let set = new Set();
    assert_equal('size' in set, true);
    assert_equal('add' in set, true);
    assert_equal('has' in set, true);
    assert_equal('delete' in set, true);
    assert_equal('nonExistent' in set, false);
}

// --- Section 13: Proxy with has trap ---
{
    let target13 = { a: 1, b: 2, c: 3 };
    let hasCalls = 0;
    let proxy13 = new Proxy(target13, {
        has(t, p) {
            hasCalls++;
            return Reflect.has(t, p);
        }
    });
    assert_equal('a' in proxy13, true);
    assert_equal('b' in proxy13, true);
    assert_equal('d' in proxy13, false);
    assert_equal(hasCalls, 3);
}

{
    // Proxy always returns true
    let alwaysTrue = new Proxy({}, {
        has() { return true; }
    });
    assert_equal('anything' in alwaysTrue, true);
    assert_equal('literally_anything' in alwaysTrue, true);
    assert_equal(Symbol() in alwaysTrue, true);
}

{
    // Proxy always returns false (when property is configurable)
    let alwaysFalse = new Proxy({ x: 1 }, {
        has() { return false; }
    });
    assert_equal('x' in alwaysFalse, false);
    assert_equal('y' in alwaysFalse, false);
}

// --- Section 14: Many lookups in a loop to stress cache ---
{
    let stressObj = {};
    for (let i = 0; i < 20; i++) {
        stressObj['s' + i] = i;
    }
    // Repeat lookups many times to thoroughly exercise cache
    for (let round = 0; round < 10; round++) {
        for (let i = 0; i < 20; i++) {
            assert_equal(('s' + i) in stressObj, true);
        }
        for (let i = 20; i < 25; i++) {
            assert_equal(('s' + i) in stressObj, false);
        }
    }
}

{
    // Alternating found/not-found to test cache eviction
    let altObj = { a: 1, b: 2, c: 3, d: 4, e: 5 };
    for (let round = 0; round < 20; round++) {
        assert_equal('a' in altObj, true);
        assert_equal('z' in altObj, false);
        assert_equal('b' in altObj, true);
        assert_equal('y' in altObj, false);
        assert_equal('c' in altObj, true);
        assert_equal('x' in altObj, false);
    }
}

// --- Section 15: Objects created with different patterns ---
{
    // Object.create with property descriptors
    let desc = Object.create(null, {
        foo: { value: 1, enumerable: true, configurable: true, writable: true },
        bar: { value: 2, enumerable: true, configurable: true, writable: true },
        baz: { value: 3, enumerable: true, configurable: true, writable: true }
    });
    assert_equal('foo' in desc, true);
    assert_equal('bar' in desc, true);
    assert_equal('baz' in desc, true);
    assert_equal('toString' in desc, false); // no prototype
    assert_equal('missing' in desc, false);
}

{
    // Object with null prototype and many properties
    let nullProto = Object.create(null);
    for (let i = 0; i < 15; i++) {
        nullProto['np' + i] = i;
    }
    for (let i = 0; i < 15; i++) {
        assert_equal(('np' + i) in nullProto, true);
    }
    assert_equal('toString' in nullProto, false);
    assert_equal('constructor' in nullProto, false);
    assert_equal('np15' in nullProto, false);
}

// --- Section 16: Getter/setter properties ---
{
    let getsetObj = {
        get gp() { return 42; },
        set sp(v) {},
        normal: 1
    };
    assert_equal('gp' in getsetObj, true);
    assert_equal('sp' in getsetObj, true);
    assert_equal('normal' in getsetObj, true);
    assert_equal('other' in getsetObj, false);
}

{
    let defined = {};
    Object.defineProperty(defined, 'hidden', {
        value: 42,
        enumerable: false,
        configurable: true
    });
    Object.defineProperty(defined, 'visible', {
        value: 100,
        enumerable: true,
        configurable: true
    });
    assert_equal('hidden' in defined, true);
    assert_equal('visible' in defined, true);
    assert_equal('other' in defined, false);
}

// --- Section 17: Class instances ---
{
    class MyClass {
        constructor() {
            this.instanceProp = 1;
            this.anotherProp = 2;
        }
        methodA() {}
        methodB() {}
    }
    let inst = new MyClass();
    assert_equal('instanceProp' in inst, true);
    assert_equal('anotherProp' in inst, true);
    assert_equal('methodA' in inst, true);
    assert_equal('methodB' in inst, true);
    assert_equal('constructor' in inst, true);
    assert_equal('nonExistent' in inst, false);
}

{
    // Subclass
    class Base {
        constructor() {
            this.baseProp = 1;
        }
        baseMethod() {}
    }
    class Derived extends Base {
        constructor() {
            super();
            this.derivedProp = 2;
        }
        derivedMethod() {}
    }
    let d = new Derived();
    assert_equal('baseProp' in d, true);
    assert_equal('derivedProp' in d, true);
    assert_equal('baseMethod' in d, true);
    assert_equal('derivedMethod' in d, true);
    assert_equal('nonExistent' in d, false);
}

{
    // Class with many instance properties
    class BigClass {
        constructor() {
            for (let i = 0; i < 15; i++) {
                this['field' + i] = i;
            }
        }
    }
    let big = new BigClass();
    for (let i = 0; i < 15; i++) {
        assert_equal(('field' + i) in big, true);
    }
    assert_equal('field15' in big, false);
}

// --- Section 18: Well-known symbols ---
{
    let iterObj = { [Symbol.iterator]: function*() { yield 1; } };
    assert_equal(Symbol.iterator in iterObj, true);
    assert_equal(Symbol.toPrimitive in iterObj, false);
    assert_equal(Symbol.hasInstance in iterObj, false);
}

{
    let toPrimObj = { [Symbol.toPrimitive]: () => 0 };
    assert_equal(Symbol.toPrimitive in toPrimObj, true);
    assert_equal(Symbol.iterator in toPrimObj, false);
}

// --- Section 19: Error types for non-objects ---
{
    let nonObjects = [42, 3.14, true, false, 'string', Symbol('s'), BigInt(123)];
    for (let val of nonObjects) {
        try {
            'prop' in val;
            assert_unreachable();
        } catch (e) {
            assert_equal(e instanceof TypeError, true);
        }
    }
}

// --- Section 20: Frozen and sealed objects ---
{
    let frozen = Object.freeze({ f1: 1, f2: 2, f3: 3 });
    assert_equal('f1' in frozen, true);
    assert_equal('f2' in frozen, true);
    assert_equal('f3' in frozen, true);
    assert_equal('f4' in frozen, false);
    // Repeated lookups on frozen
    assert_equal('f1' in frozen, true);
    assert_equal('f4' in frozen, false);
}

{
    let sealed = Object.seal({ s1: 1, s2: 2 });
    assert_equal('s1' in sealed, true);
    assert_equal('s2' in sealed, true);
    assert_equal('s3' in sealed, false);
}

{
    // Frozen large object
    let frozenLarge = {};
    for (let i = 0; i < 20; i++) {
        frozenLarge['fz' + i] = i;
    }
    Object.freeze(frozenLarge);
    for (let i = 0; i < 20; i++) {
        assert_equal(('fz' + i) in frozenLarge, true);
    }
    assert_equal('fz20' in frozenLarge, false);
}

// --- Section 21: Computed property names ---
{
    function makeKey(n) { return 'computed_' + n; }
    let compObj = {};
    for (let i = 0; i < 10; i++) {
        compObj[makeKey(i)] = i;
    }
    for (let i = 0; i < 10; i++) {
        assert_equal(makeKey(i) in compObj, true);
    }
    assert_equal(makeKey(10) in compObj, false);
    assert_equal(makeKey(99) in compObj, false);
}

// --- Section 22: Mixed numeric and string keys ---
{
    let mixed = {};
    mixed[0] = 'zero';
    mixed[1] = 'one';
    mixed['2'] = 'two';
    mixed.three = 3;
    mixed[''] = 'empty';
    assert_equal(0 in mixed, true);
    assert_equal(1 in mixed, true);
    assert_equal('2' in mixed, true);
    assert_equal(2 in mixed, true);
    assert_equal('three' in mixed, true);
    assert_equal('' in mixed, true);
    assert_equal(3 in mixed, false);
    assert_equal('four' in mixed, false);
}

// --- Section 23: Multiple distinct HClass shapes stress cache ---
{
    // Create objects with different shapes to stress cache eviction
    let shapes = [];
    for (let i = 0; i < 20; i++) {
        let o = {};
        for (let j = 0; j <= i; j++) {
            o['shape' + i + '_key' + j] = j;
        }
        shapes.push(o);
    }
    // Lookup on all shapes
    for (let i = 0; i < 20; i++) {
        for (let j = 0; j <= i; j++) {
            assert_equal(('shape' + i + '_key' + j) in shapes[i], true);
        }
        assert_equal(('shape' + i + '_key' + (i + 1)) in shapes[i], false);
    }
}

// --- Section 24: Arguments object ---
{
    function checkArgs() {
        assert_equal(0 in arguments, true);
        assert_equal(1 in arguments, true);
        assert_equal(2 in arguments, true);
        assert_equal(3 in arguments, false);
        assert_equal('length' in arguments, true);
        assert_equal('callee' in arguments, true);
    }
    checkArgs('a', 'b', 'c');
}

// --- Section 25: String wrapper object ---
{
    let strObj = new String('hello');
    assert_equal(0 in strObj, true);
    assert_equal(4 in strObj, true);
    assert_equal(5 in strObj, false);
    assert_equal('length' in strObj, true);
}

// --- Section 26: Date, RegExp, Error objects ---
{
    let d = new Date();
    assert_equal('getTime' in d, true);
    assert_equal('toISOString' in d, true);
    assert_equal('nonExistent' in d, false);
}

{
    let r = /test/g;
    assert_equal('source' in r, true);
    assert_equal('flags' in r, true);
    assert_equal('test' in r, true);
    assert_equal('exec' in r, true);
    assert_equal('nonExistent' in r, false);
}

{
    let e = new Error('test');
    assert_equal('message' in e, true);
    assert_equal('stack' in e, true);
    assert_equal('nonExistent' in e, false);
}

// --- Section 27: WeakMap/WeakSet (no 'in', but check properties) ---
{
    let wm = new WeakMap();
    assert_equal('get' in wm, true);
    assert_equal('set' in wm, true);
    assert_equal('has' in wm, true);
    assert_equal('delete' in wm, true);
    assert_equal('size' in wm, false);
}

{
    let ws = new WeakSet();
    assert_equal('add' in ws, true);
    assert_equal('has' in ws, true);
    assert_equal('delete' in ws, true);
    assert_equal('size' in ws, false);
}

// --- Section 28: DataView ---
{
    let dv = new DataView(new ArrayBuffer(8));
    assert_equal('getInt8' in dv, true);
    assert_equal('setInt8' in dv, true);
    assert_equal('getFloat64' in dv, true);
    assert_equal('byteLength' in dv, true);
    assert_equal('nonExistent' in dv, false);
}

// --- Section 29: Function properties ---
{
    function testFunc(a, b, c) {}
    assert_equal('length' in testFunc, true);
    assert_equal('name' in testFunc, true);
    assert_equal('prototype' in testFunc, true);
    assert_equal('call' in testFunc, true);
    assert_equal('apply' in testFunc, true);
    assert_equal('bind' in testFunc, true);
    assert_equal('nonExistent' in testFunc, false);
}

{
    // Arrow function (no prototype)
    let arrow = (a, b) => a + b;
    assert_equal('length' in arrow, true);
    assert_equal('name' in arrow, true);
    assert_equal('nonExistent' in arrow, false);
}

// --- Section 30: Rapid shape transitions with in operator ---
{
    let rapid = {};
    for (let i = 0; i < 30; i++) {
        let key = 'r' + i;
        assert_equal(key in rapid, false);
        rapid[key] = i;
        assert_equal(key in rapid, true);
    }
    // Verify all exist after additions
    for (let i = 0; i < 30; i++) {
        assert_equal(('r' + i) in rapid, true);
    }
    assert_equal('r30' in rapid, false);
}

test_end();