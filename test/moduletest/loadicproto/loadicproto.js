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
 * @tc.name:loadicproto
 * @tc.desc:test loadic on prototype chain
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test basic prototype chain property load
function testBasicPrototypeChainLoad() {
    const proto = { x: 10 };
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
}
testBasicPrototypeChainLoad();

// Test multi-level prototype chain
function testMultiLevelPrototypeChain() {
    const grandparent = { a: 1 };
    const parent = Object.create(grandparent);
    parent.b = 2;
    const child = Object.create(parent);
    child.c = 3;
    for (let i = 0; i < 100; i++) {
        let res1 = child.a;
        let res2 = child.b;
        let res3 = child.c;
        assert_equal(res1, 1);
        assert_equal(res2, 2);
        assert_equal(res3, 3);
    }
}
testMultiLevelPrototypeChain();

// Test prototype chain with getter
function testPrototypeChainWithGetter() {
    const proto = {
        get value() {
            return 42;
        }
    };
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res = obj.value;
        assert_equal(res, 42);
    }
}
testPrototypeChainWithGetter();

// Test prototype chain with setter
function testPrototypeChainWithSetter() {
    let value = 0;
    const proto = {
        set value(v) {
            value = v;
        }
    };
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        obj.value = i;
        assert_equal(value, i);
    }
}
testPrototypeChainWithSetter();

// Test prototype chain property shadowing
function testPrototypeChainPropertyShadowing() {
    const proto = { x: 10 };
    const obj = Object.create(proto);
    obj.x = 20;
    for (let i = 0; i < 100; i++) {
        let res = obj.x;
        assert_equal(res, 20);
    }
}
testPrototypeChainPropertyShadowing();

// Test prototype chain method inheritance
function testPrototypeChainMethodInheritance() {
    const proto = {
        greet: function() {
            return "Hello";
        }
    };
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res = obj.greet();
        assert_equal(res, "Hello");
    }
}
testPrototypeChainMethodInheritance();

// Test prototype chain with Array prototype
function testPrototypeChainArrayPrototype() {
    const arr = [1, 2, 3];
    for (let i = 0; i < 100; i++) {
        let res = arr.push;
        assert_equal(typeof res, "function");
    }
}
testPrototypeChainArrayPrototype();

// Test prototype chain with Object prototype
function testPrototypeChainObjectPrototype() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        let res1 = obj.toString;
        let res2 = obj.valueOf;
        let res3 = obj.hasOwnProperty;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
    }
}
testPrototypeChainObjectPrototype();

// Test prototype chain with Function prototype
function testPrototypeChainFunctionPrototype() {
    function foo() {}
    for (let i = 0; i < 100; i++) {
        let res1 = foo.call;
        let res2 = foo.apply;
        let res3 = foo.bind;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
    }
}
testPrototypeChainFunctionPrototype();

// Test prototype chain with String prototype
function testPrototypeChainStringPrototype() {
    const str = "hello";
    for (let i = 0; i < 100; i++) {
        let res1 = str.charAt;
        let res2 = str.substring;
        let res3 = str.indexOf;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
    }
}
testPrototypeChainStringPrototype();

// Test prototype chain with Number prototype
function testPrototypeChainNumberPrototype() {
    const num = 123;
    for (let i = 0; i < 100; i++) {
        let res1 = num.toString;
        let res2 = num.toFixed;
        let res3 = num.valueOf;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
    }
}
testPrototypeChainNumberPrototype();

// Test prototype chain modification
function testPrototypeChainModification() {
    const proto = { x: 10 };
    const obj = Object.create(proto);
    for (let i = 0; i < 50; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
    proto.x = 20;
    for (let i = 0; i < 50; i++) {
        let res = obj.x;
        assert_equal(res, 20);
    }
}
testPrototypeChainModification();

// Test prototype chain extension
function testPrototypeChainExtension() {
    const proto = { x: 10 };
    const obj = Object.create(proto);
    for (let i = 0; i < 50; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
    proto.y = 20;
    for (let i = 0; i < 50; i++) {
        let res1 = obj.x;
        let res2 = obj.y;
        assert_equal(res1, 10);
        assert_equal(res2, 20);
    }
}
testPrototypeChainExtension();

// Test prototype chain with Object.setPrototypeOf
function testPrototypeChainSetPrototypeOf() {
    const proto1 = { a: 1 };
    const proto2 = { b: 2 };
    const obj = Object.create(proto1);
    for (let i = 0; i < 50; i++) {
        let res = obj.a;
        assert_equal(res, 1);
    }
    Object.setPrototypeOf(obj, proto2);
    for (let i = 0; i < 50; i++) {
        let res = obj.b;
        assert_equal(res, 2);
    }
}
testPrototypeChainSetPrototypeOf();

// Test prototype chain with __proto__
function testPrototypeChainProto() {
    const proto = { x: 10 };
    const obj = {};
    obj.__proto__ = proto;
    for (let i = 0; i < 100; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
}
testPrototypeChainProto();

// Test prototype chain circular reference check
function testPrototypeChainCircularCheck() {
    const proto = {};
    try {
        proto.__proto__ = proto;
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}
testPrototypeChainCircularCheck();

// Test prototype chain with null prototype
function testPrototypeChainNullPrototype() {
    const obj = Object.create(null);
    obj.x = 10;
    for (let i = 0; i < 100; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
}
testPrototypeChainNullPrototype();

// Test prototype chain performance with many objects
function testPrototypeChainManyObjects() {
    const proto = { shared: 42 };
    const objs = [];
    for (let i = 0; i < 100; i++) {
        objs.push(Object.create(proto));
    }
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < objs.length; j++) {
            let res = objs[j].shared;
            assert_equal(res, 42);
        }
    }
}
testPrototypeChainManyObjects();

// Test prototype chain with accessor on prototype
function testPrototypeChainAccessorOnPrototype() {
    let count = 0;
    const proto = {
        get counter() {
            return ++count;
        }
    };
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res = obj.counter;
        assert_equal(res, i + 1);
    }
}
testPrototypeChainAccessorOnPrototype();

// Test prototype chain with non-configurable property
function testPrototypeChainNonConfigurable() {
    const proto = {};
    Object.defineProperty(proto, "fixed", {
        value: 100,
        configurable: false
    });
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res = obj.fixed;
        assert_equal(res, 100);
    }
}
testPrototypeChainNonConfigurable();

// Test prototype chain with non-writable property
function testPrototypeChainNonWritable() {
    const proto = {};
    Object.defineProperty(proto, "readonly", {
        value: 200,
        writable: false
    });
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res = obj.readonly;
        assert_equal(res, 200);
    }
}
testPrototypeChainNonWritable();

// Test prototype chain with enumerable property
function testPrototypeChainEnumerable() {
    const proto = {};
    Object.defineProperty(proto, "visible", {
        value: 300,
        enumerable: true
    });
    Object.defineProperty(proto, "hidden", {
        value: 400,
        enumerable: false
    });
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res1 = obj.visible;
        let res2 = obj.hidden;
        assert_equal(res1, 300);
        assert_equal(res2, 400);
    }
}
testPrototypeChainEnumerable();

// Test prototype chain with Symbol property
function testPrototypeChainSymbolProperty() {
    const sym = Symbol("test");
    const proto = { [sym]: 500 };
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        let res = obj[sym];
        assert_equal(res, 500);
    }
}
testPrototypeChainSymbolProperty();

// Test prototype chain with well-known Symbols
function testPrototypeChainWellKnownSymbols() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        let res1 = obj[Symbol.toStringTag];
        let res2 = obj[Symbol.iterator];
        assert_equal(res1, undefined);
        assert_equal(res2, undefined);
    }
}
testPrototypeChainWellKnownSymbols();

// Test prototype chain property lookup order
function testPrototypeChainLookupOrder() {
    const grandparent = { x: 1, y: 1 };
    const parent = Object.create(grandparent);
    parent.y = 2;
    const child = Object.create(parent);
    child.z = 3;
    for (let i = 0; i < 100; i++) {
        let res1 = child.x;
        let res2 = child.y;
        let res3 = child.z;
        assert_equal(res1, 1);
        assert_equal(res2, 2);
        assert_equal(res3, 3);
    }
}
testPrototypeChainLookupOrder();

test_end();
