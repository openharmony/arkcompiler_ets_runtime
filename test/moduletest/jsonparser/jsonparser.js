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
print(json);
let json2 = JSON.parse("[       1       ]");
print(json2);
let json3 = JSON.parse("[              ]");
print(json3);
let data = {
    "11111111" : "https://www.a.com",
    "22222222" : "https://www.b.com",
    "00000000" : "https://www.c.com"
}
let strData = JSON.stringify(data);
let res = JSON.parse(strData);
print(res["11111111"]);
print(res["22222222"]);
print(res["00000000"]);

var a = `{"code": 0, "msg": "ok"}`
function reviver(k, v) { return v; }
var o = JSON.parse(a, reviver);
print(o);

let strData2 = "1.7976971348623157e+308";
let res2 = JSON.parse(strData2);
print(res2);

let strData3 = "-1.7976971348623157e+308";
let res3 = JSON.parse(strData3);
print(res3);

let strData4 = "123";
let res4 = JSON.parse(strData4);
print(res4);

try {
    print(JSON.parse(`{"object": 42, "test":{}`))
} catch (error) {
    print(error.name)
}

let strData5 = "\"\\uDC00\"";
let res5 = JSON.parse(strData5);
print(res5.codePointAt(0))

let strData6 = '{"a": "{\\"width\\": 18}"}'
print(JSON.stringify(JSON.parse(strData6)))

let strData7 = '{"a": "{\\"name\\": \\"张三\\"}"}'
print(JSON.stringify(JSON.parse(strData7)))

let strData8 = '{"1\\u0000":"name"}'
print(JSON.stringify(JSON.parse(strData8)))

print(JSON.parse('123.456e-789'));
print(1 / JSON.parse('-0'));

print("JSON.parse with backslash")
print(JSON.parse('"\\"\\""'));  // utf8 -> utf8
print(JSON.parse('"\\u0055\\u0066"'));  // utf8 -> utf8
print(JSON.parse('"\\u5555\\u6666"'));  // utf8 -> utf16
print(JSON.parse('["\\"\\"","中文"]')[0]);  // utf16 -> utf8
print(JSON.parse('["\\u0055\\u0066","中文"]')[0]);  // utf16 -> utf8
print(JSON.parse('["\\u5555\\u6666","中文"]')[0]);  // utf16 -> utf16

const strData9 = `{"k1":"hello","k2":3}`;
const strErr = strData9.substring(0, strData9.length - 2);
try {
    JSON.parse(strErr);
} catch (err) {
    print(err.name)
}

const strData10 = `{"k1":"hello","k2":                    3}`;
const strErr2 = strData10.substring(0, strData10.length - 2);
try {
    JSON.parse(strErr2);
} catch (err) {
    print(err.name)
}

const strData11 = `{"k1":"hello","k2":311111}`;
const strErr3 = strData11.substring(0, strData11.length - 2);
try {
    JSON.parse(strErr3);
} catch (err) {
    print(err.name)
}