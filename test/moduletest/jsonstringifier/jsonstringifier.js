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

let str1="\uD83D";
let str2="\uDE0E";
let str3="\uDE0E\"测试";
let str4=new String("\uDE0E\"测试2")
let str=str1+str2
obj={};
obj[str1]=str1;
obj[str2]=str2;
obj[str3]=str3;
obj[str4]=str4;
obj[str]=str
print(JSON.stringify(obj))

print(JSON.stringify(str))
print(JSON.stringify(str1))
print(JSON.stringify(str2))
print(JSON.stringify(str3))
print(JSON.stringify(str4))

{
  var actual = [];
  var test_obj = {o: false};
  var replaced = {o: false, replaced: true};

  function replacer(key, value) {
      actual.push({ holder: this, key, value });
      if (actual.length === 1) return replaced;
      if (key === "o") return true;
      return value;
  }
  print(`{"o":true,"replaced":true}` == JSON.stringify(test_obj, replacer));
  const expect = [
      {
        holder: { "": { o: false } },
        key: "",
        value: { o: false }
      },
      {
        holder: { o: false, replaced: true },
        key: "o",
        value: false
      },
      {
        holder: { o: false, replaced: true },
        key: "replaced",
        value: true
      }
    ];
  print(JSON.stringify(expect) == JSON.stringify(actual));
  print(actual[0].holder[""] == test_obj);
};
{
  var actual = [];
  var test_obj = {o: false, toJSON };
  var nested = { toJSON: nestedToJSON };
  var toJSON1 = {o: false, toJSON1: true }
  var replaced = {o: false, replaced: true, nested };
  var toJSON2 = { toJSON2: true };

  function toJSON(key, value) {
    return toJSON1;
  }
  function nestedToJSON(key, value) {
    return toJSON2;
  }
  function replacer(key, value) {
      actual.push({ holder: this, key, value });
      if (actual.length === 1) return replaced;
      if (key === "o") return true;
      return value;
  }
  print(`{"o":true,"replaced":true,"nested":{"toJSON2":true}}` ==
              JSON.stringify(test_obj, replacer));
  const expect = [
      {
        holder: { "": { o: false, toJSON: toJSON } },
        key: "",
        value: { o: false, toJSON1: true }
      },
      {
        holder: { o: false, replaced: true, nested: { toJSON: nestedToJSON } },
        key: "o",
        value: false
      },
      {
        holder: { o: false, replaced: true, nested: { toJSON: nestedToJSON } },
        key: "replaced",
        value: true
      },
      {
        holder: { o: false, replaced: true, nested: { toJSON: nestedToJSON } },
        key: "nested",
        value: { toJSON2: true }
      },
      {
        holder: { toJSON2: true },
        key: "toJSON2",
        value: true
      }
  ];
  print(JSON.stringify(expect) == JSON.stringify(actual));
  print(actual[0].holder[""] == test_obj);
};
let obj1 = {
  get a(){
      this[102400] = 1;
      return "a";
  },
  b:"b",
}
Object.keys(obj1);
print(JSON.stringify(obj1));

try {
  let loop = {};
  loop.obj = loop;
  JSON.stringify(loop);
} catch (err) {
  print(err.name);
  print(err.message.includes("circular structure"));
}

try {
    let arkPrivate = globalThis.ArkPrivate;
    var List = arkPrivate.Load(arkPrivate.List);
    const v10 = new List();
    v10.add(v10);
    print(JSON.stringify(v10));
} catch (err) {
    print(err);
}
