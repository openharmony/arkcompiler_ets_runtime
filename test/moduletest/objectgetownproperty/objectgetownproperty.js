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
function* g() {}
print(Object.getOwnPropertyDescriptor(g.__proto__, "constructor").configurable);
print(Object.getOwnPropertyDescriptor(g.prototype.__proto__, "constructor").configurable);

function FakeGeneratorFunctionConstructor() {}
Object.defineProperty(g.__proto__, "constructor", {value: FakeGeneratorFunctionConstructor});
print(g.__proto__.constructor == FakeGeneratorFunctionConstructor);

function FakeGeneratorObjectConstructor() {}
Object.defineProperty(g.prototype.__proto__, "constructor", {value: FakeGeneratorObjectConstructor});
print(g.prototype.__proto__.constructor == FakeGeneratorObjectConstructor);

const obj = {
    name: '小明',
    age: 20,
    get fullName() {
      return this.name;
    }
  };

const descriptors = Object.getOwnPropertyDescriptors(obj);
print(descriptors.name.value === '小明');
print(descriptors.age.value === 20);
print(typeof descriptors.fullName.get === 'function');
print(descriptors);
