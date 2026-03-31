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
 * @tc.name:loadicobject
 * @tc.desc:test loadic on plain objects
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test basic object property load IC
function testBasicObjectPropertyLoad() {
    const obj = { x: 10, y: 20, z: 30 };
    for (let i = 0; i < 100; i++) {
        let res1 = obj.x;
        let res2 = obj.y;
        let res3 = obj.z;
        assert_equal(res1, 10);
        assert_equal(res2, 20);
        assert_equal(res3, 30);
    }
}
testBasicObjectPropertyLoad();

// Test object property load with different key types
function testObjectPropertyLoadWithDifferentKeys() {
    const obj = { "str": 1, 123: 2, "key with spaces": 3 };
    for (let i = 0; i < 100; i++) {
        let res1 = obj["str"];
        let res2 = obj[123];
        let res3 = obj["key with spaces"];
        assert_equal(res1, 1);
        assert_equal(res2, 2);
        assert_equal(res3, 3);
    }
}
testObjectPropertyLoadWithDifferentKeys();

// Test object property load with computed keys
function testObjectPropertyLoadWithComputedKeys() {
    const obj = { a: 1, b: 2, c: 3 };
    const keys = ["a", "b", "c"];
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < keys.length; j++) {
            let res = obj[keys[j]];
            assert_equal(res, j + 1);
        }
    }
}
testObjectPropertyLoadWithComputedKeys();

// Test object property load with symbol keys
function testObjectPropertyLoadWithSymbolKeys() {
    const sym1 = Symbol("sym1");
    const sym2 = Symbol("sym2");
    const obj = { [sym1]: 100, [sym2]: 200 };
    for (let i = 0; i < 100; i++) {
        let res1 = obj[sym1];
        let res2 = obj[sym2];
        assert_equal(res1, 100);
        assert_equal(res2, 200);
    }
}
testObjectPropertyLoadWithSymbolKeys();

// Test nested object property load IC
function testNestedObjectPropertyLoad() {
    const obj = {
        level1: {
            level2: {
                level3: "deep value"
            }
        }
    };
    for (let i = 0; i < 100; i++) {
        let res = obj.level1.level2.level3;
        assert_equal(res, "deep value");
    }
}
testNestedObjectPropertyLoad();

// Test object with method property load IC
function testObjectMethodPropertyLoad() {
    const obj = {
        method1: function() { return 1; },
        method2: () => 2,
        method3() { return 3; }
    };
    for (let i = 0; i < 100; i++) {
        let res1 = obj.method1();
        let res2 = obj.method2();
        let res3 = obj.method3();
        assert_equal(res1, 1);
        assert_equal(res2, 2);
        assert_equal(res3, 3);
    }
}
testObjectMethodPropertyLoad();

// Test object property load with getter
function testObjectPropertyLoadWithGetter() {
    const obj = {
        get value() {
            return 42;
        }
    };
    for (let i = 0; i < 100; i++) {
        let res = obj.value;
        assert_equal(res, 42);
    }
}
testObjectPropertyLoadWithGetter();

// Test object property load with getter that has side effects
function testObjectPropertyLoadWithStatefulGetter() {
    let counter = 0;
    const obj = {
        get count() {
            return ++counter;
        }
    };
    for (let i = 0; i < 100; i++) {
        let res = obj.count;
        assert_equal(res, i + 1);
    }
}
testObjectPropertyLoadWithStatefulGetter();

// Test object property load on objects with same shape
function testObjectSameShapeIC() {
    function createObject(a, b, c) {
        return { a: a, b: b, c: c };
    }
    const objs = [];
    for (let i = 0; i < 10; i++) {
        objs.push(createObject(i, i * 2, i * 3));
    }
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < objs.length; j++) {
            let res1 = objs[j].a;
            let res2 = objs[j].b;
            let res3 = objs[j].c;
            assert_equal(res1, j);
            assert_equal(res2, j * 2);
            assert_equal(res3, j * 3);
        }
    }
}
testObjectSameShapeIC();

// Test object property load on objects with different shapes
function testObjectDifferentShapeIC() {
    const obj1 = { a: 1 };
    const obj2 = { a: 1, b: 2 };
    const obj3 = { a: 1, b: 2, c: 3 };
    const objs = [obj1, obj2, obj3];
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < objs.length; j++) {
            let res = objs[j].a;
            assert_equal(res, 1);
        }
    }
}
testObjectDifferentShapeIC();

// Test object property load with in operator
function testObjectPropertyLoadWithInOperator() {
    const obj = { a: 1, b: undefined };
    for (let i = 0; i < 100; i++) {
        let res1 = "a" in obj;
        let res2 = "b" in obj;
        let res3 = "c" in obj;
        assert_equal(res1, true);
        assert_equal(res2, true);
        assert_equal(res3, false);
    }
}
testObjectPropertyLoadWithInOperator();

// Test object property load with hasOwnProperty
function testObjectHasOwnProperty() {
    const obj = { own: 1 };
    Object.setPrototypeOf(obj, { inherited: 2 });
    for (let i = 0; i < 100; i++) {
        let res1 = obj.hasOwnProperty("own");
        let res2 = obj.hasOwnProperty("inherited");
        assert_equal(res1, true);
        assert_equal(res2, false);
    }
}
testObjectHasOwnProperty();

// Test object property load after property addition
function testObjectPropertyLoadAfterAddition() {
    const obj = { a: 1 };
    for (let i = 0; i < 50; i++) {
        let res = obj.a;
        assert_equal(res, 1);
    }
    obj.b = 2;
    for (let i = 0; i < 50; i++) {
        let res1 = obj.a;
        let res2 = obj.b;
        assert_equal(res1, 1);
        assert_equal(res2, 2);
    }
}
testObjectPropertyLoadAfterAddition();

// Test object property load after property deletion
function testObjectPropertyLoadAfterDeletion() {
    const obj = { a: 1, b: 2 };
    for (let i = 0; i < 50; i++) {
        let res = obj.a;
        assert_equal(res, 1);
    }
    delete obj.a;
    for (let i = 0; i < 50; i++) {
        let res = obj.a;
        assert_equal(res, undefined);
    }
}
testObjectPropertyLoadAfterDeletion();

// Test object property load on sealed object
function testObjectPropertyLoadOnSealedObject() {
    const obj = { x: 10 };
    Object.seal(obj);
    for (let i = 0; i < 100; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
}
testObjectPropertyLoadOnSealedObject();

// Test object property load on frozen object
function testObjectPropertyLoadOnFrozenObject() {
    const obj = { x: 10 };
    Object.freeze(obj);
    for (let i = 0; i < 100; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
}
testObjectPropertyLoadOnFrozenObject();

// Test object property load on object with non-enumerable property
function testObjectPropertyLoadNonEnumerable() {
    const obj = {};
    Object.defineProperty(obj, "hidden", {
        value: 42,
        enumerable: false
    });
    for (let i = 0; i < 100; i++) {
        let res = obj.hidden;
        assert_equal(res, 42);
    }
}
testObjectPropertyLoadNonEnumerable();

// Test object property load with configurable false
function testObjectPropertyLoadNonConfigurable() {
    const obj = {};
    Object.defineProperty(obj, "fixed", {
        value: 100,
        configurable: false
    });
    for (let i = 0; i < 100; i++) {
        let res = obj.fixed;
        assert_equal(res, 100);
    }
}
testObjectPropertyLoadNonConfigurable();

// Test object property load with writable false
function testObjectPropertyLoadNonWritable() {
    const obj = {};
    Object.defineProperty(obj, "readonly", {
        value: 200,
        writable: false
    });
    for (let i = 0; i < 100; i++) {
        let res = obj.readonly;
        assert_equal(res, 200);
    }
}
testObjectPropertyLoadNonWritable();

// Test object property load chain
function testObjectPropertyLoadChain() {
    const obj = { a: { b: { c: { d: "value" } } } };
    for (let i = 0; i < 100; i++) {
        let res = obj.a.b.c.d;
        assert_equal(res, "value");
    }
}
testObjectPropertyLoadChain();

// Test object property load with function call
function testObjectPropertyLoadWithFunctionCall() {
    const obj = {
        data: [1, 2, 3],
        getLength: function() {
            return this.data.length;
        }
    };
    for (let i = 0; i < 100; i++) {
        let res = obj.getLength();
        assert_equal(res, 3);
    }
}
testObjectPropertyLoadWithFunctionCall();

// Test object property load on empty object
function testObjectPropertyLoadOnEmptyObject() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        let res = obj.nonexistent;
        assert_equal(res, undefined);
    }
}
testObjectPropertyLoadOnEmptyObject();

// Test object property load with null prototype
function testObjectPropertyLoadNullPrototype() {
    const obj = Object.create(null);
    obj.x = 10;
    for (let i = 0; i < 100; i++) {
        let res = obj.x;
        assert_equal(res, 10);
    }
}
testObjectPropertyLoadNullPrototype();

// Test object property load with custom prototype
function testObjectPropertyLoadCustomPrototype() {
    const proto = { inherited: 42 };
    const obj = Object.create(proto);
    obj.own = 10;
    for (let i = 0; i < 100; i++) {
        let res1 = obj.own;
        let res2 = obj.inherited;
        assert_equal(res1, 10);
        assert_equal(res2, 42);
    }
}
testObjectPropertyLoadCustomPrototype();

test_end();
