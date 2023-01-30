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

// === tests/cases/conformance/expressions/nullishCoalescingOperator/nullishCoalescingOperator7.ts ===
declare function AssertType(value:any, type:string):void;
declare const a: string | undefined;
AssertType(a, "union");

declare const b: string | undefined;
AssertType(b, "union");

declare const c: string | undefined;
AssertType(c, "union");

const foo1 = a ? 1 : 2;
AssertType(foo1, "union");
AssertType(a ? 1 : 2, "union");
AssertType(a, "union");
AssertType(1, "int");
AssertType(2, "int");

const foo2 = a ?? 'foo' ? 1 : 2;
AssertType(foo2, "union");
AssertType(a ?? 'foo' ? 1 : 2, "union");
AssertType(a ?? 'foo', "string");
AssertType(a, "union");
AssertType('foo', "string");
AssertType(1, "int");
AssertType(2, "int");

const foo3 = a ?? 'foo' ? (b ?? 'bar') : (c ?? 'baz');
AssertType(foo3, "string");
AssertType(a ?? 'foo' ? (b ?? 'bar') : (c ?? 'baz'), "string");
AssertType(a ?? 'foo', "string");
AssertType(a, "union");
AssertType('foo', "string");
AssertType((b ?? 'bar'), "string");
AssertType(b ?? 'bar', "string");
AssertType(b, "union");
AssertType('bar', "string");
AssertType((c ?? 'baz'), "string");
AssertType(c ?? 'baz', "string");
AssertType(c, "union");
AssertType('baz', "string");

function f () {
    const foo4 = a ?? 'foo' ? b ?? 'bar' : c ?? 'baz';
AssertType(foo4, "string");
AssertType(a ?? 'foo' ? b ?? 'bar' : c ?? 'baz', "string");
AssertType(a ?? 'foo', "string");
AssertType(a, "union");
AssertType('foo', "string");
AssertType(b ?? 'bar', "string");
AssertType(b, "union");
AssertType('bar', "string");
AssertType(c ?? 'baz', "string");
AssertType(c, "union");
AssertType('baz', "string");
}


