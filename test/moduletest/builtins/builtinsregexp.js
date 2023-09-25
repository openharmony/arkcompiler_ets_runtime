/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:builtins
 * @tc.desc:test builtins regexp
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
print("builtins regexp start");

// Test1 - Regexp backward
const url = 'https://designcloud.uiplus.huawei.com/tool//materialServer/upload/images/20210608_5V0J5lVh4xVNYx0AUE.jpg';
const data = url.match(/(?<=\/)\w+(.jpg)$/);
print(data);

// Test 2 - RegExp $1 .. $9 is read only
try{
    RegExp.$1 = "b";
    print(RegExp.$1);
} catch(error) {
    print("RegExp $1 is read-only");
}

// Test 3 - RegExp $1 .. $9 match the lastest successful results
var reg = /t(e)(st(\d?))/g;
const str = "test1test2";
const array = [...str.matchAll(reg)];
print(RegExp.$2);

// Test 4 - RegExp $1..$9 reset all and refill the results
"abc".match(/(a)/);
print(RegExp.$1);
print(RegExp.$2);

// Test 5 - RegExp $10 is undefined
print(RegExp.$10);

// Test 6 - RegExp cache
var pattern = /[A-Z]{3}/g;
var testStr = "AAAA";
var res1 = testStr.match(pattern);
print(res1[0]);
res1[0] = "BB";
var res2 = testStr.match(pattern);
print(res2[0]);
res2[0] = "C";
var res3 = testStr.match(pattern);
print(res3[0]);

// Test 7 - Word Boundary test
var reg7 = /\/(?:\B)/gm;
var str7 = "/";
print(reg7.test(str7));

// Test 8 - RegExp.test $1
var regexp = /(\d+)/g
var str8 = "Test1234"
print(regexp.test(str8));
print(RegExp.$1);

print("builtins regexp end");
