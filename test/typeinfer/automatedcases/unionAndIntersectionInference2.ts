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

// === tests/cases/conformance/types/typeRelationships/typeInference/unionAndIntersectionInference2.ts ===
declare function AssertType(value:any, type:string):void;
declare function f1<T>(x: T | string): T;

let a1: string;
AssertType(a1, "string");

let b1: string | string[];
AssertType(b1, "union");

let c1: string[] | string;
AssertType(c1, "union");

let d1: string | { name: string };
AssertType(d1, "union");
AssertType(name, "string");

let e1: number | string | boolean;
AssertType(e1, "union");

f1(a1); // string
AssertType(f1(a1), "string");
AssertType(f1, "<T>(union) => T");
AssertType(a1, "string");

f1(b1); // string[]
AssertType(f1(b1), "string[]");
AssertType(f1, "<T>(union) => T");
AssertType(b1, "union");

f1(c1); // string[]
AssertType(f1(c1), "string[]");
AssertType(f1, "<T>(union) => T");
AssertType(c1, "union");

f1(d1); // { name: string 
AssertType(f1(d1), "{ name: string; }");

AssertType(f1, "<T>(union) => T");

AssertType(d1, "union");
}

f1(e1); // number | boolean
AssertType(f1(e1), "union");
AssertType(f1, "<T>(union) => T");
AssertType(e1, "union");

declare function f2<T>(x: T & { name: string }): T;

let a2: string & { name: string };
AssertType(a2, "string & { name: string; }");
AssertType(name, "string");

let b2: { name: string } & string[];
AssertType(b2, "{ name: string; } & string[]");
AssertType(name, "string");

let c2: string & { name: string } & number;
AssertType(c2, "never");
AssertType(name, "string");

let d2: string & { name: string } & number & { name: string };
AssertType(d2, "never");
AssertType(name, "string");
AssertType(name, "string");

f2(a2); // string
AssertType(f2(a2), "string");
AssertType(f2, "<T>(T & { name: string; }) => T");
AssertType(a2, "string & { name: string; }");

f2(b2); // string[]
AssertType(f2(b2), "string[]");
AssertType(f2, "<T>(T & { name: string; }) => T");
AssertType(b2, "{ name: string; } & string[]");

f2(c2); // never
AssertType(f2(c2), "never");
AssertType(f2, "<T>(T & { name: string; }) => T");
AssertType(c2, "never");

f2(d2); // never
AssertType(f2(d2), "never");
AssertType(f2, "<T>(T & { name: string; }) => T");
AssertType(d2, "never");


