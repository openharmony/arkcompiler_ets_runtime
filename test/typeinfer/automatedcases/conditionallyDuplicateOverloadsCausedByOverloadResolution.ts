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

// === tests/cases/compiler/conditionallyDuplicateOverloadsCausedByOverloadResolution.ts ===
declare function AssertType(value:any, type:string):void;
declare function foo(func: (x: string, y: string) => any): boolean;
declare function foo(func: (x: string, y: number) => any): string;

let out = foo((x, y) => {
AssertType(out, "boolean");
AssertType(foo, "{ ((string, string) => any): boolean; ((string, number) => any): string; }");
AssertType((x, y) => {    function bar(a: typeof x): void;    function bar(b: typeof y): void;    function bar() { }    return bar;}, "(string, string) => { (string): void; (string): void; }");
AssertType(x, "string");
AssertType(y, "string");
AssertType(foo((x, y) => {    function bar(a: typeof x): void;    function bar(b: typeof y): void;    function bar() { }    return bar;}), "boolean");

    function bar(a: typeof x): void;
AssertType(bar, "{ (string): void; (string): void; }");
AssertType(a, "string");
AssertType(x, "string");

    function bar(b: typeof y): void;
AssertType(bar, "{ (string): void; (string): void; }");
AssertType(b, "string");
AssertType(y, "string");

    function bar() { 
AssertType(bar, "{ (string): void; (string): void; }");
}

AssertType(bar, "{ (string): void; (string): void; }");
    return bar;

});

declare function foo2(func: (x: string, y: string) => any): boolean;
declare function foo2(func: (x: string, y: number) => any): string;

let out2 = foo2((x, y) => {
AssertType(out2, "boolean");
AssertType(foo2, "{ ((string, string) => any): boolean; ((string, number) => any): string; }");
AssertType((x, y) => {    let bar: {        (a: typeof x): void;        (b: typeof y): void;    };    return bar;}, "(string, string) => { (string): void; (string): void; }");
AssertType(x, "string");
AssertType(y, "string");
AssertType(foo2((x, y) => {    let bar: {        (a: typeof x): void;        (b: typeof y): void;    };    return bar;}), "boolean");

    let bar: {
AssertType(bar, "{ (string): void; (string): void; }");

        (a: typeof x): void;
AssertType(a, "string");
AssertType(x, "string");

        (b: typeof y): void;
AssertType(b, "string");
AssertType(y, "string");

    };
AssertType(bar, "{ (string): void; (string): void; }");
    return bar;

});

