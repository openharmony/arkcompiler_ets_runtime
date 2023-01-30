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

// === tests/cases/compiler/commentsExternalModules_0.ts ===
declare function AssertType(value:any, type:string):void;
/** Module comment*/
export module m1 {
    /** b's comment*/
    export let b: number;
    /** foo's comment*/
    function foo() {
AssertType(b, "number");
        return b;
    }
    /** m2 comments*/
    export module m2 {
        /** class comment;*/
        export class c {
        };
        /** i*/
        export let i = new c();
    }
    /** exported function*/
    export function fooExport() {
AssertType(foo(), "number");
AssertType(foo, "() => number");
        return foo();
    }
}
m1.fooExport();
AssertType(m1.fooExport(), "number");
AssertType(m1.fooExport, "() => number");

let mylet = new m1.m2.c();
AssertType(mylet, "m1.m2.c");
AssertType(new m1.m2.c(), "m1.m2.c");
AssertType(m1.m2.c, "typeof m1.m2.c");

/** Module comment */
export module m4 {
    /** b's comment */
    export let b: number;
    /** foo's comment
    */
    function foo() {
AssertType(b, "number");
        return b;
    }
    /** m2 comments
    */
    export module m2 {
        /** class comment; */
        export class c {
        };
        /** i */
        export let i = new c();
    }
    /** exported function */
    export function fooExport() {
AssertType(foo(), "number");
AssertType(foo, "() => number");
        return foo();
    }
}
m4.fooExport();
AssertType(m4.fooExport(), "number");
AssertType(m4.fooExport, "() => number");

let mylet2 = new m4.m2.c();
AssertType(mylet2, "m4.m2.c");
AssertType(new m4.m2.c(), "m4.m2.c");
AssertType(m4.m2.c, "typeof m4.m2.c");


