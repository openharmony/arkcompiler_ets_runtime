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
 * @tc.name:storeicobject
 * @tc.desc:test storeic on plain objects
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test basic object property store
function testBasicObjectPropertyStore() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        obj.x = i;
        assert_equal(obj.x, i);
    }
}
testBasicObjectPropertyStore();

// Test object multiple property store
function testObjectMultiplePropertyStore() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        obj.a = i;
        obj.b = i * 2;
        obj.c = i * 3;
        assert_equal(obj.a, i);
        assert_equal(obj.b, i * 2);
        assert_equal(obj.c, i * 3);
    }
}
testObjectMultiplePropertyStore();

// Test object property store with different value types
function testObjectPropertyStoreDifferentTypes() {
    const obj = {};
    const values = [1, "string", true, null, undefined, {}, [], function() {}];
    for (let i = 0; i < 100; i++) {
        obj.value = values[i % values.length];
        assert_equal(obj.value, values[i % values.length]);
    }
}
testObjectPropertyStoreDifferentTypes();

// Test object property store with string keys
function testObjectPropertyStoreStringKeys() {
    const obj = {};
    const keys = ["a", "b", "c", "key1", "key2", "long_key_name"];
    for (let i = 0; i < 100; i++) {
        const key = keys[i % keys.length];
        obj[key] = i;
        assert_equal(obj[key], i);
    }
}
testObjectPropertyStoreStringKeys();

// Test object property store with numeric keys
function testObjectPropertyStoreNumericKeys() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        obj[i] = i * 10;
        assert_equal(obj[i], i * 10);
    }
}
testObjectPropertyStoreNumericKeys();

// Test object property store with symbol keys
function testObjectPropertyStoreSymbolKeys() {
    const obj = {};
    const sym1 = Symbol("sym1");
    const sym2 = Symbol("sym2");
    for (let i = 0; i < 100; i++) {
        obj[sym1] = i;
        obj[sym2] = i * 2;
        assert_equal(obj[sym1], i);
        assert_equal(obj[sym2], i * 2);
    }
}
testObjectPropertyStoreSymbolKeys();

// Test object property store with computed keys
function testObjectPropertyStoreComputedKeys() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        const key = "prop" + i;
        obj[key] = i;
        assert_equal(obj[key], i);
    }
}
testObjectPropertyStoreComputedKeys();

// Test object property overwrite
function testObjectPropertyOverwrite() {
    const obj = { x: 0 };
    for (let i = 0; i < 100; i++) {
        obj.x = i;
        assert_equal(obj.x, i);
    }
}
testObjectPropertyOverwrite();

// Test object property deletion and re-add
function testObjectPropertyDeletionReadd() {
    const obj = {};
    for (let i = 0; i < 50; i++) {
        obj.temp = i;
        assert_equal(obj.temp, i);
        delete obj.temp;
        assert_equal(obj.temp, undefined);
    }
}
testObjectPropertyDeletionReadd();

// Test object with defineProperty
function testObjectDefineProperty() {
    const obj = {};
    Object.defineProperty(obj, "fixed", {
        value: 100,
        writable: false,
        configurable: false
    });
    for (let i = 0; i < 100; i++) {
        try {
            obj.fixed = i;
        } catch (e) {
            // Should fail in strict mode
        }
        assert_equal(obj.fixed, 100);
    }
}
testObjectDefineProperty();

// Test object with getter and setter
function testObjectGetterSetter() {
    const obj = {};
    let internalValue = 0;
    Object.defineProperty(obj, "value", {
        get: function() { return internalValue; },
        set: function(v) { internalValue = v; }
    });
    for (let i = 0; i < 100; i++) {
        obj.value = i;
        assert_equal(obj.value, i);
    }
}
testObjectGetterSetter();

// Test sealed object property store
function testSealedObjectPropertyStore() {
    const obj = { x: 1 };
    Object.seal(obj);
    for (let i = 0; i < 100; i++) {
        obj.x = i;
        assert_equal(obj.x, i);
    }
}
testSealedObjectPropertyStore();

// Test frozen object property store
function testFrozenObjectPropertyStore() {
    const obj = { x: 1 };
    Object.freeze(obj);
    for (let i = 0; i < 100; i++) {
        try {
            obj.x = i;
        } catch (e) {
            // Should fail in strict mode
        }
        assert_equal(obj.x, 1);
    }
}
testFrozenObjectPropertyStore();

// Test object with prototype chain store
function testObjectProtoChainStore() {
    const proto = { inherited: 10 };
    const obj = Object.create(proto);
    for (let i = 0; i < 100; i++) {
        obj.own = i;
        assert_equal(obj.own, i);
        assert_equal(obj.inherited, 10);
    }
}
testObjectProtoChainStore();

// Test object property store and shape transition
function testObjectShapeTransition() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        obj["prop" + i] = i;
    }
    for (let i = 0; i < 100; i++) {
        assert_equal(obj["prop" + i], i);
    }
}
testObjectShapeTransition();

// Test object with null prototype store
function testNullPrototypeObjectStore() {
    const obj = Object.create(null);
    for (let i = 0; i < 100; i++) {
        obj.x = i;
        assert_equal(obj.x, i);
    }
}
testNullPrototypeObjectStore();

// Test object property store consistency
function testObjectStoreConsistency() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        obj.value = i;
        obj.value = i + 1;
        obj.value = i + 2;
        assert_equal(obj.value, i + 2);
    }
}
testObjectStoreConsistency();

// Test object with many properties store
function testObjectManyPropertiesStore() {
    const obj = {};
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < 10; j++) {
            obj["key" + j] = i * 10 + j;
        }
    }
    for (let j = 0; j < 10; j++) {
        assert_equal(obj["key" + j], 990 + j);
    }
}
testObjectManyPropertiesStore();

test_end();
