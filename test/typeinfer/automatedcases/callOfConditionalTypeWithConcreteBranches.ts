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

// === tests/cases/compiler/callOfConditionalTypeWithConcreteBranches.ts ===
declare function AssertType(value:any, type:string):void;
type Q<T> = number extends T ? (n: number) => void : never;
function fn<T>(arg: Q<T>) {
  // Expected: OK
  // Actual: Cannot convert 10 to number & T
  arg(10);
AssertType(arg(10), "void");
AssertType(arg, "Q<T>");
AssertType(10, "int");
}
// Legal invocations are not problematic
fn<string | number>(m => m.toFixed());
AssertType(fn<string | number>(m => m.toFixed()), "void");
AssertType(fn, "<T>(Q<T>) => void");
AssertType(m => m.toFixed(), "(number) => string");
AssertType(m, "number");
AssertType(m.toFixed(), "string");
AssertType(m.toFixed, "(?number) => string");

fn<number>(m => m.toFixed());
AssertType(fn<number>(m => m.toFixed()), "void");
AssertType(fn, "<T>(Q<T>) => void");
AssertType(m => m.toFixed(), "(number) => string");
AssertType(m, "number");
AssertType(m.toFixed(), "string");
AssertType(m.toFixed, "(?number) => string");

// Ensure the following real-world example that relies on substitution still works
type ExtractParameters<T> = "parameters" extends keyof T
  // The above allows "parameters" to index `T` since all later
  // instances are actually implicitly `"parameters" & keyof T`
  ?  {
        [K in keyof T["parameters"]]: T["parameters"][K];
      }[keyof T["parameters"]]
  : {};

// Original example, but with inverted variance
type Q2<T> = number extends T ? (cb: (n: number) => void) => void : never;
function fn2<T>(arg: Q2<T>) {
  function useT(_arg: T): void {
AssertType(useT, "(T) => void");

AssertType(_arg, "T");
}

  // Expected: OK
  arg(arg => useT(arg));
AssertType(arg(arg => useT(arg)), "void");
AssertType(arg, "Q2<T>");
AssertType(arg => useT(arg), "(T & number) => void");
AssertType(arg, "T & number");
AssertType(useT(arg), "void");
AssertType(useT, "(T) => void");
AssertType(arg, "T & number");
}
// Legal invocations are not problematic
fn2<string | number>(m => m(42));
AssertType(fn2<string | number>(m => m(42)), "void");
AssertType(fn2, "<T>(Q2<T>) => void");
AssertType(m => m(42), "((number) => void) => void");
AssertType(m, "(number) => void");
AssertType(m(42), "void");
AssertType(m, "(number) => void");
AssertType(42, "int");

fn2<number>(m => m(42));
AssertType(fn2<number>(m => m(42)), "void");
AssertType(fn2, "<T>(Q2<T>) => void");
AssertType(m => m(42), "((number) => void) => void");
AssertType(m, "(number) => void");
AssertType(m(42), "void");
AssertType(m, "(number) => void");
AssertType(42, "int");

// webidl-conversions example where substituion must occur, despite contravariance of the position
// due to the invariant usage in `Parameters`

type X<V> = V extends (...args: any[]) => any ? (...args: Parameters<V>) => void : Function;

// vscode - another `Parameters` example
export type AddFirstParameterToFunctions<Target> = {
  [K in keyof Target]: Target[K] extends (...args: any[]) => void
      ? (...args: Parameters<Target[K]>) => void
      : void
};

