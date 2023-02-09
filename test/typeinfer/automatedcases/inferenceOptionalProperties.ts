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

// === tests/cases/compiler/inferenceOptionalProperties.ts ===
declare function AssertType(value:any, type:string):void;
declare function test<T>(x: { [key: string]: T }): T;

declare let x1: { a?: string, b?: number };
AssertType(x1, "{ a?: string; b?: number; }");
AssertType(a, "union");
AssertType(b, "union");

declare let x2: { a?: string, b?: number | undefined };
AssertType(x2, "{ a?: string; b?: union; }");
AssertType(a, "union");
AssertType(b, "union");

const y1 = test(x1);
AssertType(y1, "union");
AssertType(test(x1), "union");
AssertType(test, "<T>({ [string]: T; }) => T");
AssertType(x1, "{ a?: string; b?: number; }");

const y2 = test(x2);
AssertType(y2, "union");
AssertType(test(x2), "union");
AssertType(test, "<T>({ [string]: T; }) => T");
AssertType(x2, "{ a?: string; b?: union; }");

let v1: Required<{ a?: string, b?: number }>;
AssertType(v1, "Required<{ a?: string; b?: number; }>");
AssertType(a, "union");
AssertType(b, "union");

let v1: { a: string, b: number };
AssertType(v1, "Required<{ a?: string; b?: number; }>");
AssertType(a, "string");
AssertType(b, "number");

let v2: Required<{ a?: string, b?: number | undefined }>;
AssertType(v2, "Required<{ a?: string; b?: union; }>");
AssertType(a, "union");
AssertType(b, "union");

let v2: { a: string, b: number | undefined };
AssertType(v2, "Required<{ a?: string; b?: union; }>");
AssertType(a, "string");
AssertType(b, "union");

let v3: Partial<{ a: string, b: string }>;
AssertType(v3, "Partial<{ a: string; b: string; }>");
AssertType(a, "string");
AssertType(b, "string");

let v3: { a?: string, b?: string };
AssertType(v3, "Partial<{ a: string; b: string; }>");
AssertType(a, "union");
AssertType(b, "union");

let v4: Partial<{ a: string, b: string | undefined }>;
AssertType(v4, "Partial<{ a: string; b: union; }>");
AssertType(a, "string");
AssertType(b, "union");

let v4: { a?: string, b?: string | undefined };
AssertType(v4, "Partial<{ a: string; b: union; }>");
AssertType(a, "union");
AssertType(b, "union");

let v5: Required<Partial<{ a: string, b: string }>>;
AssertType(v5, "Required<Partial<{ a: string; b: string; }>>");
AssertType(a, "string");
AssertType(b, "string");

let v5: { a: string, b: string };
AssertType(v5, "Required<Partial<{ a: string; b: string; }>>");
AssertType(a, "string");
AssertType(b, "string");

let v6: Required<Partial<{ a: string, b: string | undefined }>>;
AssertType(v6, "Required<Partial<{ a: string; b: union; }>>");
AssertType(a, "string");
AssertType(b, "union");

let v6: { a: string, b: string | undefined };
AssertType(v6, "Required<Partial<{ a: string; b: union; }>>");
AssertType(a, "string");
AssertType(b, "union");


