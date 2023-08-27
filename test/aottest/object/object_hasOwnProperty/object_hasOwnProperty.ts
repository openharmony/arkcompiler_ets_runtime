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

print(dog1.hasOwnProperty("name"));
print(Object.prototype.hasOwnProperty.call(dog1, "name"));
print(Object.prototype.hasOwnProperty.call(dog1, "no"));

let s = "ss";
print(Object.prototype.hasOwnProperty.call(s, "length"));
print(Object.prototype.hasOwnProperty.call(s, 0));

let a = [1, 2, 3]
print(Object.prototype.hasOwnProperty.call(a, "length"));
print(a.hasOwnProperty("length"));
print(a.hasOwnProperty("0"));
print(a.hasOwnProperty(0));
