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
 * @tc.name:storeicbyname
 * @tc.desc:test storeicbyname
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

var r = /./;
var coercibleValue = {
    length: 1,
    groups: {length : 1.1},
};
r.exec = function() {
    return coercibleValue;
};
let a = r[Symbol.replace]('', '[$<length>]');
assert_equal(a,"[1.1]")

let lineString1 = "这是一段lineString，A1这是替换的字符串A2"
let treeString1 = "这是一段treeString，".concat("B1这是替换的字符串B2")
let slicedString = "这是一段slicedString，C1这是要替换的字符串C2，xxxxxxxx".slice(0, 30);

var re1 = /[ABC]/;
var re2 = /[ABC]/g;

var res1 = lineString1.replace(re1, "X");
var res2 = lineString1.replace(re2, "X");
var res3 = treeString1.replace(re1, "X");
var res4 = treeString1.replace(re2, "X");
var res5 = slicedString.replace(re1, "X");
var res6 = slicedString.replace(re2, "X");

assert_equal(res1,"这是一段lineString，X1这是替换的字符串A2")
assert_equal(res2,"这是一段lineString，X1这是替换的字符串X2")
assert_equal(res3,"这是一段treeString，X1这是替换的字符串B2")
assert_equal(res4,"这是一段treeString，X1这是替换的字符串X2")
assert_equal(res5,"这是一段slicedString，X1这是要替换的字符串C2")
assert_equal(res6,"这是一段slicedString，X1这是要替换的字符串X2")

let lineString2 = "aaaaAbbbbBccccCaaaabbbbcccc"

function func1() {
    return "xxxxxxx".concat("yyyyyyyyy")
}
var res = lineString2.replace(re2, func1);
assert_equal(res,"aaaaxxxxxxxyyyyyyyyybbbbxxxxxxxyyyyyyyyyccccxxxxxxxyyyyyyyyyaaaabbbbcccc");

function func2() {
    return "xxxxxxx".concat("xxxxxxx")
}
res = lineString2.replace(re2, func2);
assert_equal(res,"aaaaxxxxxxxxxxxxxxbbbbxxxxxxxxxxxxxxccccxxxxxxxxxxxxxxaaaabbbbcccc");

function func3() {
    return "xxxxxxxxxxxxxx".concat("yyyyyyyy")
}
res = lineString2.replace(re2, func3);
assert_equal(res,"aaaaxxxxxxxxxxxxxxyyyyyyyybbbbxxxxxxxxxxxxxxyyyyyyyyccccxxxxxxxxxxxxxxyyyyyyyyaaaabbbbcccc");

function func4() {
    return "xxxxxxxxxxxxxx".concat("y")
}
res = lineString2.replace(re2, func4);
assert_equal(res,"aaaaxxxxxxxxxxxxxxybbbbxxxxxxxxxxxxxxyccccxxxxxxxxxxxxxxyaaaabbbbcccc");

function func5() {
    return "哈哈哈哈哈哈哈".concat("嘻嘻嘻嘻嘻嘻嘻嘻嘻")
}
res = lineString2.replace(re2, func5);
assert_equal(res,"aaaa哈哈哈哈哈哈哈嘻嘻嘻嘻嘻嘻嘻嘻嘻bbbb哈哈哈哈哈哈哈嘻嘻嘻嘻嘻嘻嘻嘻嘻cccc哈哈哈哈哈哈哈嘻嘻嘻嘻嘻嘻嘻嘻嘻aaaabbbbcccc")

function func6() {
    return "哈哈哈哈哈哈哈".concat("哈哈哈哈哈哈哈")
}
res = lineString2.replace(re2, func6);
assert_equal(res,"aaaa哈哈哈哈哈哈哈哈哈哈哈哈哈哈bbbb哈哈哈哈哈哈哈哈哈哈哈哈哈哈cccc哈哈哈哈哈哈哈哈哈哈哈哈哈哈aaaabbbbcccc")

function func7() {
    return "哈哈哈哈哈哈哈哈哈".concat("嘻嘻嘻嘻嘻嘻嘻")
}
res = lineString2.replace(re2, func7);
assert_equal(res,"aaaa哈哈哈哈哈哈哈哈哈嘻嘻嘻嘻嘻嘻嘻bbbb哈哈哈哈哈哈哈哈哈嘻嘻嘻嘻嘻嘻嘻cccc哈哈哈哈哈哈哈哈哈嘻嘻嘻嘻嘻嘻嘻aaaabbbbcccc")

function func8() {
    return "哈哈哈哈哈哈哈哈哈".concat("嘻")
}
res = lineString2.replace(re2, func8);
assert_equal(res,"aaaa哈哈哈哈哈哈哈哈哈嘻bbbb哈哈哈哈哈哈哈哈哈嘻cccc哈哈哈哈哈哈哈哈哈嘻aaaabbbbcccc")

const v3 = new BigUint64Array(128);
for (let v4 = 0; v4 < 5; v4++) {
    (v3 + 3).replace(/c(?<=a)8\P{sc=Greek}(a)/igus, 3);
}

let v5 = String.fromCharCode(0).replaceAll(/(?<a>.)/ug);
assert_equal("undefinedss".replace(undefined,v5),"undefinedss");
assert_equal("undefinedss".replaceAll(undefined,v5),"undefinedss");

test_end();