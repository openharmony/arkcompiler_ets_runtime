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
 * @tc.name:jsonstringifier
 * @tc.desc:test JSON.stringify
 * @tc.type: FUNC
 * @tc.require: issue#I7DFJC
 */

try {
    const v0 = [1, 2, 3, 4]
    function Foo(a) {
        Object.defineProperty(this, "ownKeys", { configurable: true, enumerable: true, value: a });
    }
    const v1 = new Foo("2060681564", v0, 9.53248718923);
    const v2 = new Proxy({}, v1);
    JSON.stringify(v2);
} catch (e) {
    print("test successful");
}

var obj = {
    2147483648: 2289
}
print(JSON.stringify(obj));

const a = new Uint32Array(0x10);
let  b = a.__proto__;
b[1073741823] = {}
print(JSON.stringify(a))

let o = {
    get g() {
        this[1225] |= 4294967295;    
        return 9;    
    },
    "f1":1,
    "f2":1,
    "f3":1,
    "f4":1,
    "f5":1,
    "f6":1,
    "f7":1,
    "f8":1,
}
print(JSON.stringify(o))
let o2 = {
    get g() {
        delete this.f1;
        return 8;    
    },
    "f1":1,
    "f2":1,
}
print(JSON.stringify(o2))
var handler2 = {
    get:function(target,name) {
        delete parent2.c;
        return name.toUpperCase();
    },
}
var proxy2 = new Proxy({},handler2);
var parent2 = {a:proxy2,c:"remove"};
print(JSON.stringify(parent2))

var obj={
    get 1() {
        delete this['2'];
    },
    2:2,
    3:3,
}
print(JSON.stringify(obj))

var List = undefined;
var LinkedList = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    List = ArkPrivate.Load(ArkPrivate.List);
    let list = new List();
    list.add({"f1": 1});
    list.add({"f2": 2});
    print(JSON.stringify(list));

    LinkedList = ArkPrivate.Load(ArkPrivate.LinkedList);
    let linkList = new LinkedList();
    linkList.add({"f3": 3});
    linkList.add({"f4": 4});
    print(JSON.stringify(linkList));
}

var v6="123456789\u0000";
print(JSON.stringify([{}],[String],v6))

var handler2 = {
  get: function(target, name) {
    delete parent2.c;
    return name.toUpperCase();
  }
}
var proxy2 = new Proxy({}, handler2);
var parent2 = { a: "delete", b: proxy2, c: "remove" };
print(JSON.stringify(parent2))
parent2.c = "remove";  // Revert side effect.
print(JSON.stringify(parent2))
Reflect.defineProperty(globalThis,"c",{
    get:()=>{
        delete this["d"];
        return "c";
    },
    enumerable:true,
});
Reflect.set(globalThis,"d","d");
JSON.stringify(globalThis);
print("end JSON.stringify(globalThis)")
