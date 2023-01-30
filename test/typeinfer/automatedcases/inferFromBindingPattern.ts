/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/inferFromBindingPattern.ts ===
declare function AssertType(value:any, type:string):void;
declare function f1<T extends string>(): T;
declare function f2<T extends string>(): [T];
declare function f3<T extends string>(): { x: T };

let x1 = f1();         // string
AssertType(x1, "string");
AssertType(f1(), "string");
AssertType(f1, "<T extends string>() => T");

let [x2] = f2();       // string
AssertType(x2, "string");
AssertType(f2(), "[string]");
AssertType(f2, "<T extends string>() => [T]");

let { x: x3 } = f3();  // string
AssertType(x, "any");
AssertType(x3, "string");
AssertType(f3(), "{ x: string; }");
AssertType(f3, "<T extends string>() => { x: T; }");

// Repro from #30379

function foo<T = number>(): [T] {
AssertType([42 as any], "[any]");
AssertType(42 as any, "any");
AssertType(42, "int");
	return [42 as any]
}
const [x] = foo();  // [number]
AssertType(x, "number");
AssertType(foo(), "[number]");
AssertType(foo, "<T = number>() => [T]");

// Repro from #35291

interface SelectProps<T, K> {
  selector?: (obj: T) => K;
}

type SelectResult<T, K> = [K, T];

interface Person {
  name: string;
  surname: string;
}

declare function selectJohn<K = Person>(props?: SelectProps<Person, K>): SelectResult<Person, K>;

const [person] = selectJohn();
AssertType(person, "Person");
AssertType(selectJohn(), "SelectResult<Person, Person>");
AssertType(selectJohn, "<K = Person>(?union) => SelectResult<Person, K>");

const [any, whatever] = selectJohn();
AssertType(any, "Person");
AssertType(whatever, "Person");
AssertType(selectJohn(), "SelectResult<Person, Person>");
AssertType(selectJohn, "<K = Person>(?union) => SelectResult<Person, K>");

const john = selectJohn();
AssertType(john, "SelectResult<Person, Person>");
AssertType(selectJohn(), "SelectResult<Person, Person>");
AssertType(selectJohn, "<K = Person>(?union) => SelectResult<Person, K>");

const [personAgain, nufinspecial] = john;
AssertType(personAgain, "Person");
AssertType(nufinspecial, "Person");
AssertType(john, "SelectResult<Person, Person>");

// Repro from #35291

declare function makeTuple<T1>(arg: T1): [T1];
declare function stringy<T = string>(arg?: T): T;

const isStringTuple = makeTuple(stringy());  // [string]
AssertType(isStringTuple, "[string]");
AssertType(makeTuple(stringy()), "[string]");
AssertType(makeTuple, "<T1>(T1) => [T1]");
AssertType(stringy(), "string");
AssertType(stringy, "<T = string>(?union) => T");

const [isAny] = makeTuple(stringy());  // [string]
AssertType(isAny, "string");
AssertType(makeTuple(stringy()), "[string]");
AssertType(makeTuple, "<T1>(T1) => [T1]");
AssertType(stringy(), "string");
AssertType(stringy, "<T = string>(?union) => T");


