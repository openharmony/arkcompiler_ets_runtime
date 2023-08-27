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

declare function print(arg:any):string;
function Dog(name) {
  this.name = name;
}

const dog1 = new Dog('Gabby');

Dog.prototype.toString = function dogToString() {
  return `${this.name}`;
};

print(dog1.toString());
print(Object.prototype.toString.call(dog1));

let o = new Object();
print(o.toString())

let n = new Number(1);
print(Object.prototype.toString.call(n));

let s = "ss";
print(Object.prototype.toString.call(s));
print(s.toString());

print(Object.prototype.toString.call(new Date()));
print(Object.prototype.toString.call(new String()));
// Math has its Symbol.toStringTag
print(Object.prototype.toString.call(Math));


print(Object.prototype.toString.call(null));
print(Object.prototype.toString.call());

const myDate = new Date();
print(Object.prototype.toString.call(myDate));

myDate[Symbol.toStringTag] = "myDate";
print(Object.prototype.toString.call(myDate));

Date.prototype[Symbol.toStringTag] = "prototype polluted";
print(Object.prototype.toString.call(new Date()));
