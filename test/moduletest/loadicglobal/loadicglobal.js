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
 * @tc.name:loadicglobal
 * @tc.desc:test loadic on global object
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test global object property load - undefined
function testGlobalUndefined() {
    for (let i = 0; i < 100; i++) {
        let res = undefined;
        assert_equal(res, void 0);
    }
}
testGlobalUndefined();

// Test global object property load - NaN
function testGlobalNaN() {
    for (let i = 0; i < 100; i++) {
        let res = NaN;
        assert_equal(res !== res, true);
    }
}
testGlobalNaN();

// Test global object property load - Infinity
function testGlobalInfinity() {
    for (let i = 0; i < 100; i++) {
        let res = Infinity;
        assert_equal(res > 0, true);
        assert_equal(1 / res, 0);
    }
}
testGlobalInfinity();

// Test global object property load - globalThis
function testGlobalThis() {
    for (let i = 0; i < 100; i++) {
        let res = globalThis;
        assert_equal(typeof res, "object");
    }
}
testGlobalThis();

// Test global constructor load - Object
function testGlobalObject() {
    for (let i = 0; i < 100; i++) {
        let res = Object;
        assert_equal(typeof res, "function");
    }
}
testGlobalObject();

// Test global constructor load - Array
function testGlobalArray() {
    for (let i = 0; i < 100; i++) {
        let res = Array;
        assert_equal(typeof res, "function");
    }
}
testGlobalArray();

// Test global constructor load - Function
function testGlobalFunction() {
    for (let i = 0; i < 100; i++) {
        let res = Function;
        assert_equal(typeof res, "function");
    }
}
testGlobalFunction();

// Test global constructor load - String
function testGlobalString() {
    for (let i = 0; i < 100; i++) {
        let res = String;
        assert_equal(typeof res, "function");
    }
}
testGlobalString();

// Test global constructor load - Number
function testGlobalNumber() {
    for (let i = 0; i < 100; i++) {
        let res = Number;
        assert_equal(typeof res, "function");
    }
}
testGlobalNumber();

// Test global constructor load - Boolean
function testGlobalBoolean() {
    for (let i = 0; i < 100; i++) {
        let res = Boolean;
        assert_equal(typeof res, "function");
    }
}
testGlobalBoolean();

// Test global constructor load - Date
function testGlobalDate() {
    for (let i = 0; i < 100; i++) {
        let res = Date;
        assert_equal(typeof res, "function");
    }
}
testGlobalDate();

// Test global constructor load - RegExp
function testGlobalRegExp() {
    for (let i = 0; i < 100; i++) {
        let res = RegExp;
        assert_equal(typeof res, "function");
    }
}
testGlobalRegExp();

// Test global constructor load - Error
function testGlobalError() {
    for (let i = 0; i < 100; i++) {
        let res = Error;
        assert_equal(typeof res, "function");
    }
}
testGlobalError();

// Test global constructor load - Math
function testGlobalMath() {
    for (let i = 0; i < 100; i++) {
        let res = Math;
        assert_equal(typeof res, "object");
    }
}
testGlobalMath();

// Test global constructor load - JSON
function testGlobalJSON() {
    for (let i = 0; i < 100; i++) {
        let res = JSON;
        assert_equal(typeof res, "object");
    }
}
testGlobalJSON();

// Test global Math property load
function testGlobalMathProperties() {
    for (let i = 0; i < 100; i++) {
        let res1 = Math.PI;
        let res2 = Math.E;
        let res3 = Math.LN2;
        let res4 = Math.LN10;
        assert_equal(typeof res1, "number");
        assert_equal(typeof res2, "number");
        assert_equal(typeof res3, "number");
        assert_equal(typeof res4, "number");
    }
}
testGlobalMathProperties();

// Test global Math method load
function testGlobalMathMethods() {
    for (let i = 0; i < 100; i++) {
        let res1 = Math.abs;
        let res2 = Math.floor;
        let res3 = Math.ceil;
        let res4 = Math.round;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
        assert_equal(typeof res4, "function");
    }
}
testGlobalMathMethods();

// Test global JSON method load
function testGlobalJSONMethods() {
    for (let i = 0; i < 100; i++) {
        let res1 = JSON.parse;
        let res2 = JSON.stringify;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
    }
}
testGlobalJSONMethods();

// Test global Object method load
function testGlobalObjectMethods() {
    for (let i = 0; i < 100; i++) {
        let res1 = Object.create;
        let res2 = Object.defineProperty;
        let res3 = Object.getPrototypeOf;
        let res4 = Object.keys;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
        assert_equal(typeof res4, "function");
    }
}
testGlobalObjectMethods();

// Test global Array method load
function testGlobalArrayMethods() {
    for (let i = 0; i < 100; i++) {
        let res1 = Array.isArray;
        let res2 = Array.from;
        let res3 = Array.of;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "function");
    }
}
testGlobalArrayMethods();

// Test global Number property load
function testGlobalNumberProperties() {
    for (let i = 0; i < 100; i++) {
        let res1 = Number.MAX_VALUE;
        let res2 = Number.MIN_VALUE;
        let res3 = Number.NaN;
        let res4 = Number.POSITIVE_INFINITY;
        let res5 = Number.NEGATIVE_INFINITY;
        assert_equal(typeof res1, "number");
        assert_equal(typeof res2, "number");
        assert_equal(typeof res3, "number");
        assert_equal(typeof res4, "number");
        assert_equal(typeof res5, "number");
    }
}
testGlobalNumberProperties();

// Test global Symbol function load
function testGlobalSymbol() {
    for (let i = 0; i < 100; i++) {
        let res = Symbol;
        assert_equal(typeof res, "function");
    }
}
testGlobalSymbol();

// Test global Symbol constants load
function testGlobalSymbolConstants() {
    for (let i = 0; i < 100; i++) {
        let res1 = Symbol.iterator;
        let res2 = Symbol.toStringTag;
        let res3 = Symbol.hasInstance;
        let res4 = Symbol.species;
        assert_equal(typeof res1, "symbol");
        assert_equal(typeof res2, "symbol");
        assert_equal(typeof res3, "symbol");
        assert_equal(typeof res4, "symbol");
    }
}
testGlobalSymbolConstants();

// Test global parseInt function load
function testGlobalParseInt() {
    for (let i = 0; i < 100; i++) {
        let res = parseInt;
        assert_equal(typeof res, "function");
    }
}
testGlobalParseInt();

// Test global parseFloat function load
function testGlobalParseFloat() {
    for (let i = 0; i < 100; i++) {
        let res = parseFloat;
        assert_equal(typeof res, "function");
    }
}
testGlobalParseFloat();

// Test global isNaN function load
function testGlobalIsNaN() {
    for (let i = 0; i < 100; i++) {
        let res = isNaN;
        assert_equal(typeof res, "function");
    }
}
testGlobalIsNaN();

// Test global isFinite function load
function testGlobalIsFinite() {
    for (let i = 0; i < 100; i++) {
        let res = isFinite;
        assert_equal(typeof res, "function");
    }
}
testGlobalIsFinite();

// Test global encodeURI function load
function testGlobalEncodeURI() {
    for (let i = 0; i < 100; i++) {
        let res = encodeURI;
        assert_equal(typeof res, "function");
    }
}
testGlobalEncodeURI();

// Test global decodeURI function load
function testGlobalDecodeURI() {
    for (let i = 0; i < 100; i++) {
        let res = decodeURI;
        assert_equal(typeof res, "function");
    }
}
testGlobalDecodeURI();

// Test global encodeURIComponent function load
function testGlobalEncodeURIComponent() {
    for (let i = 0; i < 100; i++) {
        let res = encodeURIComponent;
        assert_equal(typeof res, "function");
    }
}
testGlobalEncodeURIComponent();

// Test global decodeURIComponent function load
function testGlobalDecodeURIComponent() {
    for (let i = 0; i < 100; i++) {
        let res = decodeURIComponent;
        assert_equal(typeof res, "function");
    }
}
testGlobalDecodeURIComponent();

// Test global escape function load
function testGlobalEscape() {
    for (let i = 0; i < 100; i++) {
        let res = escape;
        assert_equal(typeof res, "function");
    }
}
testGlobalEscape();

// Test global unescape function load
function testGlobalUnescape() {
    for (let i = 0; i < 100; i++) {
        let res = unescape;
        assert_equal(typeof res, "function");
    }
}
testGlobalUnescape();

// Test global eval function load
function testGlobalEval() {
    for (let i = 0; i < 100; i++) {
        let res = eval;
        assert_equal(typeof res, "function");
    }
}
testGlobalEval();

// Test global property access through globalThis
function testGlobalThisAccess() {
    for (let i = 0; i < 100; i++) {
        let res1 = globalThis.Object;
        let res2 = globalThis.Array;
        let res3 = globalThis.Math;
        assert_equal(typeof res1, "function");
        assert_equal(typeof res2, "function");
        assert_equal(typeof res3, "object");
    }
}
testGlobalThisAccess();

// Test global property load with dynamic property name
function testGlobalDynamicProperty() {
    const props = ["Object", "Array", "Function", "String", "Number"];
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < props.length; j++) {
            let res = globalThis[props[j]];
            assert_equal(typeof res, "function");
        }
    }
}
testGlobalDynamicProperty();

test_end();
