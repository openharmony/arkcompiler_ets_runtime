/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.desc:test object operate
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
function assertEqual(a, b) {
    var t1 = JSON.stringify(a);
    var t2 = JSON.stringify(b);
    if (t1 == t2) {
        print("PASS");
    } else {
        print("FAIL");
    }
}

var obj1 = {a:2, b:3, c:4};
var obj2 = {d:1, ...obj1, e:5};
assertEqual(obj2, {d:1, a:2, b:3, c:4, e:5});

var obj = {["a" + "b" + "de"]:function() {return 1;}}
assertEqual(obj.abde.name, "abde");

var foo = () => {
    function f1() {
        return this;
    }
    function f2() {
        return f1;
    }
    Object.defineProperty(this, "detailed", {configurable:true, enumerable:true, get:f1, set:f2});
};
foo();
foo();  // expect no error

var b = new ArrayBuffer(400);
var v1 = new Int32Array(b);
var str = '-' + '0';
var str1 = '4.' + '67';
var str2 = "jjj" + "kk";
print(v1[str2]);
v1[str2] = 5;
print(v1[str2]);


var obj1 = {};

obj1.__proto__ = v1;

print(obj1[str]);
obj1[str] = 5;
print(obj1[str]);

v1[4] = 123;
v1[5] = 23;
print(obj1[str1]);
obj1[str1] = 5;
print(obj1[str1]);

let key = 1 + {};
print("abc"[key])

let v3 = /a/;
let v4=v3.exec(v3);
for(let i=0;i<5;i++){
    v4[{}]=print;
}
print("set obj by name test success!")
