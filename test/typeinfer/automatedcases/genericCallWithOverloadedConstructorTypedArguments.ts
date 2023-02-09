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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithOverloadedConstructorTypedArguments.ts ===
declare function AssertType(value:any, type:string):void;
// Function typed arguments with multiple signatures must be passed an implementation that matches all of them
// Inferences are made quadratic-pairwise to and from these overload sets

module NonGenericParameter {
    let a: {
        new(x: boolean): boolean;
        new(x: string): string;
    }

    function foo4(cb: typeof a) {
AssertType(new cb(null), "boolean");
AssertType(cb, "{ new (boolean): boolean; new (string): string; }");
AssertType(null, "null");
        return new cb(null);
    }

    let r = foo4(a);
    let b: { new <T>(x: T): T };
    let r2 = foo4(b);
}

module GenericParameter {
    function foo5<T>(cb: { new(x: T): string; new(x: number): T }) {
AssertType(cb, "{ new (T): string; new (number): T; }");
        return cb;
    }

    let a: {
        new (x: boolean): string;
        new (x: number): boolean;
    }
    let r5 = foo5(a); // new{} => string; new(x:number) => {}
    let b: { new<T>(x: T): string; new<T>(x: number): T; }
    let r7 = foo5(b); // new any => string; new(x:number) => any

    function foo6<T>(cb: { new(x: T): string; new(x: T, y?: T): string }) {
AssertType(cb, "{ new (T): string; new (T, ?T): string; }");
        return cb;
    }

    let r8 = foo6(a); // error
    let r9 = foo6(b); // new any => string; new(x:any, y?:any) => string

    function foo7<T>(x:T, cb: { new(x: T): string; new(x: T, y?: T): string }) {
AssertType(cb, "{ new (T): string; new (T, ?T): string; }");
        return cb;
    }

    let r13 = foo7(1, b); // new any => string; new(x:any, y?:any) => string
    let c: { new <T>(x: T): string; <T>(x: number): T; }
    let c2: { new <T>(x: T): string; new<T>(x: number): T; }
    let r14 = foo7(1, c); // new any => string; new(x:any, y?:any) => string
    let r15 = foo7(1, c2); // new any => string; new(x:any, y?:any) => string
}

