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

declare function print(arg: any): string;

function foo() {
    let intArray: number[] = [1, 2, 3];
    print(intArray[0]);

    let doubleArray: number[] = [1.1, 2.2, 3.3];
    print(doubleArray[1]);

    let objectArray: object[] = [{}, {}, {}];
    print(objectArray[1]);

    let holeyArray: (number | undefined | object)[] = [1, , {}];
    print(holeyArray[1]);
}

foo();


class C {
    x: number;
    constructor(v: number) {
        this.x = v;
    }
}

function testObject() {
    let objArray: C[] = [new C(1), new C(2)];
    for (let i: number = 0; i < objArray.length; ++i) {
        print(objArray[i].x);
    }
}

testObject();

class OrderedCollection {
    elms: number[];
    constructor() {
      this.elms = [];
    }
    add(elm: number):void {
      this.elms.push(elm);
    }

    at(index: number):number{
      return this.elms[index];
    }
}

let constraints: OrderedCollection = new OrderedCollection();
constraints.add(0);
constraints.add(1);
constraints.add(2);
print(constraints.at(0));

function testReverse() {
  const v0 = [100, 65536]
  v0.reverse();
  v0[1] = v0;
  v0.reverse();
  print("success");
}

testReverse();

function testArrayUsedAsProto() {
  let v0 = [1];
  let o = {
    __proto__: v0
  }
  v0.length = 0;
  print("testArrayUsedAsProto success")
}

testArrayUsedAsProto();

function testProto1() {
  let a = [1, 2, 3];
  let obj = {};
  obj.__proto__ = a;
  print(a[0]);
  print(ArkTools.getElementsKind(a));
}

print(ArkTools.isAOTCompiled(testProto1));
testProto1();


function testProto2() {
  let obj = {};
  for (let i = 0; i < 4; i++) {
    let a = [1, 2, 3];
    if (i == 2) {
      obj.__proto__ = a;
      a[1] = 1.5
    }
    print(a[0] + 1);
    print(ArkTools.getElementsKind(a));
  }
}

print(ArkTools.isAOTCompiled(testProto2));
testProto2();
