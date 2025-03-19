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

declare function print(arg:any) : string;
declare interface ArkTools {
    timeInUs(arg:any):number
}

function testDefineProperty() {
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 100_000; i++) {
        const o = {};
        Object.defineProperty(o, "a", {
            value: 1,
            enumerable: false,
            configurable: false,
            writable: false,
            value: "static",
        });
        Object.defineProperty(o, "b", {
            value: 2,
            writable: true,
            enumerable: true,
            configurable: true,
        });
        Object.defineProperty(o, "c", {
            value: 3,
            writable: false,
        });
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Object DefineProperty:\t"+String(time)+"\tms");
}

function testHasOwnProperty() {
    const example = {};
    example.prop = "exists";
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        example.hasOwnProperty("prop");
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Object HasOwnProperty:\t"+String(time)+"\tms");
}

function testAssign() {
    let o1 = { a: 1 };
    let o2 = { b: 2 };
    let o3 = { c: 3 };
    let res;
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        res = Object.assign(o1, o2, o3);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print(res);
    print("Object Assign:\t"+String(time)+"\tms");
}

function testCreate() {
    const person = {
      isHuman: false,
      printIntroduction: function () {
      },
    };
    let res;
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        res = Object.create(person);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print(res);
    print("Object Create:\t"+String(time)+"\tms");
}

// Tests optimization of JSArray::CreateArrayFromList() in js_array.cpp
function testValues() {
    const REP_COUNT = 1_000_000;
    const obj = {};

    let start = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        Object.values(obj); // Triggers JSArray::CreateArrayFromList()
    }
    let end = ArkTools.timeInUs();
    print(`Object.values():\t${((end - start) / 1000).toFixed(3)}\tms`);
}

class Dog {
  constructor(name, breed, color, sex) {
    this.name = name;
    this.breed = breed;
    this.color = color;
    this.sex = sex;
  }
}

function testToString() {
    let theDog = new Dog("Gabby", "Lab", "chocolate", "female");
    let res;
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        res = theDog.toString();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print(res);
    print("Object ToString:\t"+String(time)+"\tms");
}

testDefineProperty();
testHasOwnProperty();
testAssign();
testCreate();
testToString();
testValues();
