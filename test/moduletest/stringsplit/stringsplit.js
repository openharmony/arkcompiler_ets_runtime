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
    print(e);
}

// Test String.prototype.split and cache
const shortString = "ababaabcdefaaaaab";
const shortTwoBytesString = "\u0429\u0428\u0428\u0429\u0429\u0428\u0429\u0429\u0429";
const longString = new Array(200).fill("abcdef").join('');
const longTwoBytesString = new Array(200).fill("\u0426\u0427\u0428\u0429\u0430").join('');
let res1 = shortString.split('');
let res2 = shortString.split('');
print(res1.length)
print(res1.length == res2.length);
print(res1[0] == res2[0]);
let res3 = longString.split('');
let res4 = longString.split('');
print(res3.length)
print(res3.length == res4.length);
print(res3[0] == res4[0]);
let res5 = shortTwoBytesString.split('');
let res6 = shortTwoBytesString.split('');
print(res5.length)
print(res5.length == res6.length);
print(res5[0] == res6[0]);
let res7 = longTwoBytesString.split('');
let res8 = longTwoBytesString.split('');
print(res7.length)
print(res7.length == res8.length);
print(res7[0] == res8[0]);
let res9 = shortString.split('a');
let res10 = shortString.split('a');
print(res9.length)
print(res9.length == res10.length);
print(res9[0] == res10[0]);
let res11 = longString.split('a');
let res12 = longString.split('a');
print(res11.length)
print(res11.length == res12.length);
print(res11[0] == res12[0]);
let res13 = shortTwoBytesString.split('\u0429');
let res14 = shortTwoBytesString.split('\u0429');
print(res13.length)
print(res13.length == res14.length);
print(res13[0] == res14[0]);
let res15 = longTwoBytesString.split('\u0429');
let res16 = longTwoBytesString.split('\u0429');
print(res15.length)
print(res15.length == res16.length);
print(res15[0] == res16[0]);

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
print(resSplit);