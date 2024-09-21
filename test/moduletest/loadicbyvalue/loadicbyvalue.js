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
 * @tc.name:loadicbyvalue
 * @tc.desc:test loadicbyvalue
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var i = 3;
var obj = { 3: "icsuccess" };
function func1(a) {
    var b = a[i];
}
for (let j = 300; j > 0; j--) {
    func1(obj);
}
print(obj[i]);

// PoC testcase
const arr = []
for (let i = 0; i < 20; i++) {
    const v0 = "p" + i;
    const v1 = Symbol.iterator;
    const v2 = {
        [v1]() { },
    };
    arr[v0] = i;
}
const v3 = new Uint8Array(128);
v3[arr];
print("test successful !!!");

var obj1 = {
    0: 0,
    2147483647: 1,
    2147483648: 2,
    4294967295: 3,
    4294967296: 4,
}

for (let item in obj1) {
    print(item + " " + obj1[item]);
}
// test ic
var lineStr = "方舟ArK TypeScript RuntimekTS运行时（ARK TypeScript Runtime）K TypeScript Runtime是OppeScript RK TypeScripK TypeScript Runtimet Runtimeuntime）是OpeK TypeScript RuntimenHarmony上ArkTS应用使用的运行时。包含ArkTS/JS对象的分配enHarmonypeScript Runtime）是OpenHarmony上ArkTS应用使用的运行时。包含ArkTS/JS对象的分配上ArkTS应用使用peScript Runtime）是OpenHarmony上ArkTS应用使用的运行时。包含ArkTS/JS对象的分配的运行时。包含ArkTS/JS对象的分配器以及垃圾回收器（GC）、符合ECMAScript规范的标准库、用于运行ARK前端组件生成的方舟字节码（ARK Bytecode简称abc）的解释器、用于加速的内联缓存、静态类型编译器、运行时的C++/C函数接口（NAPI）等模块.";
var str = "方舟ArkTS运行时（ARK TypeScript Runtime）是OpenHarmony上ArkTS应用使用的运行时。包含ArkTS/JS对象的分配器以及垃圾回收器（GC）、符合ECMAScript规范的标准库、用于运行ARK前端组件生成的方舟字节码（ARK Bytecode简称abc）的解释器、用于加速的内联缓存、静态类型编译器、运行时的C++/C函数接口（NAPI）等模块.";
str += str;
str += str;
str += str;
var c;
var l;
var strC;
for (let i = 0; i < 100; i++) {
    strC = str.split("C");
    c = str[i];
    l = str.length;
    strC = lineStr.split("C");
    c = lineStr[i];
    l = lineStr.length;
}
var strObj = new String("方舟ArkTS运行时（ARK TypeScript Runtime）是OpenHarmony上ArkTS应用使用的运行时。包含ArkTS/JS对象的分配器以及垃圾回收器（GC）、符合ECMAScript规范的标准库、用于运行ARK前端组件生成的方舟字节码（ARK Bytecode简称abc）的解释器、用于加速的内联缓存、静态类型编译器、运行时的C++/C函数接口（NAPI）等模块.");
for (let i = 0; i < 100; i++) {
    strC = strObj.split("C");
    c = strObj[i];
    l = strObj.length;
}


for (let i = 0; i < 100; i++) {
    let arr = [0];
    arr[0] = arr;
}
print("load ic by COWArray Success")

function g(o, v) {
    let res;
    for (let i = 0; i < 100; i++) {
        res = o[1];
        if (res != v) {
            print("Error ",res);
        }
    }
    return res;
}
{
    let pro = [1,1,1,1];
    let o = { 
        __proto__: pro };
    o[102500] = 1;
    o["test"] = "test";
    print(g(o, 1));
    Object.defineProperty(o, "1", { value: 2 });
    print("change")
    print(g(o, 2));
}
{
    let pro = new Uint8Array(10);
    pro[1]=1;
    let o = { 
        __proto__: pro };
    o[102500] = 1;
    o["test"] = "test";
    print(g(o, 1));
    Object.defineProperty(o, "1", { value: 2 });
    print("change")
    print(g(o, 2));
}
print("ic load success");
