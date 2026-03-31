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
 * @tc.name:storeicprimitive
 * @tc.desc:test storeic on primitive wrapper objects
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test Number object property store
function testNumberObjectPropertyStore() {
    const num = new Number(123);
    for (let i = 0; i < 100; i++) {
        num.custom = i;
        assert_equal(num.custom, i);
    }
}
testNumberObjectPropertyStore();

// Test String object property store
function testStringObjectPropertyStore() {
    const str = new String("hello");
    for (let i = 0; i < 100; i++) {
        str.index = i;
        assert_equal(str.index, i);
    }
}
testStringObjectPropertyStore();

// Test Boolean object property store
function testBooleanObjectPropertyStore() {
    const bool = new Boolean(true);
    for (let i = 0; i < 100; i++) {
        bool.value = i;
        assert_equal(bool.value, i);
    }
}
testBooleanObjectPropertyStore();

// Test Number object multiple property store
function testNumberObjectMultiplePropertyStore() {
    const num = new Number(456);
    for (let i = 0; i < 100; i++) {
        num.a = i;
        num.b = i * 2;
        num.c = i * 3;
        assert_equal(num.a, i);
        assert_equal(num.b, i * 2);
        assert_equal(num.c, i * 3);
    }
}
testNumberObjectMultiplePropertyStore();

// Test String object multiple property store
function testStringObjectMultiplePropertyStore() {
    const str = new String("test");
    for (let i = 0; i < 100; i++) {
        str.x = i;
        str.y = i + 1;
        str.z = i + 2;
        assert_equal(str.x, i);
        assert_equal(str.y, i + 1);
        assert_equal(str.z, i + 2);
    }
}
testStringObjectMultiplePropertyStore();

// Test primitive number box property store
function testPrimitiveNumberBoxStore() {
    for (let i = 0; i < 100; i++) {
        let num = 123;
        // In strict mode, setting property on primitive throws TypeError
        // This is expected behavior - primitive values cannot have properties stored
        let errorThrown = false;
        try {
            num.custom = i;
        } catch (e) {
            errorThrown = true;
        }
        // Either error is thrown or property is silently ignored (non-strict)
        // Both behaviors are acceptable per ECMAScript spec
        assert_equal(num.custom, undefined);
    }
}
testPrimitiveNumberBoxStore();

// Test primitive string box property store
function testPrimitiveStringBoxStore() {
    for (let i = 0; i < 100; i++) {
        let str = "hello";
        // In strict mode, setting property on primitive throws TypeError
        // This is expected behavior - primitive values cannot have properties stored
        let errorThrown = false;
        try {
            str.custom = i;
        } catch (e) {
            errorThrown = true;
        }
        // Either error is thrown or property is silently ignored (non-strict)
        // Both behaviors are acceptable per ECMAScript spec
        assert_equal(str.custom, undefined);
    }
}
testPrimitiveStringBoxStore();

// Test Number object with defineProperty
function testNumberObjectDefineProperty() {
    const num = new Number(789);
    Object.defineProperty(num, "fixed", {
        value: 100,
        writable: false,
        configurable: false
    });
    for (let i = 0; i < 100; i++) {
        let res = num.fixed;
        assert_equal(res, 100);
    }
}
testNumberObjectDefineProperty();

// Test String object with defineProperty
function testStringObjectDefineProperty() {
    const str = new String("world");
    Object.defineProperty(str, "readonly", {
        get: function() { return 42; },
        set: function(v) {}
    });
    for (let i = 0; i < 100; i++) {
        str.readonly = i;
        let res = str.readonly;
        assert_equal(res, 42);
    }
}
testStringObjectDefineProperty();

// Test Number object with getter/setter
function testNumberObjectGetterSetter() {
    const num = new Number(100);
    let internal = 0;
    Object.defineProperty(num, "value", {
        get: function() { return internal; },
        set: function(v) { internal = v; }
    });
    for (let i = 0; i < 100; i++) {
        num.value = i;
        assert_equal(num.value, i);
    }
}
testNumberObjectGetterSetter();

// Test String object with getter/setter
function testStringObjectGetterSetter() {
    const str = new String("abc");
    let internal = "";
    Object.defineProperty(str, "content", {
        get: function() { return internal; },
        set: function(v) { internal = v; }
    });
    for (let i = 0; i < 100; i++) {
        str.content = "val" + i;
        assert_equal(str.content, "val" + i);
    }
}
testStringObjectGetterSetter();

// Test Number object property deletion
function testNumberObjectPropertyDeletion() {
    const num = new Number(50);
    num.temp = 123;
    for (let i = 0; i < 50; i++) {
        assert_equal(num.temp, 123);
    }
    delete num.temp;
    for (let i = 0; i < 50; i++) {
        assert_equal(num.temp, undefined);
    }
}
testNumberObjectPropertyDeletion();

// Test String object property deletion
function testStringObjectPropertyDeletion() {
    const str = new String("xyz");
    str.temp = "abc";
    for (let i = 0; i < 50; i++) {
        assert_equal(str.temp, "abc");
    }
    delete str.temp;
    for (let i = 0; i < 50; i++) {
        assert_equal(str.temp, undefined);
    }
}
testStringObjectPropertyDeletion();

// Test Number object prototype modification
function testNumberObjectProtoModification() {
    const num = new Number(10);
    Number.prototype.shared = 999;
    for (let i = 0; i < 100; i++) {
        let res = num.shared;
        assert_equal(res, 999);
    }
    delete Number.prototype.shared;
}
testNumberObjectProtoModification();

// Test String object prototype modification
function testStringObjectProtoModification() {
    const str = new String("test");
    String.prototype.shared = "common";
    for (let i = 0; i < 100; i++) {
        let res = str.shared;
        assert_equal(res, "common");
    }
    delete String.prototype.shared;
}
testStringObjectProtoModification();

// Test Number object with symbol key
function testNumberObjectSymbolKey() {
    const num = new Number(25);
    const sym = Symbol("key");
    for (let i = 0; i < 100; i++) {
        num[sym] = i;
        assert_equal(num[sym], i);
    }
}
testNumberObjectSymbolKey();

// Test String object with symbol key
function testStringObjectSymbolKey() {
    const str = new String("sym");
    const sym = Symbol("key");
    for (let i = 0; i < 100; i++) {
        str[sym] = i;
        assert_equal(str[sym], i);
    }
}
testStringObjectSymbolKey();

// Test Number object with numeric string key
function testNumberObjectNumericStringKey() {
    const num = new Number(33);
    for (let i = 0; i < 100; i++) {
        num["key" + i] = i * 10;
        assert_equal(num["key" + i], i * 10);
    }
}
testNumberObjectNumericStringKey();

// Test String object with numeric string key
function testStringObjectNumericStringKey() {
    const str = new String("num");
    for (let i = 0; i < 100; i++) {
        str["key" + i] = i * 10;
        assert_equal(str["key" + i], i * 10);
    }
}
testStringObjectNumericStringKey();

// Test Number object store and load consistency
function testNumberObjectStoreLoadConsistency() {
    const num = new Number(77);
    for (let i = 0; i < 100; i++) {
        num.data = i;
        let res = num.data;
        assert_equal(res, i);
    }
}
testNumberObjectStoreLoadConsistency();

// Test String object store and load consistency
function testStringObjectStoreLoadConsistency() {
    const str = new String("consistent");
    for (let i = 0; i < 100; i++) {
        str.data = i;
        let res = str.data;
        assert_equal(res, i);
    }
}
testStringObjectStoreLoadConsistency();

test_end();
