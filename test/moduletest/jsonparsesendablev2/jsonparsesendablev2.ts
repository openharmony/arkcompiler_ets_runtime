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
 * @tc.name:jsonparsesendablev2
 * @tc.desc:test JSON.parseSendableV2
 * @tc.type: FUNC
 * @tc.require:
 */

// @ts-nocheck

const enum BigIntMode {
    DEFAULT = 0,
    PARSE_AS_BIGINT = 1,
    ALWAYS_PARSE_AS_BIGINT = 2,
}

const enum ParseReturnType {
    OBJECT = 0,
    MAP = 1,
}

function testParseSendableV2Primitives() {
    assert_equal(JSON.parseSendableV2("null"), null);
    assert_equal(JSON.parseSendableV2("true"), true);
    assert_equal(JSON.parseSendableV2("false"), false);
    assert_equal(JSON.parseSendableV2("123"), 123);
    assert_equal(JSON.parseSendableV2("-42"), -42);
    assert_equal(JSON.parseSendableV2("1.5"), 1.5);
    assert_equal(JSON.parseSendableV2('"hello"'), "hello");
    assert_equal(JSON.parseSendableV2('""'), "");
    assert_equal(Object.is(JSON.parseSendableV2("-0"), -0), true);
    assert_equal(Object.is(JSON.parseSendableV2("0"), 0), true);
    assert_equal(typeof JSON.parseSendableV2("null"), "object");
    assert_equal(typeof JSON.parseSendableV2("123"), "number");
    assert_equal(typeof JSON.parseSendableV2('"hello"'), "string");
}

function testParseSendableV2BasicObject() {
    let text = '{"a":1,"b":"str","c":true,"d":false,"e":null,"f":{"g":2},"h":[3,4]}';
    let obj = JSON.parseSendableV2(text);
    assert_equal(obj.a, 1);
    assert_equal(obj.b, "str");
    assert_equal(obj.c, true);
    assert_equal(obj.d, false);
    assert_equal(obj.e, null);
    assert_equal(obj.f.g, 2);
    assert_equal(obj.h[0], 3);
    assert_equal(obj.h[1], 4);
    assert_equal(JSON.stringifySendable(obj), text);
    assert_equal(Object.keys(obj), ["a", "b", "c", "d", "e", "f", "h"]);
    assert_equal("a" in obj, true);
    assert_equal(Object.prototype.hasOwnProperty.call(obj, "a"), true);
    assert_equal(Object.prototype.hasOwnProperty.call(obj, "noexist"), false);
    assert_equal(typeof obj, "object");
}

function testParseSendableV2EmptyObjectAndArray() {
    let obj = JSON.parseSendableV2("{}");
    assert_equal(JSON.stringifySendable(obj), "{}");
    assert_equal(Object.keys(obj), []);
    let arr = JSON.parseSendableV2("[]");
    assert_equal(JSON.stringifySendable(arr), "[]");
    assert_equal(arr.length, 0);
    let nested = JSON.parseSendableV2('{"a":{},"b":[],"c":{"d":{}}}');
    assert_equal(JSON.stringifySendable(nested), '{"a":{},"b":[],"c":{"d":{}}}');
    let arrInArr = JSON.parseSendableV2("[[],[1],[2,[3]]]");
    assert_equal(JSON.stringifySendable(arrInArr), "[[],[1],[2,[3]]]");
}

function testParseSendableV2Nested() {
    let depth = 100;
    let text = "";
    for (let i = 0; i < depth; i++) {
        text += '{"a":';
    }
    text += "1";
    for (let i = 0; i < depth; i++) {
        text += "}";
    }
    let obj = JSON.parseSendableV2(text);
    let cur = obj;
    for (let i = 0; i < depth - 1; i++) {
        cur = cur.a;
    }
    assert_equal(cur.a, 1);

    let arrayText = "[".repeat(50) + "7" + "]".repeat(50);
    let arr = JSON.parseSendableV2(arrayText);
    let curArr = arr;
    for (let i = 0; i < 49; i++) {
        curArr = curArr[0];
    }
    assert_equal(curArr[0], 7);

    let mixed = JSON.parseSendableV2('{"list":[{"x":[1,{"y":2}]}],"end":true}');
    assert_equal(mixed.list[0].x[1].y, 2);
    assert_equal(mixed.end, true);
}

function testParseSendableV2Numbers() {
    assert_equal(JSON.parseSendableV2("0"), 0);
    assert_equal(JSON.parseSendableV2("1e2"), 100);
    assert_equal(JSON.parseSendableV2("1E+2"), 100);
    assert_equal(JSON.parseSendableV2("1e-2"), 0.01);
    assert_equal(JSON.parseSendableV2("1.5e3"), 1500);
    assert_equal(JSON.parseSendableV2("-1.5"), -1.5);
    assert_equal(JSON.parseSendableV2("1.0"), 1);
    assert_equal(JSON.parseSendableV2("9007199254740991"), 9007199254740991);
    assert_equal(JSON.parseSendableV2("9007199254740993"), 9007199254740992);
    assert_equal(JSON.parseSendableV2("1234567890.0123456"), 1234567890.0123456);
    assert_equal(JSON.parseSendableV2("1.7976931348623157e+308"), 1.7976931348623157e+308);
    assert_equal(JSON.parseSendableV2("1e309"), Infinity);
    assert_equal(JSON.parseSendableV2("-1e309"), -Infinity);
    assert_equal(JSON.parseSendableV2("1e-400"), 0);
    assert_equal(JSON.parseSendableV2("5e-324"), 5e-324);
    assert_equal(JSON.parseSendableV2("0.1"), 0.1);
}

function testParseSendableV2Strings() {
    let ctrls = JSON.parseSendableV2('"a\\nb\\tc\\rd"');
    assert_equal(ctrls.length, 7);
    assert_equal(ctrls.charCodeAt(1), 10);
    assert_equal(ctrls.charCodeAt(3), 9);
    assert_equal(ctrls.charCodeAt(5), 13);
    assert_equal(JSON.parseSendableV2('"\\b\\f"').charCodeAt(0), 8);
    assert_equal(JSON.parseSendableV2('"\\b\\f"').charCodeAt(1), 12);
    assert_equal(JSON.parseSendableV2('"quoted"'), "quoted");
    assert_equal(JSON.parseSendableV2('"\\"q\\""'), '"q"');
    assert_equal(JSON.parseSendableV2('"back\\\\slash"'), "back\\slash");
    assert_equal(JSON.parseSendableV2('"slash\\/"'), "slash/");
    assert_equal(JSON.parseSendableV2('"\\u0041\\u0042"'), "AB");
    assert_equal(JSON.parseSendableV2('"\\u4f60\\u597d"'), "你好");
    let emoji = JSON.parseSendableV2('"\\ud83d\\ude00"');
    assert_equal(emoji.length, 2);
    assert_equal(emoji.charCodeAt(0), 55357);
    assert_equal(emoji.charCodeAt(1), 56832);
    let control = JSON.parseSendableV2('"a\\u0001b"');
    assert_equal(control.length, 3);
    assert_equal(control.charCodeAt(1), 1);
    let longText = '"' + "x".repeat(1000) + '"';
    assert_equal(JSON.parseSendableV2(longText), "x".repeat(1000));
}

function testParseSendableV2Whitespace() {
    assert_equal(JSON.stringifySendable(JSON.parseSendableV2(' { "a" : 1 } ')), '{"a":1}');
    assert_equal(JSON.stringifySendable(JSON.parseSendableV2('\t\n\r [ 1 , 2 ] ')), '[1,2]');
    assert_equal(JSON.stringifySendable(JSON.parseSendableV2('  {"a" : [ 1 , { "b" : 2 } ] }  ')),
        '{"a":[1,{"b":2}]}');
    assert_equal(JSON.parseSendableV2('  "space"  '), "space");
    assert_equal(JSON.parseSendableV2('\ttrue\n'), true);
}

function testParseSendableV2DuplicateKeys() {
    let obj = JSON.parseSendableV2('{"a":1,"a":2,"b":3,"a":4}');
    assert_equal(obj.a, 4);
    assert_equal(obj.b, 3);
    assert_equal(Object.keys(obj), ["a", "b"]);
    assert_equal(JSON.stringifySendable(obj), '{"a":4,"b":3}');
}

function testParseSendableV2SpecialKeys() {
    let obj = JSON.parseSendableV2('{"":1," x ":2,"a.b":3,"a[0]":4}');
    assert_equal(obj[""], 1);
    assert_equal(obj[" x "], 2);
    assert_equal(obj["a.b"], 3);
    assert_equal(obj["a[0]"], 4);
    let numKeys = JSON.parseSendableV2('{"0":"a","1":"b","2":"c"}');
    assert_equal(numKeys[0], "a");
    assert_equal(numKeys[1], "b");
    assert_equal(numKeys[2], "c");
    assert_equal(numKeys.length, undefined);
    let sparseKeys = JSON.parseSendableV2('{"0":"a","2":"c"}');
    assert_equal(sparseKeys[0], "a");
    assert_equal(sparseKeys[2], "c");
    let protoObj = JSON.parseSendableV2('{"__proto__":{"x":1},"a":2}');
    assert_equal(Object.prototype.hasOwnProperty.call(protoObj, "__proto__"), true);
    assert_equal(protoObj.a, 2);
}

function testParseSendableV2BigIntMode() {
    let text = '{"big":1122334455667788999,"small":123,"neg":-5,"zero":0,"deci":1.5}';
    let opt1 = { bigIntMode: BigIntMode.PARSE_AS_BIGINT };
    let obj1 = JSON.parseSendableV2(text, undefined, opt1);
    assert_equal(obj1.big, BigInt("1122334455667788999"));
    assert_equal(typeof obj1.big, "bigint");
    assert_equal(obj1.small, 123);
    assert_equal(typeof obj1.small, "number");
    assert_equal(obj1.neg, -5);
    assert_equal(typeof obj1.neg, "number");
    assert_equal(obj1.zero, 0);
    assert_equal(typeof obj1.zero, "number");
    assert_equal(obj1.deci, 1.5);
    assert_equal(typeof obj1.deci, "number");
    assert_equal(JSON.stringifySendable(obj1), text);

    let opt2 = { bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT };
    let obj2 = JSON.parseSendableV2(text, undefined, opt2);
    assert_equal(obj2.small, BigInt("123"));
    assert_equal(typeof obj2.small, "bigint");
    assert_equal(obj2.zero, BigInt("0"));
    assert_equal(typeof obj2.zero, "bigint");
    assert_equal(obj2.deci, 1.5);
    assert_equal(typeof obj2.deci, "number");
    assert_equal(JSON.stringifySendable(obj2), text);

    let expText = '{"e":1e2,"d":1.5e3}';
    let obj3 = JSON.parseSendableV2(expText, undefined, opt2);
    assert_equal(obj3.e, BigInt("100"));
    assert_equal(typeof obj3.e, "bigint");
    assert_equal(obj3.d, BigInt("1500"));
    assert_equal(typeof obj3.d, "bigint");
}

function testParseSendableV2OptionsShape() {
    let text = '{"n":1122334455667788999,"m":5}';
    let invalidMode = JSON.parseSendableV2(text, undefined, { bigIntMode: 99 });
    assert_equal(typeof invalidMode.n, "number");
    assert_equal(typeof invalidMode.m, "number");
    let stringMode = JSON.parseSendableV2(text, undefined, { bigIntMode: "1" });
    assert_equal(typeof stringMode.n, "number");
    let nonObject = JSON.parseSendableV2(text, undefined, 5);
    assert_equal(typeof nonObject.n, "number");
    let nullOptions = JSON.parseSendableV2(text, undefined, null);
    assert_equal(typeof nullOptions.n, "number");
    let plain = JSON.parseSendableV2(text, undefined, undefined);
    assert_equal(typeof plain.n, "number");
    assert_equal(plain.m, 5);
}

function testParseSendableV2MapMode() {
    let opt = { parseReturnType: ParseReturnType.MAP };
    let map = JSON.parseSendableV2('{"a":1,"b":"x","c":true}', undefined, opt);
    assert_equal(map.size, 3);
    assert_equal(map.get("a"), 1);
    assert_equal(map.get("b"), "x");
    assert_equal(map.get("c"), true);
    assert_equal(map.get("notexist"), undefined);

    let nested = JSON.parseSendableV2('{"o":{"n":1},"arr":[1,2]}', undefined, opt);
    assert_equal(nested.get("o").get("n"), 1);
    assert_equal(JSON.stringifySendable(nested.get("arr")), "[1,2]");
    assert_equal(nested.get("o").size, 1);

    let emptyMap = JSON.parseSendableV2("{}", undefined, opt);
    assert_equal(emptyMap.size, 0);

    let topArray = JSON.parseSendableV2("[1,2,3]", undefined, opt);
    assert_equal(JSON.stringifySendable(topArray), "[1,2,3]");
    assert_equal(topArray[2], 3);

    let topNumber = JSON.parseSendableV2("42", undefined, opt);
    assert_equal(topNumber, 42);

    let dupMap = JSON.parseSendableV2('{"x":1,"x":2}', undefined, opt);
    assert_equal(dupMap.size, 1);
    assert_equal(dupMap.get("x"), 2);

    let combined = { bigIntMode: BigIntMode.PARSE_AS_BIGINT, parseReturnType: ParseReturnType.MAP };
    let bigMap = JSON.parseSendableV2('{"n":1122334455667788999}', undefined, combined);
    assert_equal(bigMap.get("n"), BigInt("1122334455667788999"));
    assert_equal(typeof bigMap.get("n"), "bigint");
}

function testParseSendableV2MapModeError() {
    let opt = { parseReturnType: ParseReturnType.MAP };
    try {
        JSON.parseSendableV2("{", undefined, opt);
        assert_unreachable();
    } catch (error) {
        assert_equal(error instanceof SyntaxError, true);
        assert_equal(error.message, "Unexpected MAP Prop in JSON");
    }
    try {
        JSON.parseSendableV2('{"city"}', undefined, opt);
        assert_unreachable();
    } catch (error) {
        assert_equal(error.message, "Unexpected MAP in JSON");
    }
    try {
        JSON.parseSendableV2('{"a":1,}', undefined, opt);
        assert_unreachable();
    } catch (error) {
        assert_equal(error.message, "Unexpected MAP Prop in JSON");
    }
}

function testParseSendableV2Reviver() {
    let obj = JSON.parseSendableV2('{"a":1}', undefined);
    assert_equal(obj.a, 1);
    let revivers = [
        function (k, v) { return v; },
        (k, v) => v,
        null,
        5,
        "reviver",
    ];
    for (let i = 0; i < revivers.length; i++) {
        try {
            JSON.parseSendableV2('{"a":1}', revivers[i]);
            assert_unreachable();
        } catch (error) {
            assert_equal(error instanceof TypeError, true);
            assert_equal(error.message, "reviver only supports undefined for SENDABLE_JSON");
        }
    }
}

function testParseSendableV2ToStringInput() {
    assert_equal(JSON.parseSendableV2(123), 123);
    assert_equal(JSON.parseSendableV2(-4.5), -4.5);
    assert_equal(JSON.parseSendableV2(null), null);
    assert_equal(JSON.parseSendableV2(true), true);
    assert_equal(JSON.parseSendableV2(false), false);
    try {
        JSON.parseSendableV2(undefined);
        assert_unreachable();
    } catch (error) {
        assert_equal(error.message, "Unexpected Text in JSON: Invalid Token");
    }
    try {
        JSON.parseSendableV2();
        assert_unreachable();
    } catch (error) {
        assert_equal(error instanceof SyntaxError, true);
        assert_equal(error.message, "arg is empty or arg more than three");
    }
    try {
        JSON.parseSendableV2('{"a":1}', undefined, undefined, undefined);
        assert_unreachable();
    } catch (error) {
        assert_equal(error.message, "arg is empty or arg more than three");
    }
    try {
        JSON.parseSendableV2({});
        assert_unreachable();
    } catch (error) {
        assert_equal(error.message, "Unexpected Text in JSON: Invalid Token");
    }
    try {
        JSON.parseSendableV2([]);
        assert_unreachable();
    } catch (error) {
        assert_equal(error.message, "Unexpected Text in JSON: Empty Text");
    }
}

function testParseSendableV2SyntaxErrors() {
    let badTexts = [
        ["", "Unexpected Text in JSON: Empty Text"],
        ["   ", "Unexpected end in JSON"],
        ["{", "Unexpected Object Prop in JSON"],
        ["}", "Unexpected Text in JSON: Invalid Token"],
        ["[", "Unexpected end in JSON"],
        ["[1,2", "Unexpected Number in JSON Array Or Object"],
        ['{"a"', "Unexpected end Text in JSON"],
        ['{"a":', "Unexpected end in JSON"],
        ['{"a":}', "Unexpected Text in JSON: Invalid Token"],
        ["{a:1}", "Unexpected Object Prop in JSON"],
        ["{'a':1}", "Unexpected Object Prop in JSON"],
        ['{"a" 1}', "Unexpected Object in JSON"],
        ['{"a":1,}', "Unexpected Object Prop in JSON"],
        ["[1,]", "Unexpected Text in JSON: Invalid Token"],
        ["[1 2]", "Unexpected Number in JSON Array Or Object"],
        ['{"a":1}{"b":2}', "Unexpected Text in JSON: Remaining Text Before Return"],
        ["123 456", "Unexpected Number in JSON"],
        ["tru", "Unexpected Text in JSON: ParseLiteralTrue Fail"],
        ["fals", "Unexpected Text in JSON: ParseLiteralFalse Fail"],
        ["nul", "Unexpected Text in JSON: ParseLiteralNull Fail"],
        ["truex", "Unexpected Text in JSON: Remaining Text Before Return"],
        ["nulll", "Unexpected Text in JSON: Remaining Text Before Return"],
        ["NaN", "Unexpected Text in JSON: Invalid Token"],
        ["Infinity", "Unexpected Text in JSON: Invalid Token"],
        ["-Infinity", "Unexpected Number in JSON"],
        ["undefined", "Unexpected Text in JSON: Invalid Token"],
        ["01", "Unexpected Number in JSON"],
        ["0x1", "Unexpected Number in JSON"],
        ["+1", "Unexpected Text in JSON: Invalid Token"],
        [".5", "Unexpected Text in JSON: Invalid Token"],
        ["1.", "Unexpected Number in JSON"],
        ["1e", "Unexpected Number in JSON"],
        ["1e+", "Unexpected Number in JSON"],
        ["-", "Unexpected Number in JSON"],
        ['"abc', "Unexpected end Text in JSON"],
        ['"\\u12"', "Unexpected string in JSON"],
        ['"a\tb"', "Unexpected end Text in JSON"],
    ];
    for (let i = 0; i < badTexts.length; i++) {
        try {
            JSON.parseSendableV2(badTexts[i][0]);
            assert_unreachable();
        } catch (error) {
            assert_equal(error instanceof SyntaxError, true);
            assert_equal(error.message, badTexts[i][1]);
        }
    }
}

function testParseSendableV2RoundTrip() {
    let text = '{"x":1,"y":"a","z":[1,{"w":true}],"v":null}';
    let shared = JSON.parseSendableV2(text);
    let sendableStr = JSON.stringifySendable(shared);
    assert_equal(sendableStr, text);
    let sharedAgain = JSON.parseSendableV2(sendableStr);
    assert_equal(JSON.stringifySendable(sharedAgain), text);
    let normal = JSON.parse(text);
    assert_equal(JSON.stringify(normal), text);
    assert_equal(JSON.stringifySendable(shared) === JSON.stringify(normal), true);

    let normalArr = JSON.parse("[1,2,3]");
    let sharedArr = JSON.parseSendableV2("[1,2,3]");
    assert_equal(JSON.stringify(normalArr), "[1,2,3]");
    assert_equal(JSON.stringifySendable(sharedArr), "[1,2,3]");
    assert_equal(JSON.stringify(sharedArr), "[1,2,3]");
}

function testParseSendableV2ModifyResult() {
    let obj = JSON.parseSendableV2('{"a":1,"b":2}');
    obj.a = 10;
    assert_equal(obj.a, 10);
    assert_equal(Object.isExtensible(obj), false);
    try {
        obj.c = 3;
        assert_unreachable();
    } catch (error) {
        assert_equal(error instanceof TypeError, true);
    }
    assert_equal(Object.keys(obj), ["a", "b"]);
    assert_equal(JSON.stringifySendable(obj), '{"a":10,"b":2}');
    let arr = JSON.parseSendableV2("[1,2,3]");
    arr[0] = 9;
    assert_equal(arr[0], 9);
    assert_equal(arr.length, 3);
    assert_equal(JSON.stringifySendable(arr), "[9,2,3]");
}

testParseSendableV2Primitives();
testParseSendableV2BasicObject();
testParseSendableV2EmptyObjectAndArray();
testParseSendableV2Nested();
testParseSendableV2Numbers();
testParseSendableV2Strings();
testParseSendableV2Whitespace();
testParseSendableV2DuplicateKeys();
testParseSendableV2SpecialKeys();
testParseSendableV2BigIntMode();
testParseSendableV2OptionsShape();
testParseSendableV2MapMode();
testParseSendableV2MapModeError();
testParseSendableV2Reviver();
testParseSendableV2ToStringInput();
testParseSendableV2SyntaxErrors();
testParseSendableV2RoundTrip();
testParseSendableV2ModifyResult();
test_end();
