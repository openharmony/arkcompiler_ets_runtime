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

// === tests/cases/conformance/types/members/typesWithOptionalProperty.ts ===
declare function AssertType(value:any, type:string):void;
// basic uses of optional properties without errors

interface I {
    foo: string;
    bar?: number;
    baz? (): string;
}

let a: {
AssertType(a, "{ foo: string; bar?: number; baz?(): string; }");

    foo: string;
AssertType(foo, "string");

    bar?: number;
AssertType(bar, "number");

    baz? (): string;
AssertType(baz, "() => string");

};

let b = { foo: '' };
AssertType(b, "{ foo: string; }");
AssertType({ foo: '' }, "{ foo: string; }");
AssertType(foo, "string");
AssertType('', "string");

let c = { foo: '', bar: 3 };
AssertType(c, "{ foo: string; bar: number; }");
AssertType({ foo: '', bar: 3 }, "{ foo: string; bar: number; }");
AssertType(foo, "string");
AssertType('', "string");
AssertType(bar, "number");
AssertType(3, "int");

let d = { foo: '', bar: 3, baz: () => '' };
AssertType(d, "{ foo: string; bar: number; baz: () => string; }");
AssertType({ foo: '', bar: 3, baz: () => '' }, "{ foo: string; bar: number; baz: () => string; }");
AssertType(foo, "string");
AssertType('', "string");
AssertType(bar, "number");
AssertType(3, "int");
AssertType(baz, "() => string");
AssertType(() => '', "() => string");
AssertType('', "string");

let i: I;
AssertType(i, "I");

i = b;
AssertType(i = b, "{ foo: string; }");
AssertType(i, "I");
AssertType(b, "{ foo: string; }");

i = c;
AssertType(i = c, "{ foo: string; bar: number; }");
AssertType(i, "I");
AssertType(c, "{ foo: string; bar: number; }");

i = d;
AssertType(i = d, "{ foo: string; bar: number; baz: () => string; }");
AssertType(i, "I");
AssertType(d, "{ foo: string; bar: number; baz: () => string; }");

a = b;
AssertType(a = b, "{ foo: string; }");
AssertType(a, "{ foo: string; bar?: number; baz?(): string; }");
AssertType(b, "{ foo: string; }");

a = c;
AssertType(a = c, "{ foo: string; bar: number; }");
AssertType(a, "{ foo: string; bar?: number; baz?(): string; }");
AssertType(c, "{ foo: string; bar: number; }");

a = d;
AssertType(a = d, "{ foo: string; bar: number; baz: () => string; }");
AssertType(a, "{ foo: string; bar?: number; baz?(): string; }");
AssertType(d, "{ foo: string; bar: number; baz: () => string; }");

i = a;
AssertType(i = a, "{ foo: string; bar?: number; baz?(): string; }");
AssertType(i, "I");
AssertType(a, "{ foo: string; bar?: number; baz?(): string; }");

a = i;
AssertType(a = i, "I");
AssertType(a, "{ foo: string; bar?: number; baz?(): string; }");
AssertType(i, "I");


