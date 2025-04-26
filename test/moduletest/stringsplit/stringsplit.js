/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name:split
 * @tc.desc:test String.split
 * @tc.type: FUNC
 * @tc.require: issueI8SXEG
 */

try {
    ("X").split("00QP", -32297n);
} catch (e) {
    assert_equal(e instanceof TypeError, true);
}

// Test String.prototype.split and cache
const shortString = "ababaabcdefaaaaab";
const shortTwoBytesString = "\u0429\u0428\u0428\u0429\u0429\u0428\u0429\u0429\u0429";
const longString = new Array(200).fill("abcdef").join('');
const longTwoBytesString = new Array(200).fill("\u0426\u0427\u0428\u0429\u0430").join('');
let res1 = shortString.split('');
let res2 = shortString.split('');
assert_equal(res1.length,17)
assert_equal(res1.length == res2.length,true);
assert_equal(res1[0] == res2[0],true);
let res3 = longString.split('');
let res4 = longString.split('');
assert_equal(res3.length,1200)
assert_equal(res3.length == res4.length,true);
assert_equal(res3[0] == res4[0],true);
let res5 = shortTwoBytesString.split('');
let res6 = shortTwoBytesString.split('');
assert_equal(res5.length,9)
assert_equal(res5.length == res6.length,true);
assert_equal(res5[0] == res6[0],true);
let res7 = longTwoBytesString.split('');
let res8 = longTwoBytesString.split('');
assert_equal(res7.length,1000)
assert_equal(res7.length == res8.length,true);
assert_equal(res7[0] == res8[0],true);
let res9 = shortString.split('a');
let res10 = shortString.split('a');
assert_equal(res9.length,10)
assert_equal(res9.length == res10.length,true);
assert_equal(res9[0] == res10[0],true);
let res11 = longString.split('a');
let res12 = longString.split('a');
assert_equal(res11.length,201)
assert_equal(res11.length == res12.length,true);
assert_equal(res11[0] == res12[0],true);
let res13 = shortTwoBytesString.split('\u0429');
let res14 = shortTwoBytesString.split('\u0429');
assert_equal(res13.length,7)
assert_equal(res13.length == res14.length,true);
assert_equal(res13[0] == res14[0],true);
let res15 = longTwoBytesString.split('\u0429');
let res16 = longTwoBytesString.split('\u0429');
assert_equal(res15.length,201)
assert_equal(res15.length == res16.length,true);
assert_equal(res15[0] == res16[0],true);

// Test split string is tree string
var a = "12345678910"
var b = "12345678910"
var c = a.concat(b);
c.split("")

// Test split string is sliced string
var d = a.slice(4)
d.split("")

let str = "dcidmcdififf";
for (let i = 0; i < 10; i++) {
    str.split('d');
}
let resSplit = str.split('d');
print(resSplit,["","ci","mc","ififf"]);

// Test split string cache
let str2 = "Apple,Banana-Cherry|Date;Fig Grape.Honeydew|Kiwi,Lemon Mango-Orange|Pear.Quince,Raspberry-Strawberry.";
let resSplit2 = str2.split(",");
let resSplit2Cache = str2.split(",");
assert_equal(resSplit2, resSplit2Cache);

let resSplit3 = str2.split("|");
let resSplit3Cache = str2.split("|");
assert_equal(resSplit3, resSplit3Cache);

let resSplit4 = str2.split("");
let resSplit4Cache = str2.split("");
assert_equal(resSplit4, resSplit4Cache);

test_end();