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

// === tests/cases/compiler/classFunctionMerging.ts ===
declare function AssertType(value:any, type:string):void;
// We allow ambient classes and functions to merge, this way callable classes
// which are also namespaces can be represented in declaration files
declare function Foo (x: number): Foo.Inst;
declare class Foo {
    constructor(x: string);
}
declare namespace Foo {
    export type Inst = number;
}

const a = new Foo("");
AssertType(a, "Foo");
AssertType(new Foo(""), "Foo");
AssertType(Foo, "typeof Foo");
AssertType("", "string");

const b = Foo(12);
AssertType(b, "number");
AssertType(Foo(12), "number");
AssertType(Foo, "typeof Foo");
AssertType(12, "int");


