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
 * @tc.name:primitiveic
 * @tc.desc:test primitiveic
 * @tc.type: FUNC
 */

// for number_ic
const numObj1 = 10928;
const numObj2 = 123.456;
const numObj3 = new Number(42);
for (let i = 0; i < 100; i++) {
   let res1 = numObj1.toString();
   let res2 = numObj2.toPrecision(4);
   let res3 = numObj3.valueOf();
}

// for number_ic & string_ic hasAccessor
{
  function foo() { return -4096;}
  Object.defineProperty(String.prototype, "a", {get:foo});
  Object.defineProperty(Number.prototype, "b", {get:foo});
  for (let i = 0; i < 50; i++)
  {
      const num = 123456;
      const str = "normallize";
      num.a;
      str.b;
  }
  Object.defineProperty(Number.prototype, "c", {value:"123"});
  for (let i = 0; i < 50; i++)
  {
      const num = 123456;
      const str = "normallize";
      num.a;
      str.b;
  }
}

// for number_ic & string_ic notHasAccessor
{
  Object.defineProperty(String.prototype, "d", {value:"456"});
  Object.defineProperty(Number.prototype, "e", {value:"789"});
  for (let i = 0; i < 50; i++)
  {
      const num = 123456;
      const str = "normallize";
      num.d;
      str.e;
  }
}

for(let i=0;i<2;i++){
  let obj={};
  let x=obj.__proto__;
  Object.freeze(x);
}

function f(a, b) {
  a.name;
}

for (let i = 0; i < 100; i++) {
  f(Number, 1);
  f(120, 1);
  f(Number, 1);
}

function f(a, b) {
  a.valueOf();
}

for (let i = 0; i < 100; i++) {
  f(Number.prototype, 1);
}
for (let i = 0; i < 100; i++) {
  f(120, 1);
}

const testProtoIcSuccessAssert = [];
const testProtoIcFailAssert = [
	"SendableFloat64Array",
	"SendableFloat32Array",
	"SendableInt32Array",
	"SendableInt16Array",
	"SendableInt8Array",
	"SendableUint32Array",
	"SendableUint16Array",
	"SendableUint8Array",
	"SendableUint8ClampedArray"
];
let testProtoIcSuccess = [];
let testProtoIcFail = [];

function testProtoIc(ctor) {
  for (let i = 0; i < 100; i++) { };
  let obj = new ctor(100);
  try {
    let obj1 = {
      __proto__: obj,
    };
    let obj2 = {
      __proto__: obj1,
    };
    let obj3 = {
      __proto__: obj2,
    };
    testProtoIcSuccess.push(ctor.name);
  } catch (err) {
    testProtoIcFail.push(ctor.name);
  }
}

[
  SendableFloat64Array,
  SendableFloat32Array,
  SendableInt32Array,
  SendableInt16Array,
  SendableInt8Array,
  SendableUint32Array,
  SendableUint16Array,
  SendableUint8Array,
  SendableUint8ClampedArray,
].forEach((ctor) => {
  testProtoIc(ctor);
});

assert_equal(testProtoIcSuccess,testProtoIcSuccessAssert)
assert_equal(testProtoIcFail,testProtoIcFailAssert)

function f(){return 1};
Object.defineProperty(this,"g",{
    get:f,
    set:f,
})
for(let i=0;i<2;i++){
  assert_equal(g,1)
}

function func1(o, v) {
  let  res;
  for (let i = 0; i < 100; i++) {
      res=o.x;
      if (res != v) {
      }
  }
  return res;
}
{
  let pro = {
      get x() {
          return 1;
      }
  }
  let o = {
      __proto__: pro
  };
  o[102500] = 1;
  o["test"] = "test";
  assert_equal(func1(o, 1),1);
  Object.defineProperty(o, "x", { value: 2 });
  assert_equal(func1(o, 2),2);
}

{
  let pro = {
      get x() {
          return 1;
      }
  }
  let pro2 = {
      __proto__: pro
  };
  let o = {
      __proto__: pro2
  };
  pro2[102500] = 1;
  pro2["test"] = "test";
  assert_equal(func1(o, 1),1);
  Object.defineProperty(pro2, "x", { value: 2 });
  func1(o, 2);
}

{
  function getNumber(o) {
      let res;
      for (let i = 0; i < 100; i++) {
        res=o.Number;
      }
      return res;
  }
  let pro = globalThis
  let pro2 = {
      __proto__: pro
  };
  let o = {
      __proto__: pro2
  };
  pro2[102500] = 1;
  pro2["test"] = "test";
  for (let i = 0; i < 2; i++) {
    assert_equal(getNumber(o).toString(),'function Number() { [native code] }')
  }
  Object.defineProperty(o, "Number", { value: 2 });

  for (let i = 0; i < 2; i++) {
    assert_equal(getNumber(o),2)
  }
}

test_end();