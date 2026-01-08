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

var A = {a:1, b:2};
print('a' in A);
print('b' in A);
print('c' in A);

let key1 = {
    toString: function() {
        return "3";
    } 
}

let key2 = {
    valueOf: function() {
        return "4";
    },
    toString: null
}

let obj = {1: 1, "abc": 2, [key1]: 3, [key2]: 4};

let obj2 = {"5": 5};
obj.__proto__ = obj2;

print(1 in obj)
print("abc" in obj)
print("3" in obj)
print(5 in obj)
print(6 in obj)
print([key1] in obj)
print([key2] in obj)

let obj3 = {"abcdef": "abcdef"};
print("ab" + "cd" + "ef" in obj3);

var str = new String("hello world");
print('1' in str);
print('11' in str);

// =====================================================================
// Section 1: Linear search path (1~9 properties)
// =====================================================================
{
    let obj1 = { p0: 0 };
    print('p0' in obj1);        // true
    print('missing' in obj1);   // false
}

{
    let obj2 = { p0: 0, p1: 1 };
    print('p0' in obj2);        // true
    print('p1' in obj2);        // true
    print('p2' in obj2);        // false
}

{
    let obj3 = { p0: 0, p1: 1, p2: 2 };
    print('p0' in obj3);        // true
    print('p2' in obj3);        // true
    print('p3' in obj3);        // false
}

{
    let obj5 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4 };
    print('p0' in obj5);        // true
    print('p2' in obj5);        // true
    print('p4' in obj5);        // true
    print('p5' in obj5);        // false
}

{
    let obj7 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4, p5: 5, p6: 6 };
    print('p0' in obj7);        // true
    print('p3' in obj7);        // true
    print('p6' in obj7);        // true
    print('p7' in obj7);        // false
}

{
    // 9 properties - boundary of linear search
    let obj9 = { p0: 0, p1: 1, p2: 2, p3: 3, p4: 4, p5: 5, p6: 6, p7: 7, p8: 8 };
    print('p0' in obj9);        // true
    print('p4' in obj9);        // true
    print('p8' in obj9);        // true
    print('p9' in obj9);        // false
}

// =====================================================================
// Section 2: Binary search path (>9 properties)
// =====================================================================
{
    let obj10 = { a0: 0, a1: 1, a2: 2, a3: 3, a4: 4, a5: 5, a6: 6, a7: 7, a8: 8, a9: 9 };
    print('a0' in obj10);       // true
    print('a5' in obj10);       // true
    print('a9' in obj10);       // true
    print('a10' in obj10);      // false
}

{
    let obj15 = {};
    for (let i = 0; i < 15; i++) {
        obj15['prop' + i] = i;
    }
    print('prop0' in obj15);    // true
    print('prop7' in obj15);    // true
    print('prop14' in obj15);   // true
    print('prop15' in obj15);   // false
}

{
    let obj25 = {};
    for (let i = 0; i < 25; i++) {
        obj25['key' + i] = i;
    }
    print('key0' in obj25);     // true
    print('key12' in obj25);    // true
    print('key24' in obj25);    // true
    print('key25' in obj25);    // false
}

// =====================================================================
// Section 3: Cache hit - repeated lookups
// =====================================================================
{
    let cached = { alpha: 1, beta: 2, gamma: 3, delta: 4, epsilon: 5 };
    // First lookup
    print('alpha' in cached);   // true
    print('missing' in cached); // false
    // Second lookup - hits cache
    print('alpha' in cached);   // true
    print('missing' in cached); // false
    // Third
    print('beta' in cached);    // true
    print('beta' in cached);    // true
}

{
    let bigCached = {};
    for (let i = 0; i < 20; i++) {
        bigCached['c' + i] = i;
    }
    // First pass
    print('c0' in bigCached);   // true
    print('c10' in bigCached);  // true
    print('c19' in bigCached);  // true
    print('c20' in bigCached);  // false
    // Second pass - cache hit
    print('c0' in bigCached);   // true
    print('c10' in bigCached);  // true
    print('c19' in bigCached);  // true
    print('c20' in bigCached);  // false
}

// =====================================================================
// Section 4: NOT_FOUND caching for non-shared
// =====================================================================
{
    let normalObj = { a: 1, b: 2, c: 3 };
    print('z' in normalObj);    // false
    print('z' in normalObj);    // false - cache hit
    print('y' in normalObj);    // false
    print('y' in normalObj);    // false - cache hit
    print('a' in normalObj);    // true
}

{
    let largeObj = {};
    for (let i = 0; i < 25; i++) {
        largeObj['exist' + i] = i;
    }
    print('noSuchKey' in largeObj);       // false
    print('noSuchKey' in largeObj);       // false - cache hit
    print('anotherMissing' in largeObj);  // false
    print('exist0' in largeObj);          // true
    print('exist12' in largeObj);         // true
    print('exist24' in largeObj);         // true
}

// =====================================================================
// Section 5: Same HClass (same shape) objects
// =====================================================================
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
    print('aa' in o1);          // true
    print('aa' in o2);          // true
    print('aa' in o3);          // true
    print('bb' in o1);          // true
    print('cc' in o2);          // true
    print('dd' in o1);          // false
    print('dd' in o2);          // false
}

{
    function makeLargeObj(prefix) {
        let o = {};
        for (let i = 0; i < 12; i++) {
            o['prop' + i] = prefix + i;
        }
        return o;
    }
    let la = makeLargeObj('a');
    let lb = makeLargeObj('b');
    print('prop0' in la);       // true
    print('prop11' in la);      // true
    print('prop12' in la);      // false
    print('prop0' in lb);       // true
    print('prop11' in lb);      // true
    print('prop12' in lb);      // false
}

// =====================================================================
// Section 6: Property addition after cache population
// =====================================================================
{
    let evolving = { m0: 0, m1: 1, m2: 2 };
    print('m0' in evolving);    // true
    print('m3' in evolving);    // false
    evolving.m3 = 3;
    print('m3' in evolving);    // true
    print('m0' in evolving);    // true
    print('m4' in evolving);    // false
    evolving.m4 = 4;
    evolving.m5 = 5;
    print('m4' in evolving);    // true
    print('m5' in evolving);    // true
    print('m6' in evolving);    // false
}

{
    // Grow past linear search threshold
    let growing = {};
    for (let i = 0; i < 8; i++) {
        growing['g' + i] = i;
    }
    print('g0' in growing);     // true
    print('g7' in growing);     // true
    print('g8' in growing);     // false
    growing.g8 = 8;
    growing.g9 = 9;
    growing.g10 = 10;
    print('g0' in growing);     // true
    print('g8' in growing);     // true
    print('g10' in growing);    // true
    print('g11' in growing);    // false
}

// =====================================================================
// Section 7: Property deletion
// =====================================================================
{
    let del = { d0: 0, d1: 1, d2: 2, d3: 3, d4: 4 };
    print('d2' in del);         // true
    delete del.d2;
    print('d2' in del);         // false
    print('d0' in del);         // true
    print('d4' in del);         // true
}

{
    let delLarge = {};
    for (let i = 0; i < 15; i++) {
        delLarge['dl' + i] = i;
    }
    print('dl7' in delLarge);   // true
    delete delLarge.dl7;
    print('dl7' in delLarge);   // false
    print('dl0' in delLarge);   // true
    print('dl14' in delLarge);  // true
}

// =====================================================================
// Section 8: Prototype chain lookup
// =====================================================================
{
    let proto = { inherited: 'yes', shared: true };
    let child = Object.create(proto);
    child.own = 42;
    print('own' in child);          // true
    print('inherited' in child);    // true
    print('shared' in child);       // true
    print('missing' in child);      // false
}

{
    let p1 = { level1: 1 };
    let p2 = Object.create(p1);
    p2.level2 = 2;
    let p3 = Object.create(p2);
    p3.level3 = 3;
    let p4 = Object.create(p3);
    p4.level4 = 4;
    print('level4' in p4);     // true
    print('level3' in p4);     // true
    print('level2' in p4);     // true
    print('level1' in p4);     // true
    print('level0' in p4);     // false
}

{
    let bigProto = {};
    for (let i = 0; i < 15; i++) {
        bigProto['bp' + i] = i;
    }
    let heir = Object.create(bigProto);
    heir.own1 = 'a';
    print('bp0' in heir);      // true
    print('bp7' in heir);      // true
    print('bp14' in heir);     // true
    print('own1' in heir);     // true
    print('bp15' in heir);     // false
}

// =====================================================================
// Section 9: Symbol property lookups
// =====================================================================
{
    let s1 = Symbol('sym1');
    let s2 = Symbol('sym2');
    let symObj = { [s1]: 1, [s2]: 2, normal: 3 };
    print(s1 in symObj);        // true
    print(s2 in symObj);        // true
    print('normal' in symObj);  // true
    print('sym1' in symObj);    // false
}

// =====================================================================
// Section 10: Array index properties
// =====================================================================
{
    let arr = [10, 20, 30, 40, 50];
    print(0 in arr);            // true
    print(4 in arr);            // true
    print(5 in arr);            // false
    print('length' in arr);     // true
    print('0' in arr);          // true
}

{
    let sparse = [];
    sparse[0] = 'a';
    sparse[100] = 'b';
    sparse[9999] = 'c';
    print(0 in sparse);        // true
    print(1 in sparse);        // false
    print(50 in sparse);       // false
    print(100 in sparse);      // true
    print(9999 in sparse);     // true
    print(10000 in sparse);    // false
}

// =====================================================================
// Section 11: TypedArray
// =====================================================================
{
    let i32 = new Int32Array(new ArrayBuffer(16));
    print(0 in i32);           // true
    print(3 in i32);           // true
    print(4 in i32);           // false
    print('length' in i32);    // true
}

{
    let u8 = new Uint8Array(10);
    print(0 in u8);            // true
    print(9 in u8);            // true
    print(10 in u8);           // false
}

// =====================================================================
// Section 12: Built-in object properties
// =====================================================================
{
    print('length' in []);              // true
    print('prototype' in Function);     // true
    print('constructor' in {});         // true
    print('toString' in {});            // true
    print('hasOwnProperty' in {});      // true
}

{
    let map = new Map();
    print('size' in map);      // true
    print('get' in map);       // true
    print('set' in map);       // true
    print('nonExistent' in map); // false
}

{
    let set = new Set();
    print('size' in set);      // true
    print('add' in set);       // true
    print('has' in set);       // true
    print('nonExistent' in set); // false
}

// =====================================================================
// Section 13: Proxy with has trap
// =====================================================================
{
    let target = { a: 1, b: 2 };
    let proxy = new Proxy(target, {
        has(t, p) {
            return Reflect.has(t, p);
        }
    });
    print('a' in proxy);       // true
    print('b' in proxy);       // true
    print('c' in proxy);       // false
}

{
    let alwaysTrue = new Proxy({}, {
        has() { return true; }
    });
    print('anything' in alwaysTrue);    // true
    print('whatever' in alwaysTrue);    // true
}

// =====================================================================
// Section 14: Stress cache with loops
// =====================================================================
{
    let stressObj = {};
    for (let i = 0; i < 20; i++) {
        stressObj['s' + i] = i;
    }
    let allFound = true;
    let allMissing = true;
    for (let round = 0; round < 10; round++) {
        for (let i = 0; i < 20; i++) {
            if (!(('s' + i) in stressObj)) allFound = false;
        }
        for (let i = 20; i < 25; i++) {
            if (('s' + i) in stressObj) allMissing = false;
        }
    }
    print(allFound);            // true
    print(allMissing);          // true
}

{
    let altObj = { a: 1, b: 2, c: 3, d: 4, e: 5 };
    let ok = true;
    for (let round = 0; round < 20; round++) {
        if (!('a' in altObj)) ok = false;
        if ('z' in altObj) ok = false;
        if (!('b' in altObj)) ok = false;
        if ('y' in altObj) ok = false;
        if (!('c' in altObj)) ok = false;
        if ('x' in altObj) ok = false;
    }
    print(ok);                  // true
}

// =====================================================================
// Section 15: Object.create with null prototype
// =====================================================================
{
    let desc = Object.create(null, {
        foo: { value: 1, enumerable: true, configurable: true, writable: true },
        bar: { value: 2, enumerable: true, configurable: true, writable: true },
    });
    print('foo' in desc);          // true
    print('bar' in desc);          // true
    print('toString' in desc);     // false
    print('missing' in desc);      // false
}

{
    let nullProto = Object.create(null);
    for (let i = 0; i < 15; i++) {
        nullProto['np' + i] = i;
    }
    print('np0' in nullProto);         // true
    print('np7' in nullProto);         // true
    print('np14' in nullProto);        // true
    print('toString' in nullProto);    // false
    print('np15' in nullProto);        // false
}

// =====================================================================
// Section 16: Getter/setter and defineProperty
// =====================================================================
{
    let getsetObj = {
        get gp() { return 42; },
        set sp(_v) {},
        normal: 1
    };
    print('gp' in getsetObj);      // true
    print('sp' in getsetObj);      // true
    print('normal' in getsetObj);  // true
    print('other' in getsetObj);   // false
}

{
    let defined = {};
    Object.defineProperty(defined, 'hidden', { value: 42, enumerable: false, configurable: true });
    Object.defineProperty(defined, 'visible', { value: 100, enumerable: true, configurable: true });
    print('hidden' in defined);    // true
    print('visible' in defined);   // true
    print('other' in defined);     // false
}

// =====================================================================
// Section 17: Class instances and inheritance
// =====================================================================
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
    print('instanceProp' in inst);  // true
    print('anotherProp' in inst);   // true
    print('methodA' in inst);       // true
    print('methodB' in inst);       // true
    print('nonExistent' in inst);   // false
}

{
    class Base {
        constructor() { this.baseProp = 1; }
        baseMethod() {}
    }
    class Derived extends Base {
        constructor() { super(); this.derivedProp = 2; }
        derivedMethod() {}
    }
    let d = new Derived();
    print('baseProp' in d);         // true
    print('derivedProp' in d);      // true
    print('baseMethod' in d);       // true
    print('derivedMethod' in d);    // true
    print('nonExistent' in d);      // false
}

{
    class BigClass {
        constructor() {
            for (let i = 0; i < 15; i++) {
                this['field' + i] = i;
            }
        }
    }
    let big = new BigClass();
    print('field0' in big);         // true
    print('field7' in big);         // true
    print('field14' in big);        // true
    print('field15' in big);        // false
}

// =====================================================================
// Section 18: Well-known symbols
// =====================================================================
{
    let iterObj = { [Symbol.iterator]: function*() { yield 1; } };
    print(Symbol.iterator in iterObj);      // true
    print(Symbol.toPrimitive in iterObj);   // false
}

// =====================================================================
// Section 19: TypeError for non-objects
// =====================================================================
{
    let nonObjects = [42, 3.14, true, false, 'string', Symbol('s')];
    let allThrew = true;
    for (let val of nonObjects) {
        try {
            'prop' in val;
            allThrew = false;
        } catch (e) {
            if (!(e instanceof TypeError)) allThrew = false;
        }
    }
    print(allThrew);                // true
}

// =====================================================================
// Section 20: Frozen and sealed objects
// =====================================================================
{
    let frozen = Object.freeze({ f1: 1, f2: 2, f3: 3 });
    print('f1' in frozen);     // true
    print('f2' in frozen);     // true
    print('f3' in frozen);     // true
    print('f4' in frozen);     // false
    // Repeated on frozen
    print('f1' in frozen);     // true
    print('f4' in frozen);     // false
}

{
    let sealed = Object.seal({ s1: 1, s2: 2 });
    print('s1' in sealed);     // true
    print('s2' in sealed);     // true
    print('s3' in sealed);     // false
}

{
    let frozenLarge = {};
    for (let i = 0; i < 20; i++) {
        frozenLarge['fz' + i] = i;
    }
    Object.freeze(frozenLarge);
    print('fz0' in frozenLarge);    // true
    print('fz10' in frozenLarge);   // true
    print('fz19' in frozenLarge);   // true
    print('fz20' in frozenLarge);   // false
}

// =====================================================================
// Section 21: Mixed numeric and string keys
// =====================================================================
{
    let mixed = {};
    mixed[0] = 'zero';
    mixed[1] = 'one';
    mixed['2'] = 'two';
    mixed.three = 3;
    mixed[''] = 'empty';
    print(0 in mixed);         // true
    print(1 in mixed);         // true
    print('2' in mixed);       // true
    print(2 in mixed);         // true
    print('three' in mixed);   // true
    print('' in mixed);        // true
    print(3 in mixed);         // false
    print('four' in mixed);    // false
}

// =====================================================================
// Section 22: Many distinct shapes stress cache
// =====================================================================
{
    let allCorrect = true;
    for (let i = 0; i < 20; i++) {
        let o = {};
        for (let j = 0; j <= i; j++) {
            o['shape' + i + '_key' + j] = j;
        }
        for (let j = 0; j <= i; j++) {
            if (!(('shape' + i + '_key' + j) in o)) allCorrect = false;
        }
        if (('shape' + i + '_key' + (i + 1)) in o) allCorrect = false;
    }
    print(allCorrect);         // true
}

// =====================================================================
// Section 23: Arguments object
// =====================================================================
{
    function checkArgs() {
        print(0 in arguments);         // true
        print(1 in arguments);         // true
        print(2 in arguments);         // true
        print(3 in arguments);         // false
        print('length' in arguments);  // true
    }
    checkArgs('a', 'b', 'c');
}

// =====================================================================
// Section 24: String wrapper object
// =====================================================================
{
    let strObj = new String('hello');
    print(0 in strObj);        // true
    print(4 in strObj);        // true
    print(5 in strObj);        // false
    print('length' in strObj); // true
}

// =====================================================================
// Section 25: Date, RegExp, Error objects
// =====================================================================
{
    let d = new Date();
    print('getTime' in d);         // true
    print('toISOString' in d);     // true
    print('nonExistent' in d);     // false
}

{
    let r = /test/g;
    print('source' in r);     // true
    print('flags' in r);      // true
    print('test' in r);       // true
    print('nonExistent' in r); // false
}

{
    let e = new Error('test');
    print('message' in e);    // true
    print('stack' in e);      // true
    print('nonExistent' in e); // false
}

// =====================================================================
// Section 26: Function properties
// =====================================================================
{
    function testFunc(a, b, c) {}
    print('length' in testFunc);       // true
    print('name' in testFunc);         // true
    print('prototype' in testFunc);    // true
    print('call' in testFunc);         // true
    print('nonExistent' in testFunc);  // false
}

// =====================================================================
// Section 27: Rapid shape transitions
// =====================================================================
{
    let rapid = {};
    let ok = true;
    for (let i = 0; i < 30; i++) {
        let key = 'r' + i;
        if (key in rapid) ok = false;
        rapid[key] = i;
        if (!(key in rapid)) ok = false;
    }
    // Verify all exist
    for (let i = 0; i < 30; i++) {
        if (!(('r' + i) in rapid)) ok = false;
    }
    if ('r30' in rapid) ok = false;
    print(ok);                 // true
}

// =====================================================================
// Section 28: WeakMap/WeakSet/DataView
// =====================================================================
{
    let wm = new WeakMap();
    print('get' in wm);       // true
    print('set' in wm);       // true
    print('has' in wm);       // true
    print('size' in wm);      // false
}

{
    let ws = new WeakSet();
    print('add' in ws);       // true
    print('has' in ws);       // true
    print('size' in ws);      // false
}

{
    let dv = new DataView(new ArrayBuffer(8));
    print('getInt8' in dv);       // true
    print('setInt8' in dv);       // true
    print('byteLength' in dv);    // true
    print('nonExistent' in dv);   // false
}