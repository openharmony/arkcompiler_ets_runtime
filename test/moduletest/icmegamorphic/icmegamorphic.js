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
 * @tc.name:icmegamorphic
 * @tc.desc:test megamorphic inline cache behavior
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test megamorphic IC with many object types
function testMegamorphicManyTypes() {
    function access(o) {
        return o.x;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = {};
        obj.x = i;
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicManyTypes();

// Test megamorphic IC with different shapes
function testMegamorphicDifferentShapes() {
    function access(o) {
        return o.value;
    }
    const objects = [];
    for (let i = 0; i < 20; i++) {
        const obj = {};
        for (let j = 0; j <= i; j++) {
            obj["prop" + j] = j;
        }
        obj.value = i;
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 20];
        assert_equal(access(obj), i % 20);
    }
}
testMegamorphicDifferentShapes();

// Test megamorphic IC with prototype chains
function testMegamorphicPrototypeChains() {
    function access(o) {
        return o.x;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const proto = { x: i };
        const obj = Object.create(proto);
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicPrototypeChains();

// Test megamorphic IC with mixed property types
function testMegamorphicMixedPropertyTypes() {
    function access(o) {
        return o.value;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = {};
        if (i % 2 === 0) {
            obj.value = i;
        } else {
            Object.defineProperty(obj, "value", {
                get: function() { return i; }
            });
        }
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicMixedPropertyTypes();

// Test megamorphic IC store
function testMegamorphicStore() {
    function store(o, v) {
        o.x = v;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        objects.push({ x: 0 });
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        store(obj, i);
    }
    for (let i = 0; i < 10; i++) {
        assert_equal(objects[i].x, 190 + i);
    }
}
testMegamorphicStore();

// Test megamorphic IC with arrays
function testMegamorphicArrays() {
    function access(o) {
        return o[0];
    }
    const arrays = [];
    for (let i = 0; i < 10; i++) {
        arrays.push([i]);
    }
    for (let i = 0; i < 200; i++) {
        let arr = arrays[i % 10];
        assert_equal(access(arr), i % 10);
    }
}
testMegamorphicArrays();

// Test megamorphic IC with functions
function testMegamorphicFunctions() {
    function access(o) {
        return o.x;
    }
    const functions = [];
    for (let i = 0; i < 10; i++) {
        const func = function() {};
        func.x = i;
        functions.push(func);
    }
    for (let i = 0; i < 200; i++) {
        let func = functions[i % 10];
        assert_equal(access(func), i % 10);
    }
}
testMegamorphicFunctions();

// Test megamorphic IC with class instances
function testMegamorphicClassInstances() {
    function access(o) {
        return o.value;
    }
    const classes = [];
    for (let i = 0; i < 10; i++) {
        classes.push(class {
            constructor() {
                this.value = i;
            }
        });
    }
    const instances = [];
    for (let i = 0; i < 10; i++) {
        instances.push(new classes[i]());
    }
    for (let i = 0; i < 200; i++) {
        let instance = instances[i % 10];
        assert_equal(access(instance), i % 10);
    }
}
testMegamorphicClassInstances();

// Test megamorphic IC with null prototype objects
function testMegamorphicNullPrototype() {
    function access(o) {
        return o.x;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = Object.create(null);
        obj.x = i;
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicNullPrototype();

// Test megamorphic IC with symbol properties
function testMegamorphicSymbolProperties() {
    function access(o) {
        return o[sym];
    }
    const sym = Symbol("test");
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = {};
        obj[sym] = i;
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicSymbolProperties();

// Test megamorphic IC with numeric string keys
function testMegamorphicNumericStringKeys() {
    function access(o) {
        return o["0"];
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = {};
        obj["0"] = i;
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicNumericStringKeys();

// Test megamorphic IC with computed keys
function testMegamorphicComputedKeys() {
    function access(o, key) {
        return o[key];
    }
    const objects = [];
    const keys = [];
    for (let i = 0; i < 10; i++) {
        const obj = {};
        const key = "prop" + i;
        obj[key] = i;
        objects.push(obj);
        keys.push(key);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        let key = keys[i % 10];
        assert_equal(access(obj, key), i % 10);
    }
}
testMegamorphicComputedKeys();

// Test megamorphic IC with method calls
function testMegamorphicMethodCalls() {
    function call(o) {
        return o.getValue();
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = {
            value: i,
            getValue: function() { return this.value; }
        };
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(call(obj), i % 10);
    }
}
testMegamorphicMethodCalls();

// Test megamorphic IC with getters
function testMegamorphicGetters() {
    function access(o) {
        return o.value;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = {};
        Object.defineProperty(obj, "value", {
            get: function() { return i; }
        });
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicGetters();

// Test megamorphic IC with setters
function testMegamorphicSetters() {
    function store(o, v) {
        o.value = v;
    }
    const objects = [];
    const values = [];
    for (let i = 0; i < 10; i++) {
        const obj = {};
        values.push({ val: 0 });
        Object.defineProperty(obj, "value", {
            set: function(v) { values[i].val = v; }
        });
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        store(obj, i);
    }
    for (let i = 0; i < 10; i++) {
        assert_equal(values[i].val, 190 + i);
    }
}
testMegamorphicSetters();

// Test megamorphic IC with sealed objects
function testMegamorphicSealed() {
    function access(o) {
        return o.x;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = { x: i };
        Object.seal(obj);
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicSealed();

// Test megamorphic IC with frozen objects
function testMegamorphicFrozen() {
    function access(o) {
        return o.x;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = { x: i };
        Object.freeze(obj);
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicFrozen();

// Test megamorphic IC with preventExtensions
function testMegamorphicPreventExtensions() {
    function access(o) {
        return o.x;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = { x: i };
        Object.preventExtensions(obj);
        objects.push(obj);
    }
    for (let i = 0; i < 200; i++) {
        let obj = objects[i % 10];
        assert_equal(access(obj), i % 10);
    }
}
testMegamorphicPreventExtensions();

// Test megamorphic IC transition from mono to mega
function testMonoToMegaTransition() {
    function access(o) {
        return o.x;
    }
    const objects = [];
    for (let i = 0; i < 10; i++) {
        const obj = { x: i };
        objects.push(obj);
        for (let j = 0; j < 20; j++) {
            access(obj);
        }
    }
    for (let i = 0; i < 10; i++) {
        assert_equal(access(objects[i]), i);
    }
}
testMonoToMegaTransition();

test_end();
