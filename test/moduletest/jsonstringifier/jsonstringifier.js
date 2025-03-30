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
    assert_unreachable();
} catch (e) {
  assert_equal(true,true);
}

var obj = {
    2147483648: 2289
}
assert_equal(JSON.stringify(obj),'{"2147483648":2289}');

const a = new Uint32Array(0x10);
let  b = a.__proto__;
b[1073741823] = {}
assert_equal(JSON.stringify(a),'{"0":0,"1":0,"2":0,"3":0,"4":0,"5":0,"6":0,"7":0,"8":0,"9":0,"10":0,"11":0,"12":0,"13":0,"14":0,"15":0}')

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
assert_equal(JSON.stringify(o),'{"g":9,"f1":1,"f2":1,"f3":1,"f4":1,"f5":1,"f6":1,"f7":1,"f8":1}')
let o2 = {
    get g() {
        delete this.f1;
        return 8;    
    },
    "f1":1,
    "f2":1,
}
assert_equal(JSON.stringify(o2),'{"g":8,"f2":1}')
var handler2 = {
    get:function(target,name) {
        delete parent2.c;
        return name.toUpperCase();
    },
}
var proxy2 = new Proxy({},handler2);
var parent2 = {a:proxy2,c:"remove"};
assert_equal(JSON.stringify(parent2),'{"a":{}}')

var obj={
    get 1() {
        delete this['2'];
    },
    2:2,
    3:3,
}
assert_equal(JSON.stringify(obj),'{"3":3}')

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
assert_equal(JSON.stringify([{}]),'[{}]')
assert_equal(JSON.stringify([String]),'[null]')
assert_equal(JSON.stringify(v6),'"123456789\\u0000"')


var handler2 = {
  get: function(target, name) {
    delete parent2.c;
    return name.toUpperCase();
  }
}
var proxy2 = new Proxy({}, handler2);
var parent2 = { a: "delete", b: proxy2, c: "remove" };
assert_equal(JSON.stringify(parent2),'{"a":"delete","b":{}}')
parent2.c = "remove";  // Revert side effect.
assert_equal(JSON.stringify(parent2),'{"a":"delete","b":{}}')
Reflect.defineProperty(globalThis,"c",{
    get:()=>{
        delete this["d"];
        return "c";
    },
    enumerable:true,
});
Reflect.set(globalThis,"d","d");
JSON.stringify(globalThis);

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
assert_equal(JSON.stringify(obj),'{"\\ud83d":"\\ud83d","\\ude0e":"\\ude0e","\\ude0e\\"测试":"\\ude0e\\"测试","\\ude0e\\"测试2":"\\ude0e\\"测试2","😎":"😎"}')

assert_equal(JSON.stringify(str),'"😎"')
assert_equal(JSON.stringify(str1),'"\\ud83d"')
assert_equal(JSON.stringify(str2),'"\\ude0e"')
assert_equal(JSON.stringify(str3),'"\\ude0e\\"测试"')
assert_equal(JSON.stringify(str4),'"\\ude0e\\"测试2"')

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
  assert_equal(`{"o":true,"replaced":true}` == JSON.stringify(test_obj, replacer),true);
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
  assert_equal(JSON.stringify(expect) == JSON.stringify(actual),true);
  assert_equal(actual[0].holder[""] == test_obj,true);
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
  assert_equal(`{"o":true,"replaced":true,"nested":{"toJSON2":true}}` ==
            JSON.stringify(test_obj, replacer),true);
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
  assert_equal(JSON.stringify(expect) == JSON.stringify(actual),true);
  assert_equal(actual[0].holder[""] == test_obj,true);
};
let obj1 = {
  get a(){
      this[102400] = 1;
      return "a";
  },
  b:"b",
}
Object.keys(obj1);
assert_equal(JSON.stringify(obj1),'{"a":"a","b":"b"}');

try {
  let loop = {};
  loop.obj = loop;
  JSON.stringify(loop);
  assert_unreachable();
} catch (err) {
  assert_equal(err.name,'TypeError');
  assert_equal(err.message.includes("circular structure"),true);
}

const specialString = '"Hello\nWorld\\tThis is a test string with special characters: \u00A9 \u00AE "';
const nestedObj = {
  level1: {
    level2: {
      level3: {
        message: specialString,
        number: 42,
        truthy: true,
        anotherObj: {
          key: 'value'
        },
        array: [1, 'two', false, { nested: 'object' }]
      }
    }
  }
};
assert_equal((JSON.stringify(nestedObj, null, 2)).substring(2, 5), '  "');
assert_equal((JSON.stringify(nestedObj, null, " ")).substring(2, 4), ' "');

try {
    let arkPrivate = globalThis.ArkPrivate;
    var List = arkPrivate.Load(arkPrivate.List);
    const v10 = new List();
    v10.add(v10);
    JSON.stringify(v10);
    assert_unreachable();
} catch (err) {
    assert_equal("Caught an error: "+ err, "Caught an error: TypeError: stack contains value, usually caused by circular structure");
}

const testspaceobj = {
  name: "abc",
  age: 123,
  city: "Shanghai"
};
const testspaceobjAssert = JSON.stringify(testspaceobj, null, Infinity);
assert_equal(JSON.stringify(testspaceobj, null, Infinity),testspaceobjAssert);

{
    let obj = {};
    let tmp = obj;
    for (let i = 0; i < 5000; i++) {
        tmp.name = {};
        tmp = tmp.name;
    }
    try {
        let res = JSON.stringify(obj);
        assert_unreachable();
    } catch (error) {
        assert_equal(error instanceof RangeError, true);
    }
}

{
  let obj = {};
  for (let i = 0; i < 9; i++) {
      obj["k" + i] = "value" + i;
  }
  let exceptStr1 = '{"k0":"value0","k1":"value1","k2":"value2","k3":"value3","k4":"value4","k5":"value5","k6":"value6","k7":"value7","k8":"value8"}';
  assert_equal(JSON.stringify(obj), exceptStr1);

  for (let i = 9; i < 20; i++) {
      obj["k" + i] = "value" + i;
  }
  let exceptStr2 = '{"k0":"value0","k1":"value1","k2":"value2","k3":"value3","k4":"value4","k5":"value5","k6":"value6","k7":"value7","k8":"value8","k9":"value9","k10":"value10","k11":"value11","k12":"value12","k13":"value13","k14":"value14","k15":"value15","k16":"value16","k17":"value17","k18":"value18","k19":"value19"}';
  assert_equal(JSON.stringify(obj), exceptStr2);
}

test_end();