/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

function foo(a:any, b:any) {
    return a + b
}

print(foo(1, 2));

var bindfunc = foo.bind(null, 37);

print(bindfunc(5));

const person = {
    name: 'Alice'
};

const greetPerson = function aaaaaa(message, message2, message3, message4) {
    print(message + ', ' + message2 + ', ' + message3 + ', ' + message4 + ', ' + this.name + '!');
}.bind(person, 'hello');
greetPerson();
print(greetPerson.name);
print(greetPerson.length);

const greetPerson2 = greetPerson.bind(person, 'hello2');
greetPerson2();
print(greetPerson2.name);
print(greetPerson2.length);

const greetPerson3 = greetPerson2.bind(greetPerson, 'hello3');
greetPerson3();
print(greetPerson3.name);
print(greetPerson3.length);