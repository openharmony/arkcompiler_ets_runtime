/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

// Test 1: Basic optimization - no replacer and gap (ReplacerAndGapUndefined = true)
{
    let obj = { a: 1, b: 2, c: 3 };
    assert_equal(JSON.stringify(obj), '{"a":1,"b":2,"c":3}');
}

{
    let arr = [1, 2, 3, 4, 5];
    assert_equal(JSON.stringify(arr), '[1,2,3,4,5]');
}

// Test 2: Nested objects with optimization
{
    let obj = {
        level1: {
            level2: {
                level3: {
                    value: 42
                }
            }
        }
    };
    assert_equal(JSON.stringify(obj), '{"level1":{"level2":{"level3":{"value":42}}}}');
}

// Test 3: Array with nested structures
{
    let arr = [
        { id: 1, name: "first" },
        { id: 2, name: "second" },
        { id: 3, name: "third" }
    ];
    assert_equal(JSON.stringify(arr), '[{"id":1,"name":"first"},{"id":2,"name":"second"},{"id":3,"name":"third"}]');
}

// Test 4: Key caching optimization - repeated keys
{
    let obj = {};
    let key1 = "repeatedKey";
    let key2 = "repeatedKey";
    obj[key1] = "value1";
    obj[key2] = "value2";
    assert_equal(JSON.stringify(obj), '{"repeatedKey":"value2"}');
}

// Test 5: Multiple identical keys in object literal
{
    let obj = {
        same: 1,
        same: 2,
        same: 3
    };
    assert_equal(JSON.stringify(obj), '{"same":3}');
}

// Test 6: Array with repeated values
{
    let arr = new Array(10);
    for (let i = 0; i < 10; i++) {
        arr[i] = i;
    }
    assert_equal(JSON.stringify(arr), '[0,1,2,3,4,5,6,7,8,9]');
}

// Test 7: String encoding optimization - ASCII only strings
{
    let obj = {
        a: "hello",
        b: "world",
        c: "test"
    };
    assert_equal(JSON.stringify(obj), '{"a":"hello","b":"world","c":"test"}');
}

// Test 8: Empty strings
{
    let obj = {
        empty: "",
        nullValue: null,
        undefined: undefined
    };
    assert_equal(JSON.stringify(obj), '{"empty":"","nullValue":null}');
}

// Test 9: Boolean and number values
{
    let obj = {
        boolTrue: true,
        boolFalse: false,
        zero: 0,
        negative: -42,
        float: 3.14159,
        infinity: Infinity,
        nan: NaN
    };
    assert_equal(JSON.stringify(obj), '{"boolTrue":true,"boolFalse":false,"zero":0,"negative":-42,"float":3.14159,"infinity":null,"nan":null}');
}

// Test 10: Special characters in strings
{
    let obj = {
        newline: "line1\nline2",
        tab: "col1\tcol2",
        quote: 'say "hello"',
        backslash: "path\\to\\file"
    };
    assert_equal(JSON.stringify(obj), '{"newline":"line1\\nline2","tab":"col1\\tcol2","quote":"say \\"hello\\"","backslash":"path\\\\to\\\\file"}');
}

// Test 11: Large number keys
{
    let obj = {};
    obj[4294967295] = "max uint32";
    obj[2147483648] = "over int32";
    obj[1000000000000] = "large number";
    let result = JSON.stringify(obj);
    assert_equal(result.includes('"4294967295"'), true);
    assert_equal(result.includes('"2147483648"'), true);
    assert_equal(result.includes('"1000000000000"'), true);
}

// Test 12: Array with holes (sparse array)
{
    let arr = [1, , 3, , 5, , 7];
    assert_equal(JSON.stringify(arr), '[1,null,3,null,5,null,7]');
}

// Test 13: Unicode and emoji
{
    let obj = {
        chinese: "你好世界",
        japanese: "こんにちは",
        emoji: "😀😁😂",
        mixed: "Hello世界🌍"
    };
    assert_equal(JSON.stringify(obj), '{"chinese":"你好世界","japanese":"こんにちは","emoji":"😀😁😂","mixed":"Hello世界🌍"}');
}

// Test 14: Surrogate pairs
{
    let str1 = "\uD83D\uDE00"; // 😀
    let str2 = "\uD83D\uDE01"; // 😁
    let obj = {
        emoji1: str1,
        emoji2: str2,
        combined: str1 + str2
    };
    assert_equal(JSON.stringify(obj), '{"emoji1":"😀","emoji2":"😁","combined":"😀😁"}');
}

// Test 15: Invalid surrogate pairs
{
    let str1 = "\uD83D"; // High surrogate without pair
    let str2 = "\uDE00"; // Low surrogate without pair
    let obj = {
        invalid1: str1,
        invalid2: str2,
        reversed: "\uDE00\uD83D" // Wrong order
    };
    assert_equal(JSON.stringify(obj), '{"invalid1":"\\ud83d","invalid2":"\\ude00","reversed":"\\ude00\\ud83d"}');
}

// Test 16: Array with replacer (ReplacerAndGapUndefined = false)
{
    let arr = [1, 2, 3, 4, 5];
    function replacer(key, value) {
        if (typeof value === 'number') {
            return value * 2;
        }
        return value;
    }
    assert_equal(JSON.stringify(arr, replacer), '[2,4,6,8,10]');
}

// Test 17: Object with array replacer
{
    let obj = {
        name: "test",
        age: 30,
        city: "Shanghai",
        country: "China"
    };
    assert_equal(JSON.stringify(obj, ["name", "country"]), '{"name":"test","country":"China"}');
}

// Test 18: Object with replacer function
{
    let obj = {
        a: 1,
        b: 2,
        c: 3
    };
    function replacer(key, value) {
        if (key === "b") {
            return undefined;
        }
        return value;
    }
    assert_equal(JSON.stringify(obj, replacer), '{"a":1,"c":3}');
}

// Test 20: Gap parameter - number
{
    let obj = {
        a: 1,
        b: {
            c: 2,
            d: 3
        }
    };
    let result = JSON.stringify(obj, null, 2);
    assert_equal(result.includes('  "a": 1'), true);
    assert_equal(result.includes('  "b": {'), true);
}

// Test 21: Gap parameter - string
{
    let obj = {
        a: 1,
        b: {
            c: 2
        }
    };
    let result = JSON.stringify(obj, null, "  ");
    assert_equal(result.includes('  "a": 1'), true);
}

// Test 22: Gap parameter - empty string
{
    let obj = { a: 1, b: 2 };
    assert_equal(JSON.stringify(obj, null, ""), '{"a":1,"b":2}');
}

// Test 23: Gap parameter - single space
{
    let obj = { a: 1, b: 2 };
    let result = JSON.stringify(obj, null, " ");
    assert_equal(result.includes(' "a": 1'), true);
}

// Test 24: Nested arrays
{
    let arr = [[1, 2], [3, 4], [5, 6]];
    assert_equal(JSON.stringify(arr), '[[1,2],[3,4],[5,6]]');
}

// Test 25: Array of arrays with different types
{
    let arr = [
        [1, 2, 3],
        ["a", "b", "c"],
        [true, false, null],
        [{ nested: "object" }]
    ];
    assert_equal(JSON.stringify(arr), '[[1,2,3],["a","b","c"],[true,false,null],[{"nested":"object"}]]');
}

// Test 26: toJSON method
{
    let obj = {
        a: 1,
        b: 2,
        toJSON: function(key) {
            return { custom: this.a + this.b };
        }
    };
    assert_equal(JSON.stringify(obj), '{"custom":3}');
}

// Test 27: Nested toJSON
{
    let obj = {
        x: 10,
        y: {
            z: 20,
            toJSON: function() {
                return this.z * 2;
            }
        }
    };
    assert_equal(JSON.stringify(obj), '{"x":10,"y":40}');
}

// Test 28: Array with toJSON
{
    let arr = [1, 2, 3];
    arr.toJSON = function() {
        return this.map(x => x * 10);
    };
    assert_equal(JSON.stringify(arr), '[10,20,30]');
}

// Test 29: Symbol keys (should be ignored)
{
    let sym = Symbol("test");
    let obj = {
        a: 1,
        [sym]: "symbol value",
        b: 2
    };
    assert_equal(JSON.stringify(obj), '{"a":1,"b":2}');
}

// Test 30: Symbol in array
{
    let arr = [1, Symbol("test"), 3];
    assert_equal(JSON.stringify(arr), '[1,null,3]');
}

// Test 31: BigInt (should throw)
{
    try {
        let obj = { big: 123n };
        JSON.stringify(obj);
        assert_unreachable();
    } catch (err) {
        assert_equal(err instanceof TypeError, true);
    }
}

// Test 32: Function values (should be ignored in objects)
{
    let obj = {
        a: 1,
        b: function() {},
        c: 3
    };
    assert_equal(JSON.stringify(obj), '{"a":1,"c":3}');
}

// Test 33: undefined in array
{
    let arr = [1, undefined, 3, undefined, 5];
    assert_equal(JSON.stringify(arr), '[1,null,3,null,5]');
}

// Test 34: null vs undefined
{
    let obj = {
        nullValue: null,
        undefinedValue: undefined
    };
    assert_equal(JSON.stringify(obj), '{"nullValue":null}');
}

// Test 35: Very long string
{
    let longStr = "a".repeat(1000);
    let obj = { long: longStr };
    let result = JSON.stringify(obj);
    assert_equal(result.includes('"long"'), true);
    assert_equal(result.includes('aaaaaaaaaa'), true);
    assert_equal(result.length > 1000, true);
}

// Test 36: Deep nesting (circular detection)
{
    try {
        let obj = {};
        obj.self = obj;
        JSON.stringify(obj);
        assert_unreachable();
    } catch (err) {
        assert_equal(err.name, 'TypeError');
        assert_equal(err.message.includes("circular"), true);
    }
}

// Test 37: Array circular reference
{
    try {
        let arr = [1, 2, 3];
        arr.push(arr);
        JSON.stringify(arr);
        assert_unreachable();
    } catch (err) {
        assert_equal(err.name, 'TypeError');
    }
}

// Test 38: Mixed circular reference
{
    try {
        let obj = { arr: [1, 2] };
        obj.arr.push(obj);
        JSON.stringify(obj);
        assert_unreachable();
    } catch (err) {
        assert_equal(err.name, 'TypeError');
    }
}

// Test 39: Getter that modifies object during serialization
{
    let obj = {
        get computed() {
            return 42;
        },
        normal: "value"
    };
    let result = JSON.stringify(obj);
    assert_equal(result.includes('"computed":42'), true);
    assert_equal(result.includes('"normal":"value"'), true);
}

// Test 40: Getter that deletes properties
{
    let obj = {
        get removable() {
            delete this.toDelete;
            return "kept";
        },
        toDelete: "should be removed",
        normal: "value"
    };
    assert_equal(JSON.stringify(obj), '{"removable":"kept","normal":"value"}');
}

// Test 41: Proxy handler
{
    let target = { a: 1, b: 2 };
    let handler = {
        get: function(obj, prop) {
            return prop in obj ? obj[prop] : "proxy value";
        }
    };
    let proxy = new Proxy(target, handler);
    assert_equal(JSON.stringify(proxy), '{"a":1,"b":2}');
}

// Test 42: Proxy that modifies during access
{
    let target = { a: 1 };
    let callCount = 0;
    let handler = {
        get: function(obj, prop) {
            callCount++;
            if (prop === "b" && callCount === 1) {
                obj[prop] = 2; // Add during access
            }
            return obj[prop];
        }
    };
    let proxy = new Proxy(target, handler);
    assert_equal(JSON.stringify(proxy), '{"a":1}');
}

// Test 43: Large array
{
    let arr = new Array(2000);
    for (let i = 0; i < 2000; i++) {
        arr[i] = i;
    }
    let result = JSON.stringify(arr);
    
    assert_equal(result.startsWith('['), true);
    assert_equal(result.endsWith(']'), true);
    assert_equal(result.includes(',999'), true);
}

// Test 44: Object with many properties
{
    let obj = {};
    for (let i = 0; i < 100; i++) {
        obj["prop" + i] = i;
    }
    assert_equal(JSON.stringify(obj).includes('"prop99":99'), true);
}

// Test 45: Typed arrays
{
    let uint8 = new Uint8Array([1, 2, 3, 4, 5]);
    assert_equal(JSON.stringify(uint8), '{"0":1,"1":2,"2":3,"3":4,"4":5}');
}

{
    let int16 = new Int16Array([-1, 0, 1, 32767, -32768]);
    assert_equal(JSON.stringify(int16), '{"0":-1,"1":0,"2":1,"3":32767,"4":-32768}');
}

// Test 46: ArrayBuffer
{
    let buffer = new ArrayBuffer(8);
    let view = new Uint8Array(buffer);
    view[0] = 1;
    view[1] = 2;
    assert_equal(JSON.stringify(buffer), '{}');
}

// Test 48: RegExp
{
    let regexp = /test/gi;
    assert_equal(JSON.stringify(regexp), '{}');
}

// Test 49: Map (if available)
{
    if (typeof Map !== "undefined") {
        let map = new Map();
        map.set("key1", "value1");
        map.set("key2", "value2");
        // Maps are serialized as empty objects in basic JSON.stringify
        assert_equal(JSON.stringify(map), '{}');
    }
}

// Test 50: Set (if available)
{
    if (typeof Set !== "undefined") {
        let set = new Set([1, 2, 3]);
        // Sets are serialized as empty objects in basic JSON.stringify
        assert_equal(JSON.stringify(set), '{}');
    }
}

// Test 51: Very large number values
{
    let obj = {
        maxSafe: Number.MAX_SAFE_INTEGER,
        minSafe: Number.MIN_SAFE_INTEGER,
        maxValue: Number.MAX_VALUE,
        epsilon: Number.EPSILON
    };
    let result = JSON.stringify(obj);
    assert_equal(result.includes('"maxSafe":9007199254740991'), true);
}

// Test 52: Negative zero
{
    let obj = {
        negZero: -0,
        posZero: 0
    };
    assert_equal(JSON.stringify(obj), '{"negZero":0,"posZero":0}');
}

// Test 53: String with null character
{
    let str = "hello\u0000world";
    assert_equal(JSON.stringify(str), '"hello\\u0000world"');
}

// Test 54: Object with numeric string keys
{
    let obj = {
        "1": "one",
        "2": "two",
        "10": "ten"
    };
    assert_equal(JSON.stringify(obj), '{"1":"one","2":"two","10":"ten"}');
}

// Test 55: Array with non-contiguous indices
{
    let arr = [];
    arr[0] = "zero";
    arr[2] = "two";
    arr[4] = "four";
    let result = JSON.stringify(arr);
    assert_equal(result, '["zero",null,"two",null,"four"]')
}

// Test 56: String with all control characters
{
    let str = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
    assert_equal(JSON.stringify(str).includes('\\u0000'), true);
}

// Test 57: Property descriptor in object
{
    let obj = {};
    Object.defineProperty(obj, "hidden", {
        value: 42,
        enumerable: false
    });
    Object.defineProperty(obj, "visible", {
        value: 100,
        enumerable: true
    });
    assert_equal(JSON.stringify(obj), '{"visible":100}');
}

// Test 58: Prototype chain properties
{
    let proto = { protoProp: "in prototype" };
    let obj = Object.create(proto);
    obj.ownProp = "own property";
    assert_equal(JSON.stringify(obj), '{"ownProp":"own property"}');
}

// Test 59: Array with custom length
{
    let arr = [1, 2, 3];
    arr.length = 5;
    assert_equal(JSON.stringify(arr), '[1,2,3,null,null]');
}

// Test 60: Object with numeric keys
{
    let obj = {};
    obj[0] = "zero";
    obj[1] = "one";
    obj[2] = "two";
    assert_equal(JSON.stringify(obj), '{"0":"zero","1":"one","2":"two"}');
}

// Test 61: Multiple toJSON calls in chain
{
    let obj = {
        a: 1,
        toJSON: function() {
            return { b: this.a * 2 };
        }
    };
    let wrapper = {
        obj: obj,
        toJSON: function() {
            return { result: this.obj };
        }
    };
    assert_equal(JSON.stringify(wrapper), '{"result":{"b":2}}');
}

// Test 62: Infinity and -Infinity
{
    let obj = {
        inf: Infinity,
        ninf: -Infinity,
        ninf2: Number.NEGATIVE_INFINITY
    };
    assert_equal(JSON.stringify(obj), '{"inf":null,"ninf":null,"ninf2":null}');
}

// Test 63: String with unicode escapes
{
    let str = "\x41\x42\x43"; // ABC
    assert_equal(JSON.stringify(str), '"ABC"');
}

// Test 64: Object with null prototype
{
    let obj = Object.create(null);
    obj.a = 1;
    obj.b = 2;
    assert_equal(JSON.stringify(obj), '{"a":1,"b":2}');
}

// Test 65: Array with negative indices (accessor)
{
    let arr = [1, 2, 3];
    // Negative indices on arrays are not part of the array in JSON serialization
    // This test verifies behavior with array properties
    Object.defineProperty(arr, -1, {
        get: function() { return "negative"; },
        enumerable: true
    });
    // Array indices must be non-negative integers
    assert_equal(JSON.stringify(arr), '[1,2,3]');
}

// Test 66: Very nested but valid structure
{
    let obj = { a: { b: { c: { d: { e: { f: { g: { h: { i: { j: "deep" } } } } } } } } } };
    assert_equal(JSON.stringify(obj), '{"a":{"b":{"c":{"d":{"e":{"f":{"g":{"h":{"i":{"j":"deep"}}}}}}}}}}');
}

// Test 67: Array with string indices
{
    let arr = [1, 2, 3];
    arr["str"] = "string index";
    // Arrays serialize only numeric indices in order
    assert_equal(JSON.stringify(arr), '[1,2,3]');
}

// Test 68: Object with constructor property
{
    let obj = {
        constructor: Array,
        a: 1
    };
    assert_equal(JSON.stringify(obj), '{"a":1}');
}

// Test 69: Boolean objects
{
    let obj = {
        bool1: new Boolean(true),
        bool2: new Boolean(false),
        num: new Number(42),
        str: new String("hello")
    };
    assert_equal(JSON.stringify(obj), '{"bool1":true,"bool2":false,"num":42,"str":"hello"}');
}

// Test 70: Array-like objects
{
    let obj = {
        0: "a",
        1: "b",
        2: "c",
        length: 3
    };
    assert_equal(JSON.stringify(obj), '{"0":"a","1":"b","2":"c","length":3}');
}

// Test 71: Many keys with doubles and escape characters
{
    let obj = {};
    for (let i = 0; i < 50; i++) {
        obj["key" + i] = i + 0.5;
    }
    obj["quote"] = 'say "hello"';
    obj["backslash"] = "path\\to\\file";
    obj["newline"] = "line1\nline2";
    obj["tab"] = "col1\tcol2";
    obj["normal"] = "no escape needed";
    let result = JSON.stringify(obj);
    assert_equal(result.includes('"key49":49.5'), true);
    assert_equal(result.includes('\\"hello\\"'), true);
    assert_equal(result.includes('\\\\to\\\\'), true);
    assert_equal(result.includes('\\n'), true);
    assert_equal(result.includes('\\t'), true);
    assert_equal(result.includes('"normal":"no escape needed"'), true);
}

// Test 72: Duplicate keys (later value wins)
{
    let obj = {
        same: "first",
        same: "second",
        same: "third",
        same: "final",
        other: "value"
    };
    assert_equal(obj.same, "final");
    assert_equal(JSON.stringify(obj), '{"same":"final","other":"value"}');
}

// Test 73: Mixed double values and special characters
{
    let obj = {
        pi: 3.14159265359,
        e: 2.71828182846,
        negative: -123.456,
        zeroPointFive: 0.5,
        large: 999999.999,
        small: 0.000001,
        special: "quotes \"here\"",
        path: "C:\\Users\\test",
        code: "if (a < b && c > d) {\n  return true;\n}"
    };
    let result = JSON.stringify(obj);
    let realResult = '{"pi":3.14159265359,"e":2.71828182846,"negative":-123.456,"zeroPointFive":0.5,"large":999999.999,"small":0.000001,"special":"quotes \\"here\\"","path":"C:\\\\Users\\\\test","code":"if (a < b && c > d) {\\n  return true;\\n}"}';
    assert_equal(result, realResult);
}

// Test 74: Many duplicate keys with different value types
{
    let obj = {
        name: "value1",
        name: 2.5,
        name: true,
        name: null,
        name: "string again",
        name: 100.25,
        data: "other data"
    };
    assert_equal(JSON.stringify(obj), '{"name":100.25,"data":"other data"}');
}

// Test 75: Keys with various escape scenarios
{
    let obj = {
        key1: "simple string",
        key2: "contains \"quotes\"",
        key3: "backslash\\here",
        key4: "line1\nline2\nline3",
        key5: "tab\tseparated\tvalues",
        key6: "quote \"and\" double quote",
        key7: "apostrophe's test",
        key8: 42.5,
        key9: 0.123456789,
        key10: "no special chars",
        key11: "\x00\x01\x02", // Control characters
        key12: -999.999
    };
    let result = JSON.stringify(obj);
    assert_equal(result.includes('"key2":"contains \\"quotes\\""'), true);
    assert_equal(result.includes('"key3":"backslash\\\\here"'), true);
    assert_equal(result.includes('\\n'), true);
    assert_equal(result.includes('\\t'), true);
    assert_equal(result.includes('"key8":42.5'), true);
    assert_equal(result.includes('"key9":0.123456789'), true);
    assert_equal(result.includes('"key10":"no special chars"'), true);
    assert_equal(result.includes('\\u0000'), true);
    assert_equal(result.includes('"key12":-999.999'), true);
}

// Test 76: Duplicate keys with doubles and escapes
{
    let obj = {
        key: "first",
        key: 3.14,
        key: "has \"quotes\"",
        key: 2.71828,
        key: "final \"value\"",
        other: 99.99
    };
    let result = JSON.stringify(obj);
    assert_equal(result, '{"key":"final \\"value\\"","other":99.99}');
}

// Test 77: Large object with mixed numeric values
{
    let obj = {};
    for (let i = 0; i < 10; i++) {
        if (i % 3 === 0) {
            obj["num" + i] = i;
        } else if (i % 3 === 1) {
            obj["num" + i] = i + 0.5;
        } else {
            obj["num" + i] = "string_" + i;
        }
    }
    obj["special"] = "escape: \"quotes\" and \\backslashes\\ and \n newlines";
    let result = JSON.stringify(obj);
    assert_equal(result, '{"num0":0,"num1":1.5,"num2":"string_2","num3":3,"num4":4.5,"num5":"string_5","num6":6,"num7":7.5,"num8":"string_8","num9":9,"special":"escape: \\\"quotes\\\" and \\\\backslashes\\\\ and \\n newlines"}');
}

// Test 78: Complete JSON_ESCAPE_LENGTH_TABLE coverage (0x00-0x5F)
// This test covers all entries in JSON_ESCAPE_LENGTH_TABLE table
{
    // Create string with all characters from 0x00 to 0x5F
    let allChars = "";
    for (let i = 0; i < 0x60; i++) {
        allChars += String.fromCharCode(i);
    }

    // Test with different escape scenarios
    let obj1 = {
        control_chars: allChars.substring(0, 32), // 0x00-0x1F
        special_quotes: "\"test\"",
        special_backslash: "\\test\\",
        space_and_punct: " !#$%&'()*+,-./",
        numbers: "0123456789:;<=>?",
        uppercase: "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        bracket_backslash: "[\\]^_",
        lowercase: "abcdefghijklmnopqrstuvwxyz",
        curly_backslash: "{|}~"
    };

    let result1 = JSON.stringify(obj1);

    // Verify all escape sequences are correct
    assert_equal(result1, '{"control_chars":"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f","special_quotes":"\\"test\\"","special_backslash":"\\\\test\\\\","space_and_punct":" !#$%&\'()*+,-./","numbers":"0123456789:;<=>?","uppercase":"ABCDEFGHIJKLMNOPQRSTUVWXYZ","bracket_backslash":"[\\\\]^_","lowercase":"abcdefghijklmnopqrstuvwxyz","curly_backslash":"{|}~"}');
}

// Test 79: Direct coverage of all JSON_ESCAPE_LENGTH_TABLE entries
{
    // Test all 96 entries (0x00-0x5F) with individual characters
    let testString = "";

    // 0x00-0x07: Control chars - length 6
    testString += "\x00\x01\x02\x03\x04\x05\x06\x07";

    // 0x08: backspace - length 2
    testString += "\x08";

    // 0x09: tab - length 2
    testString += "\x09";

    // 0x0A: newline - length 2
    testString += "\x0A";

    // 0x0B: vertical tab - length 6
    testString += "\x0B";

    // 0x0C: form feed - length 2
    testString += "\x0C";

    // 0x0D: carriage return - length 2
    testString += "\x0D";

    // 0x0E-0x1F: More control chars - length 6
    testString += "\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";

    // 0x20: space - length 1
    // 0x21: ! - length 1
    // 0x22: " - length 2
    testString += " !\"";

    // 0x23-0x2F: Various punctuation - length 1
    testString += "#$%&'()*+,-./";

    // 0x30-0x3F: Numbers and more punctuation - length 1
    testString += "0123456789:;<=>?";

    // 0x40: @ - length 1
    // 0x41-0x5B: A-Z - length 1
    // 0x5C: \ - length 2
    // 0x5D: ] - length 1
    // 0x5E: ^ - length 1
    // 0x5F: _ - length 1
    testString += "@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\]^_";

    // 0x60: ` - length 1
    testString += "`";

    let obj = { all_chars: testString };
    let result = JSON.stringify(obj);

    // Verify complete escape sequence
    assert_equal(result, '{"all_chars":"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f !\\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\\\]^_`"}');
}

{
    let obj = {}
    let obj2 = {}
    obj2['a'] = 1;
    for (let i = 0; i < 578; i++) {
        const key = String(i).padStart(8, '0');
        obj[key] = '';
    }
    const key580 = String(580).padStart(8, '0')
    obj[key580] = '12';
    obj['0a'] = obj2;
    let ss = "bcdef";
    for (let i = 0; i < 5; i++) {
        const key = String(ss[i]).padStart(1, '0');
        obj[key] = '\x00';
    }
    obj['a'] = "";
    let s = JSON.stringify(obj);
}

test_end();
