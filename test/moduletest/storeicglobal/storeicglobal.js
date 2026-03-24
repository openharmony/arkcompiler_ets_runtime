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
 * @tc.name:storeicglobal
 * @tc.desc:test storeic on global object
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test global object property store
function testGlobalPropertyStore() {
    for (let i = 0; i < 100; i++) {
        globalThis.testGlobalVar = i;
        assert_equal(globalThis.testGlobalVar, i);
    }
}
testGlobalPropertyStore();

// Test global object multiple property store
function testGlobalMultiplePropertyStore() {
    for (let i = 0; i < 100; i++) {
        globalThis.globalA = i;
        globalThis.globalB = i * 2;
        globalThis.globalC = i * 3;
        assert_equal(globalThis.globalA, i);
        assert_equal(globalThis.globalB, i * 2);
        assert_equal(globalThis.globalC, i * 3);
    }
}
testGlobalMultiplePropertyStore();

// Test global object function assignment
function testGlobalFunctionAssignment() {
    for (let i = 0; i < 100; i++) {
        globalThis.globalFunc = function() { return i; };
        assert_equal(globalThis.globalFunc(), i);
    }
}
testGlobalFunctionAssignment();

// Test global object object assignment
function testGlobalObjectAssignment() {
    for (let i = 0; i < 100; i++) {
        globalThis.globalObj = { value: i };
        assert_equal(globalThis.globalObj.value, i);
    }
}
testGlobalObjectAssignment();

// Test global object array assignment
function testGlobalArrayAssignment() {
    for (let i = 0; i < 100; i++) {
        globalThis.globalArr = [i, i + 1, i + 2];
        assert_equal(globalThis.globalArr[0], i);
    }
}
testGlobalArrayAssignment();

// Test global object property modification
function testGlobalPropertyModification() {
    globalThis.modifiable = 0;
    for (let i = 0; i < 100; i++) {
        globalThis.modifiable = i;
        assert_equal(globalThis.modifiable, i);
    }
}
testGlobalPropertyModification();

// Test global object property deletion and recreation
function testGlobalPropertyDeletionRecreation() {
    for (let i = 0; i < 50; i++) {
        globalThis.tempGlobal = i;
        assert_equal(globalThis.tempGlobal, i);
        delete globalThis.tempGlobal;
        assert_equal(globalThis.tempGlobal, undefined);
        globalThis.tempGlobal = i + 100;
        assert_equal(globalThis.tempGlobal, i + 100);
    }
}
testGlobalPropertyDeletionRecreation();

// Test global object with symbol key
function testGlobalSymbolKey() {
    const sym = Symbol("global");
    for (let i = 0; i < 100; i++) {
        globalThis[sym] = i;
        assert_equal(globalThis[sym], i);
    }
}
testGlobalSymbolKey();

// Test global object with computed key
function testGlobalComputedKey() {
    for (let i = 0; i < 100; i++) {
        globalThis["globalProp" + i] = i;
        assert_equal(globalThis["globalProp" + i], i);
    }
}
testGlobalComputedKey();

// Test global object string property store
function testGlobalStringPropertyStore() {
    for (let i = 0; i < 100; i++) {
        globalThis["stringProp"] = "value" + i;
        assert_equal(globalThis.stringProp, "value" + i);
    }
}
testGlobalStringPropertyStore();

// Test global object number property store
function testGlobalNumberPropertyStore() {
    for (let i = 0; i < 100; i++) {
        globalThis[i] = i * 10;
        assert_equal(globalThis[i], i * 10);
    }
}
testGlobalNumberPropertyStore();

// Test global object boolean property store
function testGlobalBooleanPropertyStore() {
    for (let i = 0; i < 100; i++) {
        globalThis.boolProp = i % 2 === 0;
        assert_equal(globalThis.boolProp, i % 2 === 0);
    }
}
testGlobalBooleanPropertyStore();

// Test global object null property store
function testGlobalNullPropertyStore() {
    globalThis.nullProp = "initial";
    for (let i = 0; i < 100; i++) {
        if (i === 50) {
            globalThis.nullProp = null;
        }
    }
    assert_equal(globalThis.nullProp, null);
}
testGlobalNullPropertyStore();

// Test global object undefined property store
function testGlobalUndefinedPropertyStore() {
    globalThis.undefinedProp = "initial";
    for (let i = 0; i < 100; i++) {
        if (i === 50) {
            globalThis.undefinedProp = undefined;
        }
    }
    assert_equal(globalThis.undefinedProp, undefined);
}
testGlobalUndefinedPropertyStore();

// Test global object property enumeration
function testGlobalPropertyEnumeration() {
    globalThis.enumTest1 = 1;
    globalThis.enumTest2 = 2;
    globalThis.enumTest3 = 3;
    let count = 0;
    for (let key in globalThis) {
        if (key.startsWith("enumTest")) {
            count++;
        }
    }
    assert_equal(count >= 3, true);
}
testGlobalPropertyEnumeration();

// Test global object property descriptor
function testGlobalPropertyDescriptor() {
    Object.defineProperty(globalThis, "descriptorTest", {
        value: 100,
        writable: true,
        enumerable: true,
        configurable: true
    });
    for (let i = 0; i < 100; i++) {
        globalThis.descriptorTest = i;
        assert_equal(globalThis.descriptorTest, i);
    }
}
testGlobalPropertyDescriptor();

// Test global object getter/setter
function testGlobalGetterSetter() {
    let internalValue = 0;
    Object.defineProperty(globalThis, "accessorTest", {
        get: function() { return internalValue; },
        set: function(v) { internalValue = v; }
    });
    for (let i = 0; i < 100; i++) {
        globalThis.accessorTest = i;
        assert_equal(globalThis.accessorTest, i);
    }
}
testGlobalGetterSetter();

// Test global object property consistency
function testGlobalPropertyConsistency() {
    for (let i = 0; i < 100; i++) {
        globalThis.consistent = i;
        globalThis.consistent = i + 1;
        globalThis.consistent = i + 2;
        assert_equal(globalThis.consistent, i + 2);
    }
}
testGlobalPropertyConsistency();

test_end();
