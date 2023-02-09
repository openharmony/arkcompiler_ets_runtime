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

// === tests/cases/compiler/returnTypeParameterWithModules.ts ===
declare function AssertType(value:any, type:string):void;
module M1 {
    export function reduce<A>(ar, f, e?): Array<A> {
AssertType(Array.prototype.reduce.apply(ar, e ? [f, e] : [f]), "any");
AssertType(Array.prototype.reduce.apply, "(Function, any, ?any) => any");
AssertType(Array.prototype.reduce, "{ ((any, any, number, any[]) => any): any; ((any, any, number, any[]) => any, any): any; <U>((U, any, number, any[]) => U, U): U; }");
AssertType(Array.prototype, "any[]");
AssertType(ar, "any");
AssertType(e ? [f, e] : [f], "any[]");
AssertType(e, "any");
AssertType([f, e], "any[]");
AssertType(f, "any");
AssertType(e, "any");
AssertType([f], "any[]");
AssertType(f, "any");
        return Array.prototype.reduce.apply(ar, e ? [f, e] : [f]);

    };
};
module M2 {
  import A = M1
  export function compose() {
        A.reduce(arguments, compose2);
AssertType(A.reduce(arguments, compose2), "unknown[]");
AssertType(A.reduce, "<A>(any, any, ?any) => A[]");
AssertType(arguments, "IArguments");
AssertType(compose2, "<B, C, D>((B) => C, (D) => B) => (D) => C");

    };
    export function compose2<B, C, D>(g: (x: B) => C, f: (x: D) => B): (x: D) => C {
AssertType(function (x) { return g(f(x)); }, "(D) => C");
    return function (x) { return g(f(x)); 

AssertType(x, "D");

AssertType(g(f(x)), "C");

AssertType(g, "(B) => C");

AssertType(f(x), "B");

AssertType(f, "(D) => B");

AssertType(x, "D");
}

  };
};

