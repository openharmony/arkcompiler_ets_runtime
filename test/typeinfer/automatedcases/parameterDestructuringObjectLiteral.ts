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

// === tests/cases/compiler/parameterDestructuringObjectLiteral.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #22644

const fn1 = (options: { headers?: {} }) => { };
AssertType(fn1, "({    headers?: {};}) => void");
AssertType((options: { headers?: {} }) => { }, "({    headers?: {};}) => void");
AssertType(options, "{ headers?: {}; }");
AssertType(headers, "{}");

fn1({ headers: { foo: 1 } });
AssertType(fn1({ headers: { foo: 1 } }), "void");
AssertType(fn1, "({ headers?: {}; }) => void");
AssertType({ headers: { foo: 1 } }, "{ headers: { foo: number; }; }");
AssertType(headers, "{ foo: number; }");
AssertType({ foo: 1 }, "{ foo: number; }");
AssertType(foo, "number");
AssertType(1, "int");

const fn2 = ({ headers = {} }) => { };
AssertType(fn2, "({ headers?: {}; }) => void");
AssertType(({ headers = {} }) => { }, "({ headers?: {}; }) => void");
AssertType(headers, "{}");
AssertType({}, "{}");

fn2({ headers: { foo: 1 } });
AssertType(fn2({ headers: { foo: 1 } }), "void");
AssertType(fn2, "({ headers?: {}; }) => void");
AssertType({ headers: { foo: 1 } }, "{ headers: { foo: number; }; }");
AssertType(headers, "{ foo: number; }");
AssertType({ foo: 1 }, "{ foo: number; }");
AssertType(foo, "number");
AssertType(1, "int");


