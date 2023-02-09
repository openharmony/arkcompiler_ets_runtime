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

// === tests/cases/conformance/types/namedTypes/genericInstantiationEquivalentToObjectLiteral.ts ===
declare function AssertType(value:any, type:string):void;
interface Pair<T1, T2> { first: T1; second: T2; }
let x: Pair<string, number>
AssertType(x, "Pair<string, number>");

let y: { first: string; second: number; 
AssertType(y, "{ first: string; second: number; }");

AssertType(first, "string");

AssertType(second, "number");
}

x = y;
AssertType(x = y, "{ first: string; second: number; }");
AssertType(x, "Pair<string, number>");
AssertType(y, "{ first: string; second: number; }");

y = x;
AssertType(y = x, "Pair<string, number>");
AssertType(y, "{ first: string; second: number; }");
AssertType(x, "Pair<string, number>");

declare function f<T, U>(x: Pair<T, U>);
declare function f2<T, U>(x: { first: T; second: U; });

f(x);
AssertType(f(x), "any");
AssertType(f, "<T, U>(Pair<T, U>) => any");
AssertType(x, "Pair<string, number>");

f(y);
AssertType(f(y), "any");
AssertType(f, "<T, U>(Pair<T, U>) => any");
AssertType(y, "{ first: string; second: number; }");

f2(x);
AssertType(f2(x), "any");
AssertType(f2, "<T, U>({ first: T; second: U; }) => any");
AssertType(x, "Pair<string, number>");

f2(y);
AssertType(f2(y), "any");
AssertType(f2, "<T, U>({ first: T; second: U; }) => any");
AssertType(y, "{ first: string; second: number; }");


