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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithOverloadedFunctionTypedArguments.ts ===
declare function AssertType(value:any, type:string):void;
// Function typed arguments with multiple signatures must be passed an implementation that matches all of them
// Inferences are made quadratic-pairwise to and from these overload sets

module NonGenericParameter {
    let a: {
        (x: boolean): boolean;
        (x: string): string;
    }

    function foo4(cb: typeof a) {
AssertType(cb, "{ (boolean): boolean; (string): string; }");
        return cb;
    }

    let r = foo4(a);
    let r2 = foo4(<T>(x: T) => x);
    let r4 = foo4(x => x);    
}

module GenericParameter {
    function foo5<T>(cb: { (x: T): string; (x: number): T }) {
AssertType(cb, "{ (T): string; (number): T; }");
        return cb;
    }

    let r5 = foo5(x => x); // any => string (+1 overload) [inferences are made for T, but lambda not contextually typed]. T is any
    let a: { <T>(x: T): string; <T>(x: number): T; }
    let r7 = foo5(a); // any => string (+1 overload)

    function foo6<T>(cb: { (x: T): string; (x: T, y?: T): string }) {
AssertType(cb, "{ (T): string; (T, ?T): string; }");
        return cb;
    }

    let r8 = foo6(x => x); // any => string (+1 overload) [inferences are made for T, but lambda not contextually typed]. T is any
    let r9 = foo6(<T>(x: T) => ''); // any => string (+1 overload)
    let r11 = foo6(<T>(x: T, y?: T) => ''); // any => string (+1 overload)

    function foo7<T>(x:T, cb: { (x: T): string; (x: T, y?: T): string }) {
AssertType(cb, "{ (T): string; (T, ?T): string; }");
        return cb;
    }

    let r12 = foo7(1, (x) => x); // any => string (+1 overload) [inferences are made for T, but lambda not contextually typed]
    let r13 = foo7(1, <T>(x: T) => ''); // any => string (+1 overload) [inferences are made for T, but lambda not contextually typed]
    let a: { <T>(x: T): string; <T>(x: number): T; }
    let r14 = foo7(1, a); // any => string (+1 overload) [inferences are made for T, but lambda not contextually typed]
}

