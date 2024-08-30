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
 * @tc.name:objoperate
 * @tc.desc:test object hasOwnProperty
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var str = "wodehaoxiongditiantiandouxuyao";
var str1 = "wodehaoxiongdi";
var str2 = "回家好好slsa";
var str3 = "jiarenmeng";
var str4 = str1 + str2;
var str5 = str1 + str3;
function fn() {
    this.大家 = "hao";
    this[str1] = "hao1";
    this[str2] = "hao2";
    this[str4] = "hao3";
    this[str5] = "hao4";
}

var obj = new fn();
var str6 = str.substring(0, 20);
obj[str6] = "hao5";
print(obj.hasOwnProperty("大家"));
print(obj.hasOwnProperty("wodehaoxiongdi"));
print(obj.hasOwnProperty("回家好好slsa"));
print(obj.hasOwnProperty("wodehaoxiongdi回家好好slsa"));
print(obj.hasOwnProperty("wodehaoxiongdijiarenmeng"));
print(obj.hasOwnProperty("wodehaoxiongditianti"));

var arr = new Array(4);
arr[0] = 3;
print(arr.hasOwnProperty("0"));
print(arr.hasOwnProperty("1"));
print(arr.hasOwnProperty("wodehaoxiongdi"));

Object.defineProperty(Array.prototype, "new1", {
    value:37,
    writable:false,
});

var arr1 = new Array(4);
print(arr1.new1);
print(arr1.hasOwnProperty("0"));
print(arr1.hasOwnProperty("new1"));

var k = 5;
print(str5.hasOwnProperty("t"));
print(str5.hasOwnProperty("wode"));
print(k.hasOwnProperty("t"));

var person = {name: "hhh"};
var proxy = new Proxy(person, {
    get: function(target, property) {
        if (property in target) {
            return target[property];
        } else {
            return "cuowu";
        }
    }
});
print(proxy.name);
print(proxy.age);
print(proxy.hasOwnProperty('name'));
print(proxy.hasOwnProperty('age'));

let obj2 = {};
obj2.property1 = 12;
// is not intern string branch and not found in intern string table
print(obj2.hasOwnProperty(String.fromCodePoint("")));
// is not intern string branch and found in intern string table
print(obj2.hasOwnProperty(String.fromCodePoint(123)));