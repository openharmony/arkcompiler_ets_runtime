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

// === tests/cases/compiler/mappedTypeNestedGenericInstantiation.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #13346

interface Chainable<T> {
  value(): T;
  mapValues<U>(func: (v: T[keyof T]) => U): Chainable<{[k in keyof T]: U}>;
}

declare function chain<T>(t: T): Chainable<T>;

const square = (x: number) => x * x;
AssertType(square, "(number) => number");
AssertType((x: number) => x * x, "(number) => number");
AssertType(x, "number");
AssertType(x * x, "number");
AssertType(x, "number");
AssertType(x, "number");

const v = chain({a: 1, b: 2}).mapValues(square).value();
AssertType(v, "{ a: number; b: number; }");
AssertType(chain({a: 1, b: 2}).mapValues(square).value(), "{ a: number; b: number; }");
AssertType(chain({a: 1, b: 2}).mapValues(square).value, "() => { a: number; b: number; }");


