/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:jsonparser
 * @tc.desc:test Json.parse
 * @tc.type: FUNC
 * @tc.require: issue#I6BFOC
 */

let json = JSON.parse("[ 1, 2, 3]");
assert_equal(json,[1,2,3]);
let json2 = JSON.parse("[       1       ]");
assert_equal(json2,[1]);
let json3 = JSON.parse("[              ]");
assert_equal(json3,[]);
let data = {
    "11111111" : "https://www.a.com",
    "22222222" : "https://www.b.com",
    "00000000" : "https://www.c.com"
}
let strData = JSON.stringify(data);
let res = JSON.parse(strData);
assert_equal(res["11111111"],'https://www.a.com');
assert_equal(res["22222222"],'https://www.b.com');
assert_equal(res["00000000"],'https://www.c.com');

var a = `{"code": 0, "msg": "ok"}`
function reviver(k, v) { return v; }
var o = JSON.parse(a, reviver);
assert_equal(o.toString(),'[object Object]');

let strData2 = "1.7976971348623157e+308";
let res2 = JSON.parse(strData2);
assert_equal(res2,Infinity);

let strData3 = "-1.7976971348623157e+308";
let res3 = JSON.parse(strData3);
assert_equal(res3,-Infinity);

let strData4 = "123";
let res4 = JSON.parse(strData4);
assert_equal(res4,123);

try {
    JSON.parse(`{"object": 42, "test":{}`)
} catch (error) {
    assert_equal(error.name,'SyntaxError')
    
}

let strData5 = "\"\\uDC00\"";
let res5 = JSON.parse(strData5);
assert_equal(res5.codePointAt(0),56320)

let strData6 = '{"a": "{\\"width\\": 18}"}'
assert_equal(JSON.stringify(JSON.parse(strData6)),'{"a":"{\\"width\\": 18}"}')

let strData7 = '{"a": "{\\"name\\": \\"张三\\"}"}'
assert_equal(JSON.stringify(JSON.parse(strData7)),'{"a":"{\\"name\\": \\"张三\\"}"}')

let strData8 = '{"1\\u0000":"name"}'
assert_equal(JSON.stringify(JSON.parse(strData8)),'{"1\\u0000":"name"}')

assert_equal(JSON.parse('123.456e-789'),0);
assert_equal(1 / JSON.parse('-0'),-Infinity);

var string = "JSON.parse with backslash";
assert_equal(string,"JSON.parse with backslash");
assert_equal(JSON.parse('"\\"\\""'),'""');  // utf8 -> utf8
assert_equal(JSON.parse('"\\u0055\\u0066"'),'Uf');  // utf8 -> utf8
assert_equal(JSON.parse('"\\u5555\\u6666"'),'啕晦');  // utf8 -> utf16
assert_equal(JSON.parse('["\\"\\"","中文"]')[0],'""');  // utf16 -> utf8
assert_equal(JSON.parse('["\\u0055\\u0066","中文"]')[0],'Uf');  // utf16 -> utf8
assert_equal(JSON.parse('["\\u5555\\u6666","中文"]')[0],'啕晦');  // utf16 -> utf16

const strData9 = `{"k1":"hello","k2":3}`;
const strErr = strData9.substring(0, strData9.length - 2);
try {
    JSON.parse(strErr);
} catch (err) {
    assert_equal(err.name,'SyntaxError');
}

const strData10 = `{"k1":"hello","k2":                    3}`;
const strErr2 = strData10.substring(0, strData10.length - 2);
try {
    JSON.parse(strErr2);
} catch (err) {
    assert_equal(err.name,'SyntaxError');
}

const strData11 = `{"k1":"hello","k2":311111}`;
const strErr3 = strData11.substring(0, strData11.length - 2);
try {
    JSON.parse(strErr3);
} catch (err) {
    assert_equal(err.name,'SyntaxError');
}

let jsonSingleStr = `{
    "a": 10,
    "b": "hello",
    "c": true,
    "d": null,
	"e": 5,
	"f": 6,
	"g": 7,
	"h": 8,
	"i": 9,
	"j": 10,
	"k": 11,
	"l": 12,
	"m": 13,
	"n": 14,
	"o": 15,
	"p": 16,
	"q": 17,
	"r": 18,
	"s": 19,
	"t": 20,
	"u": 21,
	"v": 22,
	"w": 23,
	"x": 24,
	"y": 25,
	"z": 26,
	"A": 27,
	"B": 28,
	"C": 29,
	"D": 30,
	"E": 31,
	"F": 32,
	"G": 33,
	"H": 34,
	"I": 35,
	"J": 36,
	"K": 37,
	"L": 38,
	"M": 39,
	"N": 40,
	"O": 41,
	"P": 42,
	"Q": 43,
	"R": 44,
	"S": 45,
	"T": 46,
	"U": 47,
	"V": 48,
	"W": 49,
	"X": 50,
	"Y": 51,
	"Z": 52
}`;

let parsedObj = JSON.parse(jsonSingleStr);
assert_equal(parsedObj.a, 10);
assert_equal(parsedObj.b, 'hello');
assert_equal(parsedObj.c, true);
assert_equal(parsedObj.n, 14);
assert_equal(parsedObj.x, 24);
assert_equal(parsedObj.C, 29);
assert_equal(parsedObj.J, 36);
assert_equal(parsedObj.T, 46);

let numStr = `{
    "numberval1": 1.79e+308,
    "numberval2": 1.7976931348623158e+309,
    "numberval3": 5e+320,
    "numberval4": 2.225e-308,
	"numberval5": 2.225e-309,
	"numberval6": 3e-320,
	"numberval7": 5e-324,
	"numberval8": 5e-325,
	"numberval9": 7e-350
}`;
let numParsedObj = JSON.parse(numStr);
assert_equal(numParsedObj.numberval1, 1.79e+308);// DBL_MAX
assert_equal(numParsedObj.numberval2, Infinity);// greater than DBL_MAX, expect Infinity
assert_equal(numParsedObj.numberval3, Infinity);// greater than DBL_MAX, expect Infinity
assert_equal(numParsedObj.numberval4, 2.225e-308);// DBL_MIN
assert_equal(numParsedObj.numberval5, 2.225e-309);// less than DBL_MIN
assert_equal(numParsedObj.numberval6, 3e-320);// less than DBL_MIN
assert_equal(numParsedObj.numberval7, 5e-324);// Number.MIN_VALUE
assert_equal(numParsedObj.numberval8, 0);// less than Number.MIN_VALUE, expect 0
assert_equal(numParsedObj.numberval9, 0);// less than Number.MIN_VALUE, expect 0

{
	let err = {};
	try {
		JSON.parse(`{"object哈哈": 42, "test":{}`)
	} catch (error) {
		err = error;
	}
	assert_equal(err.name, 'SyntaxError');
}

{
	const v12 = new Uint16Array(4);
	const v13 = new Uint16Array(2);
	v12.toJSON = v13;
	function f14() {
		const v14 = new Uint16Array(3);
		this.toJSON = v14;
	}
	const v21 = JSON.stringify(v12);
	JSON.parse(v21, f14);
}
{
  // Case 1: Unquoted string keys
  try {
    JSON.parse('{name: "John"}');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 2: Trailing comma in object
  try {
    JSON.parse('{"a": 1,}');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 3: Trailing comma in array
  try {
    JSON.parse('[1, 2,]');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 4: Unescaped control characters
  try {
    JSON.parse('"\n"');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 5: Single quotes instead of double quotes
  try {
    JSON.parse("{'key': 'value'}");
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 6: Hexadecimal numbers (not supported in JSON)
  try {
    JSON.parse('0x1A');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 7: Unclosed string
  try {
    JSON.parse('"unclosed string');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 8: Unclosed array
  try {
    JSON.parse('[1, 2,');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 9: Unclosed object
  try {
    JSON.parse('{"a": 1');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 10: Leading zeros in numbers
  try {
    JSON.parse('0123');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 11: Multiple decimal points
  try {
    JSON.parse('123.45.67');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 12: Invalid escape character
  try {
    JSON.parse('"\\x"');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 13: Unescaped tab character
  try {
    JSON.parse('"\t"');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 14: Invalid Unicode escape
  try {
    JSON.parse('"\\uZZZZ"');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Case 15: Incomplete Unicode escape
  try {
    JSON.parse('"\\u123"');
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError, true);
  }
}

{
  // Parse basic types - string, number, boolean, null
  const json = '{"str": "hello", "num": 42, "bool": true, "null": null}';
  const result = JSON.parse(json);
  assert_equal(result.str, "hello");
  assert_equal(result.num, 42);
  assert_equal(result.bool, true);
  assert_equal(result.null, null);
}

{
  // Parse nested objects and arrays
  const json = '{"user": {"name": "John", "age": 30}, "tags": ["js", "json", "test"]}';
  const result = JSON.parse(json);
  assert_equal(result.user.name, "John");
  assert_equal(result.user.age, 30);
  assert_equal(Array.isArray(result.tags), true);
  assert_equal(result.tags.length, 3);
  assert_equal(result.tags[0], "js");
  assert_equal(result.tags[1], "json");
  assert_equal(result.tags[2], "test");
}

{
  // Parse special characters and escaped sequences
  const json = '{"message": "Line 1\\nLine 2\\tTab\\\\Backslash\\"Quote\\""}';
  const result = JSON.parse(json);
  assert_equal(result.message, "Line 1\nLine 2\tTab\\Backslash\"Quote\"");
}

{
  // Parse numbers with different formats (integer, float, negative, scientific notation)
  const json = '{"int": 123, "float": 3.14159, "negative": -42, "sci": 1.23e4}';
  const result = JSON.parse(json);
  assert_equal(result.int, 123);
  assert_equal(result.float, 3.14159);
  assert_equal(result.negative, -42);
  assert_equal(result.sci, 12300);
}

{
  // Parse empty structures and complex nested combinations
  const json = '{"emptyObj": {}, "emptyArr": [], "mixed": [null, true, 1, "text", {"key": "value"}]}';
  const result = JSON.parse(json);
  assert_equal(Object.keys(result.emptyObj).length, 0);
  assert_equal(result.emptyArr.length, 0);
  assert_equal(result.mixed[0], null);
  assert_equal(result.mixed[1], true);
  assert_equal(result.mixed[2], 1);
  assert_equal(result.mixed[3], "text");
  assert_equal(result.mixed[4].key, "value");
}
test_end();
