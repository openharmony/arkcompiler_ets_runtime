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

// === tests/cases/compiler/declFileTypeofFunction.ts ===
declare function AssertType(value:any, type:string):void;
function f(n: typeof f): string;
function f(n: typeof g): string;
function f() { 
AssertType(undefined, "undefined");
return undefined; 
}

function g(n: typeof g): number;
function g(n: typeof f): number;
function g() { 
AssertType(undefined, "undefined");
return undefined; 
}

let b: () => typeof b;
AssertType(b, "() => typeof b");
AssertType(b, "() => typeof b");

function b1() {
AssertType(b1, "() => typeof b1");
    return b1;
}

function foo(): typeof foo {
AssertType(null, "null");
    return null;
}
let foo1: typeof foo;
AssertType(foo1, "() => typeof foo");
AssertType(foo, "() => typeof foo");

let foo2 = foo;
AssertType(foo2, "() => typeof foo");
AssertType(foo, "() => typeof foo");

let foo3 = function () {
AssertType(foo3, "() => any");
AssertType(function () {    return foo3;}, "() => any");

AssertType(foo3, "() => any");
    return foo3;
}
let x = () => {
AssertType(x, "() => any");
AssertType(() => {    return x;}, "() => any");

AssertType(x, "() => any");
    return x;
}

function foo5(x: number) {
    function bar(x: number) {
AssertType(bar, "(number) => number");
AssertType(x, "number");

AssertType(x, "number");
        return x;
    }
AssertType(bar, "(number) => number");
    return bar;
}

