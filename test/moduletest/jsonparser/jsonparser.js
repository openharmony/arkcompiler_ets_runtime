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

test_end();