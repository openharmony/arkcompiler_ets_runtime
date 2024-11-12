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

// enable ic
var a = 0
for (var i = 0; i < 10000; i++) {
    a = a + a;
}

// case 1
// test toString in holder.
function t() {}
var I = t.prototype
I.toInt = function() {}
I.toNumber = function() {}
I.toString = function() {}
print(Object.getOwnPropertyDescriptor(I, 'toString').enumerable)

const o6 = {
  ..."function",
}
print(Object.getOwnPropertyDescriptor(o6, 0).configurable)

try {
  const v2 = ("string").match(ArrayBuffer)
  class C4 extends Array {
    constructor(a6, a7) {
      super()
      try {
        this.concat(ArrayBuffer, v2, ArrayBuffer);
        return v2
      } catch (e) {}
    }
  }
  new C4(C4, C4);
} catch(e) {}

let arr = new Int8Array(10)
for (let v5 =0; v5 < 400;v5++) {
}
Object.defineProperty(arr,"a", {
       set(x) {
           Reflect.deleteProperty(arr , "b")
           return "a";
       } 
})
arr["b"] = "b"
for (let i = 0; i < 1; ++i) {
    arr["a"] = 1;
    print(arr["a"])
}

let obj = {
  get a() {
      Reflect.deleteProperty(this, "b")
      print("get")
      return "a";
  },
  b:"b"
}

for (let v5 =0;v5 < 400; v5++) {
}

for (let i = 0; i <2; i++) {
  obj["a"]
}

let arr2 = {}
for (let v5 =0; v5 < 400;v5++) {

}
Object.defineProperty(arr2,"a", {
    set(x) {
           Reflect.deleteProperty(this , "b")
           return "a";
    }   
})
arr2["b"] = "b"

for(let i = 0; i <i ;++i) {
    arr2["a"] = 1;
}
print(arr2["a"])

let v1 = 0;
class c0 {
    set y(v3) {}
}

class c1 extends c0 {
    set y(v4) {
        if (v1++ === 29) {
            delete c1.prototype.y;
        }
    }
}

class c2 extends c1 {}
let v2 = new c2();
for (let v5 = 0;v5 < 2; v5++) {
    print(v5);
    v2.y = 42;
}

function fun1(n) {
    function fun2(o) {
        return o.a;
    }
    let obj = {a: 1};
    for (let i = 0; i < n; i++) {
        fun2(obj);
    }
    return fun2(obj);
}

print(fun1(100));
print(fun1(1));

const v0 = [4.0, 3.0, -1.7976931348623157e+308, 1000000000.0];
const v3 = { __proto__: v0 };
const v4 = { __proto__: v3 };
v4[3] = v4;
v0[1] = v0;
v3.splice(0, 0);
print(v3.length);

class C3 {
  constructor() {
      for (let i = 0; i < 1000; i++) {}
      let v6 = new Proxy(this, {
          defineProperty: () => {}
      });
      v6.h = 4;
      print("v6");
  }
}
new C3(); 