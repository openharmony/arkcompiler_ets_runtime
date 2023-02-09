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

// === tests/cases/compiler/circularInferredTypeOfVariable.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #14428

(async () => {
AssertType(async () => {    function foo(p: string[]): string[] {        return [];    }    function bar(p: string[]): string[] {        return [];    }    let a1: string[] | undefined = [];    while (true) {        let a2 = foo(a1!);        a1 = await bar(a2);    }}, "() => Promise<never>");
AssertType((async () => {    function foo(p: string[]): string[] {        return [];    }    function bar(p: string[]): string[] {        return [];    }    let a1: string[] | undefined = [];    while (true) {        let a2 = foo(a1!);        a1 = await bar(a2);    }}), "() => Promise<never>");

    function foo(p: string[]): string[] {
AssertType(foo, "(string[]) => string[]");
AssertType(p, "string[]");

AssertType([], "undefined[]");
        return [];
    }

    function bar(p: string[]): string[] {
AssertType(bar, "(string[]) => string[]");
AssertType(p, "string[]");

AssertType([], "undefined[]");
        return [];
    }

    let a1: string[] | undefined = [];
AssertType(a1, "string[]");
AssertType([], "undefined[]");

    while (true) {
AssertType(true, "boolean");

        let a2 = foo(a1!);
AssertType(a2, "string[]");
AssertType(foo(a1!), "string[]");
AssertType(foo, "(string[]) => string[]");
AssertType(a1!, "string[]");
AssertType(a1, "string[]");

        a1 = await bar(a2);
AssertType(a1 = await bar(a2), "string[]");
AssertType(a1, "string[]");
AssertType(await bar(a2), "string[]");
AssertType(bar(a2), "string[]");
AssertType(bar, "(string[]) => string[]");
AssertType(a2, "string[]");
    }
});

