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

/*
 * @tc.name:builtinsreflect
 * @tc.desc:test builtins reflect
 * @tc.type: FUNC
 * @tc.require: issueI8SXHD
 */
print("builtins reflect start");

// test1 -- reflect set length
var y = [];
Object.defineProperty(y, 1, { value: 42, configurable: false });
var tag1 = Reflect.set(y, 'length', 0);
var tag2 = Reflect.set(y, 'length', 5);
print(tag1);
print(tag2);

const v0 = 102630708;
let v55 = [];
let v56 = Object.create(v55)
Reflect.set(v56, "length", v0)
Reflect.set(v55, "length", v0, v56)
print("v56.length",v56.length)

var global = this;
var sym = Symbol("gaga");
var objects = [
    {},
    [],
    function() {},
    function() {
      return arguments;
    }(),
    Object(1),
    Object(true),
    Object('bla'),
    new Date,
    new RegExp,
    new Set,
    new Map,
    new WeakMap,
    new WeakSet,
    new ArrayBuffer(10),
    new Int32Array(5),
    Object,
    Function,
    Date,
    RegExp,
    global
];

function prepare(target) {
    target["bla"] = true;
    target[4] = 42;
    target[sym] = "foo";
    target["noconf"] = 43;
    Object.defineProperty(target, "noconf",
        { configurable: false });
    Object.defineProperty(target, "nowrite",
        { writable: false, configurable: true, value: 44 });
    Object.defineProperty(target, "getter",
        { get: function () {return this.bla}, configurable: true });
    Object.defineProperty(target, "setter",
        { set: function (x) {this.gaga = x}, configurable: true });
    Object.defineProperty(target, "setter2",
        { set: function (x) {}, configurable: true });
}

(function testReflectGetOnObject() {
    let i = 0;
    for (let target of objects) {
        prepare(target);
        if (true == Reflect.get(target, "bla") &&
            42 == Reflect.get(target, 4) &&
            42 == Reflect.get(target, "4") &&
            "foo" == Reflect.get(target, sym) &&
            43 == Reflect.get(target, "noconf") &&
            true == Reflect.get(target, "getter") &&
            undefined == Reflect.get(target, "setter") &&
            undefined == Reflect.get(target, "foo") &&
            undefined == Reflect.get(target, 333)) {
                print(i + "success_1");
        }
        let proto = target.__proto__;
        target.__proto__ = { get foo() {return this.bla} };
        if (true == Reflect.get(target, "foo")) {
            print(i + "success_2");
        }
        target.__proto__ = proto;
        i++;
    }
})();
let obj = {name:"tom"};
let pxobj = new Proxy(obj,{});
print(Reflect.get(pxobj,"name"))
print("builtins reflect end");
