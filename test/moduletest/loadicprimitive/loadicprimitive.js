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
 * @tc.name:loadicprimitive
 * @tc.desc:test loadic on primitive types
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test number primitive IC - toString method
function testNumberToString() {
    const num = 12345;
    for (let i = 0; i < 100; i++) {
        let res = num.toString();
        assert_equal(res, "12345");
    }
}
testNumberToString();

// Test number primitive IC - toFixed method
function testNumberToFixed() {
    const num = 123.456;
    for (let i = 0; i < 100; i++) {
        let res = num.toFixed(2);
        assert_equal(res, "123.46");
    }
}
testNumberToFixed();

// Test number primitive IC - toExponential method
function testNumberToExponential() {
    const num = 12345;
    for (let i = 0; i < 100; i++) {
        let res = num.toExponential(2);
        assert_equal(res, "1.23e+4");
    }
}
testNumberToExponential();

// Test number primitive IC - toPrecision method
function testNumberToPrecision() {
    const num = 123.456;
    for (let i = 0; i < 100; i++) {
        let res = num.toPrecision(4);
        assert_equal(res, "123.5");
    }
}
testNumberToPrecision();

// Test number primitive IC - valueOf method
function testNumberValueOf() {
    const num = 42;
    for (let i = 0; i < 100; i++) {
        let res = num.valueOf();
        assert_equal(res, 42);
    }
}
testNumberValueOf();

// Test string primitive IC - charAt method
function testStringCharAt() {
    const str = "Hello World";
    for (let i = 0; i < 100; i++) {
        let res = str.charAt(0);
        assert_equal(res, "H");
    }
}
testStringCharAt();

// Test string primitive IC - charCodeAt method
function testStringCharCodeAt() {
    const str = "ABC";
    for (let i = 0; i < 100; i++) {
        let res = str.charCodeAt(0);
        assert_equal(res, 65);
    }
}
testStringCharCodeAt();

// Test string primitive IC - concat method
function testStringConcat() {
    const str1 = "Hello";
    const str2 = " World";
    for (let i = 0; i < 100; i++) {
        let res = str1.concat(str2);
        assert_equal(res, "Hello World");
    }
}
testStringConcat();

// Test string primitive IC - indexOf method
function testStringIndexOf() {
    const str = "Hello World";
    for (let i = 0; i < 100; i++) {
        let res = str.indexOf("World");
        assert_equal(res, 6);
    }
}
testStringIndexOf();

// Test string primitive IC - slice method
function testStringSlice() {
    const str = "Hello World";
    for (let i = 0; i < 100; i++) {
        let res = str.slice(0, 5);
        assert_equal(res, "Hello");
    }
}
testStringSlice();

// Test string primitive IC - substring method
function testStringSubstring() {
    const str = "Hello World";
    for (let i = 0; i < 100; i++) {
        let res = str.substring(6, 11);
        assert_equal(res, "World");
    }
}
testStringSubstring();

// Test string primitive IC - substr method
function testStringSubstr() {
    const str = "Hello World";
    for (let i = 0; i < 100; i++) {
        let res = str.substr(6, 5);
        assert_equal(res, "World");
    }
}
testStringSubstr();

// Test string primitive IC - split method
function testStringSplit() {
    const str = "a,b,c,d";
    for (let i = 0; i < 100; i++) {
        let res = str.split(",");
        assert_equal(res.length, 4);
    }
}
testStringSplit();

// Test string primitive IC - toLowerCase method
function testStringToLowerCase() {
    const str = "HELLO WORLD";
    for (let i = 0; i < 100; i++) {
        let res = str.toLowerCase();
        assert_equal(res, "hello world");
    }
}
testStringToLowerCase();

// Test string primitive IC - toUpperCase method
function testStringToUpperCase() {
    const str = "hello world";
    for (let i = 0; i < 100; i++) {
        let res = str.toUpperCase();
        assert_equal(res, "HELLO WORLD");
    }
}
testStringToUpperCase();

// Test string primitive IC - trim method
function testStringTrim() {
    const str = "  hello world  ";
    for (let i = 0; i < 100; i++) {
        let res = str.trim();
        assert_equal(res, "hello world");
    }
}
testStringTrim();

// Test string primitive IC - length property
function testStringLength() {
    const str = "Hello World";
    for (let i = 0; i < 100; i++) {
        let res = str.length;
        assert_equal(res, 11);
    }
}
testStringLength();

// Test boolean primitive IC - valueOf method
function testBooleanValueOf() {
    const bool = true;
    for (let i = 0; i < 100; i++) {
        let res = bool.valueOf();
        assert_equal(res, true);
    }
}
testBooleanValueOf();

// Test boolean primitive IC - toString method
function testBooleanToString() {
    const bool = false;
    for (let i = 0; i < 100; i++) {
        let res = bool.toString();
        assert_equal(res, "false");
    }
}
testBooleanToString();

// Test symbol primitive IC - toString method
function testSymbolToString() {
    const sym = Symbol("test");
    for (let i = 0; i < 100; i++) {
        let res = sym.toString();
        assert_equal(typeof res, "string");
    }
}
testSymbolToString();

// Test symbol primitive IC - valueOf method
function testSymbolValueOf() {
    const sym = Symbol("test");
    for (let i = 0; i < 100; i++) {
        let res = sym.valueOf();
        assert_equal(typeof res, "symbol");
    }
}
testSymbolValueOf();

// Test number object wrapper IC
function testNumberObjectWrapper() {
    const numObj = new Number(123);
    for (let i = 0; i < 100; i++) {
        let res1 = numObj.toString();
        let res2 = numObj.valueOf();
        assert_equal(res1, "123");
        assert_equal(res2, 123);
    }
}
testNumberObjectWrapper();

// Test string object wrapper IC
function testStringObjectWrapper() {
    const strObj = new String("test");
    for (let i = 0; i < 100; i++) {
        let res1 = strObj.toString();
        let res2 = strObj.valueOf();
        assert_equal(res1, "test");
        assert_equal(res2, "test");
    }
}
testStringObjectWrapper();

// Test boolean object wrapper IC
function testBooleanObjectWrapper() {
    const boolObj = new Boolean(true);
    for (let i = 0; i < 100; i++) {
        let res1 = boolObj.toString();
        let res2 = boolObj.valueOf();
        assert_equal(res1, "true");
        assert_equal(res2, true);
    }
}
testBooleanObjectWrapper();

// Test mixed primitive IC - number and string alternation
function testMixedPrimitiveIC() {
    for (let i = 0; i < 100; i++) {
        let num = 123;
        let str = "abc";
        let res1 = num.toString();
        let res2 = str.toUpperCase();
        assert_equal(res1, "123");
        assert_equal(res2, "ABC");
    }
}
testMixedPrimitiveIC();

// Test primitive IC with prototype modification
function testPrimitiveICWithProtoModification() {
    function foo() { return 100; }
    Object.defineProperty(Number.prototype, "customMethod", { get: foo, configurable: true });
    const num = 42;
    for (let i = 0; i < 100; i++) {
        let res = num.customMethod;
        assert_equal(res, 100);
    }
    delete Number.prototype.customMethod;
}
testPrimitiveICWithProtoModification();

// Test primitive IC with data property on prototype
function testPrimitiveICWithDataProperty() {
    Object.defineProperty(String.prototype, "customProp", { value: "customValue", configurable: true });
    const str = "test";
    for (let i = 0; i < 100; i++) {
        let res = str.customProp;
        assert_equal(res, "customValue");
    }
    delete String.prototype.customProp;
}
testPrimitiveICWithDataProperty();

// Test IC on special numeric values
function testSpecialNumericValues() {
    const values = [0, -0, Infinity, -Infinity, NaN];
    for (let j = 0; j < values.length; j++) {
        for (let i = 0; i < 20; i++) {
            let res = values[j].toString();
            assert_equal(typeof res, "string");
        }
    }
}
testSpecialNumericValues();

// Test IC on very long strings
function testLongStringIC() {
    let longStr = "a";
    for (let i = 0; i < 10; i++) {
        longStr = longStr + longStr;
    }
    for (let i = 0; i < 100; i++) {
        let res = longStr.length;
        assert_equal(res, 1024);
    }
}
testLongStringIC();

// Test IC consistency across function calls
function testICConsistency() {
    function testNumber(num) {
        return num.toString();
    }
    for (let i = 0; i < 50; i++) {
        assert_equal(testNumber(100), "100");
        assert_equal(testNumber(200), "200");
        assert_equal(testNumber(300), "300");
    }
}
testICConsistency();

// Test primitive IC megamorphic behavior
function testMegamorphicPrimitiveIC() {
    for (let i = 0; i < 100; i++) {
        let num1 = 1;
        let num2 = 2;
        let num3 = 3;
        let num4 = 4;
        let num5 = 5;
        num1.toString();
        num2.toString();
        num3.toString();
        num4.toString();
        num5.toString();
    }
}
testMegamorphicPrimitiveIC();

// Test primitive IC with different prototype chains
function testPrimitiveICWithDifferentProtos() {
    const num = 123;
    for (let i = 0; i < 100; i++) {
        let res1 = num.toString;
        let res2 = num.valueOf;
        let res3 = num.toFixed;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
    }
}
testPrimitiveICWithDifferentProtos();

test_end();
