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

declare function assert_equal(a: Object, b: Object):void;

function foo(a:any, b:any) {
    return a + b
}

assert_equal(foo(1, 2), 3);

var bindfunc = foo.bind(null, 37);

assert_equal(bindfunc(5), 42)

const person = {
    name: 'Alice'
};

const greetPerson = function aaaaaa(message, message2, message3, message4) {
    print(message + ', ' + message2 + ', ' + message3 + ', ' + message4 + ', ' + this.name + '!');
}.bind(person, 'hello');
greetPerson();
assert_equal(greetPerson.name, "bound aaaaaa");
assert_equal(greetPerson.length, 3);

const greetPerson2 = greetPerson.bind(person, 'hello2');
greetPerson2();
assert_equal(greetPerson2.name, "bound bound aaaaaa");
assert_equal(greetPerson2.length, 2);

const greetPerson3 = greetPerson2.bind(greetPerson, 'hello3');
greetPerson3();
assert_equal(greetPerson3.name, "bound bound bound aaaaaa");
assert_equal(greetPerson3.length, 1);