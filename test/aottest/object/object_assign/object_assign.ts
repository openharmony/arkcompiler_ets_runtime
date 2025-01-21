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
const obja = { a: 1 };
const copy = Object.assign({}, obja);
print(copy.a);

const o1 = { a: 1 };
const o2 = { b: 2 };
const o3 = { c: 3 };

const obj = Object.assign(o1, o2, o3);
print(obj.a + obj.b + obj.c)
const o4 = { a: 1, b: 1, c: 1 };
const o5 = { b: 2, c: 2 };
const o6 = { c: 3 };

const obj1 = Object.assign({}, o4, o5, o6, o6);
print(obj1.a + obj1.b + obj1.c);

const obj2 = Object.create(
  { foo: 1 },
  {
    bar: {
      value: 2,
    },
    baz: {
      value: 3,
      enumerable: true,
    },
  },
);

const copy1 = Object.assign({}, obj2);
print(copy1.baz);
print(copy1.hasOwnProperty("bar"));

const v1 = "abc";
const v2 = true;
const v3 = 10;
const v4 = Symbol("foo");

const obj3 = Object.assign({}, v1, null, v2, undefined, v3, v4);
print(Object.values(obj3));

const v5 = new Uint8Array();
const o7 = {
    __proto__ : v5,
};
const o8 = {
    get f() {
        return this;
    },
};
Object.assign(o7, o8);
print(o7.f === o8);
