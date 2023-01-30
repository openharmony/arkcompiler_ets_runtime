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

// === tests/cases/compiler/enumLiteralUnionNotWidened.ts ===
declare function AssertType(value:any, type:string):void;
// repro from #22093
enum A { one = "one", two = "two" };
enum B { foo = "foo", bar = "bar" };

type C = A | B.foo;
type D = A | "foo";

class List<T>
{
	private readonly items: T[] = [];
}

function asList<T>(arg: T): List<T> { 
AssertType(new List(), "List<T>");
return new List(); 

AssertType(List, "typeof List");
}

// TypeScript incorrectly infers the return type of "asList(x)" to be "List<A | B>"
// The correct type is "List<A | B.foo>"
function fn1(x: C): List<C> { 
AssertType(asList(x), "List<C>");
return asList(x); 

AssertType(asList, "<T>(T) => List<T>");

AssertType(x, "C");
}

// If we use the literal "foo" instead of B.foo, the correct type is inferred
function fn2(x: D): List<D> { 
AssertType(asList(x), "List<D>");
return asList(x); 

AssertType(asList, "<T>(T) => List<T>");

AssertType(x, "D");
}


