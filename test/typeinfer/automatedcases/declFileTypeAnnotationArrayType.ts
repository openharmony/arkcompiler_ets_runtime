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

// === tests/cases/compiler/declFileTypeAnnotationArrayType.ts ===
declare function AssertType(value:any, type:string):void;
class c {
}
module m {
    export class c {
    }
    export class g<T> {
    }
}
class g<T> {
}

// Just the name
function foo(): c[] {
AssertType([new c()], "c[]");
AssertType(new c(), "c");
AssertType(c, "typeof c");
    return [new c()];
}
function foo2() {
AssertType([new c()], "c[]");
AssertType(new c(), "c");
AssertType(c, "typeof c");
    return [new c()];
}

// Qualified name
function foo3(): m.c[] {
AssertType([new m.c()], "m.c[]");
AssertType(new m.c(), "m.c");
AssertType(m.c, "typeof m.c");
    return [new m.c()];
}
function foo4() {
AssertType(m.c, "typeof m.c");
    return m.c;
}

// Just the name with type arguments
function foo5(): g<string>[] {
AssertType([new g<string>()], "g<string>[]");
AssertType(new g<string>(), "g<string>");
AssertType(g, "typeof g");
    return [new g<string>()];
}
function foo6() {
AssertType([new g<string>()], "g<string>[]");
AssertType(new g<string>(), "g<string>");
AssertType(g, "typeof g");
    return [new g<string>()];
}

// Qualified name with type arguments
function foo7(): m.g<number>[] {
AssertType([new m.g<number>()], "m.g<number>[]");
AssertType(new m.g<number>(), "m.g<number>");
AssertType(m.g, "typeof m.g");
    return [new m.g<number>()];
}
function foo8() {
AssertType([new m.g<number>()], "m.g<number>[]");
AssertType(new m.g<number>(), "m.g<number>");
AssertType(m.g, "typeof m.g");
    return [new m.g<number>()];
}

// Array of function types
function foo9(): (()=>c)[] {
AssertType([() => new c()], "(() => c)[]");
AssertType(() => new c(), "() => c");
AssertType(new c(), "c");
AssertType(c, "typeof c");
    return [() => new c()];
}
function foo10() {
AssertType([() => new c()], "(() => c)[]");
AssertType(() => new c(), "() => c");
AssertType(new c(), "c");
AssertType(c, "typeof c");
    return [() => new c()];
}

