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

// === tests/cases/conformance/interfaces/interfaceDeclarations/interfaceWithSpecializedCallAndConstructSignatures.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo {
    (x: 'a'): number;
    (x: string): any;

    new (x: 'a'): any;
    new (x: string): Object;
}

let f: Foo;
AssertType(f, "Foo");

let r = f('a');
AssertType(r, "number");
AssertType(f('a'), "number");
AssertType(f, "Foo");
AssertType('a', "string");

let r2 = f('A');
AssertType(r2, "any");
AssertType(f('A'), "any");
AssertType(f, "Foo");
AssertType('A', "string");

let r3 = new f('a');
AssertType(r3, "any");
AssertType(new f('a'), "any");
AssertType(f, "Foo");
AssertType('a', "string");

let r4 = new f('A');
AssertType(r4, "Object");
AssertType(new f('A'), "Object");
AssertType(f, "Foo");
AssertType('A', "string");


