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

// === tests/cases/conformance/types/spread/objectSpreadNoTransform.ts ===
declare function AssertType(value:any, type:string):void;
const y = { a: 'yes', b: 'no' };
AssertType(y, "{ a: string; b: string; }");
AssertType({ a: 'yes', b: 'no' }, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType('yes', "string");
AssertType(b, "string");
AssertType('no', "string");

const o = { x: 1, ...y };
AssertType(o, "{ a: string; b: string; x: number; }");
AssertType({ x: 1, ...y }, "{ a: string; b: string; x: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "{ a: string; b: string; }");

let b;
AssertType(b, "any");

let rest: any;
AssertType(rest, "any");

({ b, ...rest } = o);
AssertType(({ b, ...rest } = o), "{ a: string; b: string; x: number; }");
AssertType({ b, ...rest } = o, "{ a: string; b: string; x: number; }");
AssertType({ b, ...rest }, "any");
AssertType(b, "any");
AssertType(rest, "any");
AssertType(o, "{ a: string; b: string; x: number; }");


