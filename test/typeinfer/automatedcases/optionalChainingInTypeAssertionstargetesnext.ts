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

// === tests/cases/conformance/expressions/optionalChaining/optionalChainingInTypeAssertions.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
    m() {}
}

const foo = new Foo();
AssertType(foo, "Foo");
AssertType(new Foo(), "Foo");
AssertType(Foo, "typeof Foo");

(foo.m as any)?.();
AssertType((foo.m as any)?.(), "any");
AssertType((foo.m as any), "any");
AssertType(foo.m as any, "any");
AssertType(foo.m, "() => void");

(<any>foo.m)?.();
AssertType((<any>foo.m)?.(), "any");
AssertType((<any>foo.m), "any");
AssertType(<any>foo.m, "any");
AssertType(foo.m, "() => void");

/*a1*/(/*a2*/foo.m as any/*a3*/)/*a4*/?.();
AssertType((/*a2*/foo.m as any/*a3*/)/*a4*/?.(), "any");
AssertType((/*a2*/foo.m as any/*a3*/), "any");
AssertType(foo.m as any, "any");
AssertType(foo.m, "() => void");

/*b1*/(/*b2*/<any>foo.m/*b3*/)/*b4*/?.();
AssertType((/*b2*/<any>foo.m/*b3*/)/*b4*/?.(), "any");
AssertType((/*b2*/<any>foo.m/*b3*/), "any");
AssertType(<any>foo.m, "any");
AssertType(foo.m, "() => void");

// https://github.com/microsoft/TypeScript/issues/50148
(foo?.m as any).length;
AssertType((foo?.m as any).length, "any");

(<any>foo?.m).length;
AssertType((<any>foo?.m).length, "any");

(foo?.["m"] as any).length;
AssertType((foo?.["m"] as any).length, "any");

(<any>foo?.["m"]).length;
AssertType((<any>foo?.["m"]).length, "any");


