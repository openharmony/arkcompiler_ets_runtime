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
 * @tc.name:icpolymorphic
 * @tc.desc:test polymorphic inline cache behavior
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test 2-type polymorphic IC
function test2TypePolymorphic() {
    function access(o) {
        return o.x;
    }
    const obj1 = { x: 1 };
    const obj2 = { x: 2 };
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(obj1), 1);
        } else {
            assert_equal(access(obj2), 2);
        }
    }
}
test2TypePolymorphic();

// Test 3-type polymorphic IC
function test3TypePolymorphic() {
    function access(o) {
        return o.x;
    }
    const obj1 = { x: 1 };
    const obj2 = { x: 2 };
    const obj3 = { x: 3 };
    for (let i = 0; i < 100; i++) {
        if (i % 3 === 0) {
            assert_equal(access(obj1), 1);
        } else if (i % 3 === 1) {
            assert_equal(access(obj2), 2);
        } else {
            assert_equal(access(obj3), 3);
        }
    }
}
test3TypePolymorphic();

// Test 4-type polymorphic IC
function test4TypePolymorphic() {
    function access(o) {
        return o.x;
    }
    const objs = [
        { x: 1 },
        { x: 2 },
        { x: 3 },
        { x: 4 }
    ];
    for (let i = 0; i < 100; i++) {
        let obj = objs[i % 4];
        assert_equal(access(obj), (i % 4) + 1);
    }
}
test4TypePolymorphic();

// Test polymorphic with different property positions
function testPolymorphicDifferentPositions() {
    function access(o) {
        return o.value;
    }
    const obj1 = { a: 1, value: 100 };
    const obj2 = { value: 200, b: 2 };
    const obj3 = { c: 3, value: 300, d: 4 };
    for (let i = 0; i < 100; i++) {
        if (i % 3 === 0) {
            assert_equal(access(obj1), 100);
        } else if (i % 3 === 1) {
            assert_equal(access(obj2), 200);
        } else {
            assert_equal(access(obj3), 300);
        }
    }
}
testPolymorphicDifferentPositions();

// Test polymorphic with prototype chain
function testPolymorphicPrototypeChain() {
    function access(o) {
        return o.x;
    }
    const proto1 = { x: 1 };
    const proto2 = { x: 2 };
    const obj1 = Object.create(proto1);
    const obj2 = Object.create(proto2);
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(obj1), 1);
        } else {
            assert_equal(access(obj2), 2);
        }
    }
}
testPolymorphicPrototypeChain();

// Test polymorphic with mixed own and inherited
function testPolymorphicMixedOwnInherited() {
    function access(o) {
        return o.x;
    }
    const proto = { x: 1 };
    const obj1 = Object.create(proto);
    const obj2 = { x: 2 };
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(obj1), 1);
        } else {
            assert_equal(access(obj2), 2);
        }
    }
}
testPolymorphicMixedOwnInherited();

// Test polymorphic with accessor
function testPolymorphicWithAccessor() {
    function access(o) {
        return o.x;
    }
    const obj1 = { x: 1 };
    const obj2 = {};
    Object.defineProperty(obj2, "x", {
        get: function() { return 2; }
    });
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(obj1), 1);
        } else {
            assert_equal(access(obj2), 2);
        }
    }
}
testPolymorphicWithAccessor();

// Test polymorphic store
function testPolymorphicStore() {
    function store(o, v) {
        o.x = v;
    }
    const obj1 = { x: 0 };
    const obj2 = { x: 0 };
    const obj3 = { x: 0 };
    for (let i = 0; i < 100; i++) {
        if (i % 3 === 0) {
            store(obj1, i);
        } else if (i % 3 === 1) {
            store(obj2, i);
        } else {
            store(obj3, i);
        }
    }
    assert_equal(obj1.x, 99);
    assert_equal(obj2.x, 97);
    assert_equal(obj3.x, 98);
}
testPolymorphicStore();

// Test polymorphic array and object
function testPolymorphicArrayObject() {
    function access(o) {
        return o[0];
    }
    const arr = [1];
    const obj = { 0: 2 };
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(arr), 1);
        } else {
            assert_equal(access(obj), 2);
        }
    }
}
testPolymorphicArrayObject();

// Test polymorphic with different object shapes
function testPolymorphicDifferentShapes() {
    function access(o) {
        return o.x;
    }
    const obj1 = { x: 1 };
    const obj2 = { x: 2, y: 2 };
    const obj3 = { x: 3, y: 3, z: 3 };
    const obj4 = { x: 4, y: 4, z: 4, w: 4 };
    for (let i = 0; i < 100; i++) {
        let res;
        switch (i % 4) {
            case 0: res = access(obj1); assert_equal(res, 1); break;
            case 1: res = access(obj2); assert_equal(res, 2); break;
            case 2: res = access(obj3); assert_equal(res, 3); break;
            case 3: res = access(obj4); assert_equal(res, 4); break;
        }
    }
}
testPolymorphicDifferentShapes();

// Test polymorphic with primitive and object
function testPolymorphicPrimitiveObject() {
    function access(o) {
        return o.valueOf();
    }
    const num = 42;
    const obj = { valueOf: function() { return 100; } };
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(num), 42);
        } else {
            assert_equal(access(obj), 100);
        }
    }
}
testPolymorphicPrimitiveObject();

// Test polymorphic with function and object
function testPolymorphicFunctionObject() {
    function access(o) {
        return o.x;
    }
    function func() {}
    func.x = 1;
    const obj = { x: 2 };
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(func), 1);
        } else {
            assert_equal(access(obj), 2);
        }
    }
}
testPolymorphicFunctionObject();

// Test polymorphic with class instances
function testPolymorphicClassInstances() {
    function access(o) {
        return o.value;
    }
    class A {
        constructor() { this.value = 1; }
    }
    class B {
        constructor() { this.value = 2; }
    }
    class C {
        constructor() { this.value = 3; }
    }
    const a = new A();
    const b = new B();
    const c = new C();
    for (let i = 0; i < 100; i++) {
        if (i % 3 === 0) {
            assert_equal(access(a), 1);
        } else if (i % 3 === 1) {
            assert_equal(access(b), 2);
        } else {
            assert_equal(access(c), 3);
        }
    }
}
testPolymorphicClassInstances();

// Test polymorphic with null and object
function testPolymorphicNullObject() {
    function access(o) {
        if (o === null) return null;
        return o.x;
    }
    const obj = { x: 1 };
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(null), null);
        } else {
            assert_equal(access(obj), 1);
        }
    }
}
testPolymorphicNullObject();

// Test polymorphic with undefined and object
function testPolymorphicUndefinedObject() {
    function access(o) {
        if (o === undefined) return undefined;
        return o.x;
    }
    const obj = { x: 1 };
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(undefined), undefined);
        } else {
            assert_equal(access(obj), 1);
        }
    }
}
testPolymorphicUndefinedObject();

// Test polymorphic with sealed and normal objects
function testPolymorphicSealedNormal() {
    function access(o) {
        return o.x;
    }
    const obj1 = { x: 1 };
    const obj2 = { x: 2 };
    Object.seal(obj2);
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(obj1), 1);
        } else {
            assert_equal(access(obj2), 2);
        }
    }
}
testPolymorphicSealedNormal();

// Test polymorphic with frozen and normal objects
function testPolymorphicFrozenNormal() {
    function access(o) {
        return o.x;
    }
    const obj1 = { x: 1 };
    const obj2 = { x: 2 };
    Object.freeze(obj2);
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(obj1), 1);
        } else {
            assert_equal(access(obj2), 2);
        }
    }
}
testPolymorphicFrozenNormal();

// Test polymorphic with extensible and non-extensible
function testPolymorphicExtensible() {
    function access(o) {
        return o.x;
    }
    const obj1 = { x: 1 };
    const obj2 = { x: 2 };
    Object.preventExtensions(obj2);
    for (let i = 0; i < 100; i++) {
        if (i % 2 === 0) {
            assert_equal(access(obj1), 1);
        } else {
            assert_equal(access(obj2), 2);
        }
    }
}
testPolymorphicExtensible();

test_end();
