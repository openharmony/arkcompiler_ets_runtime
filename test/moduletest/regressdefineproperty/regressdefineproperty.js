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
let a = [];
Object.defineProperty(a, "length", {writable: false});
function f() {
  return a.pop();
}
try{
    f();
} catch (e) {
    print(e instanceof TypeError)
}

function f1() {
    let x = [0,0,0,0,0];
    Object.defineProperty(x, 'length', {value : 4, enumerable : true});
}
try{
    f1();
} catch (e) {
    print(e instanceof TypeError)
}

let array = [];
Object.defineProperty(array, 'length', {writable: false});
print(array.length);
try {
    array.shift()
} catch (e) {
   print(e instanceof TypeError)
}
let object = { length: 0 };
Object.defineProperty(object, 'length', {writable: false});
print(object.length);
try {
    Array.prototype.shift.call(object)
} catch {
    print("true")
}

Object.defineProperty(this, 'x', {
  configurable: true,
  get: function () {return 100}
});
Object.defineProperty(this, 'x', {
  value: 10
});
print(JSON.stringify(Object.getOwnPropertyDescriptor(this, 'x')));

const o1 = {
  k: 1
};
for (let i = 0; i < 1100; i++) {
  Object.defineProperty(o1, "k" + i, {
    value: 0,
    enumerable: false
  });
}
print(JSON.stringify(o1))

function fn() { };
let v0 = function fn1() { }.bind(fn);
Object.defineProperty(v0, "length", {
  writable: true
})
v0.length = 42;
print(v0.length)
