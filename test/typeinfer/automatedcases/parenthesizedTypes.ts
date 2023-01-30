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

// === tests/cases/conformance/types/specifyingTypes/typeLiterals/parenthesizedTypes.ts ===
declare function AssertType(value:any, type:string):void;
let a: string;
AssertType(a, "string");

let a: (string);
AssertType(a, "string");

let a: ((string) | string | (((string))));
AssertType(a, "string");

let a: ((((((((((((((((((((((((((((((((((((((((string))))))))))))))))))))))))))))))))))))))));
AssertType(a, "string");

let b: (x: string) => string;
AssertType(b, "(string) => string");
AssertType(x, "string");

let b: ((x: (string)) => (string));
AssertType(b, "(string) => string");
AssertType(x, "string");

let c: string[] | number[];
AssertType(c, "union");

let c: (string)[] | (number)[];
AssertType(c, "union");

let c: ((string)[]) | ((number)[]);
AssertType(c, "union");

let d: (((x: string) => string) | ((x: number) => number))[];
AssertType(d, "(union)[]");
AssertType(x, "string");
AssertType(x, "number");

let d: ({ (x: string): string } | { (x: number): number })[];
AssertType(d, "(union)[]");
AssertType(x, "string");
AssertType(x, "number");

let d: Array<((x: string) => string) | ((x: number) => number)>;
AssertType(d, "(union)[]");
AssertType(x, "string");
AssertType(x, "number");

let d: Array<{ (x: string): string } | { (x: number): number }>;
AssertType(d, "(union)[]");
AssertType(x, "string");
AssertType(x, "number");

let d: (Array<{ (x: string): string } | { (x: number): number }>);
AssertType(d, "(union)[]");
AssertType(x, "string");
AssertType(x, "number");

let e: typeof a[];
AssertType(e, "string[]");
AssertType(a, "string");

let e: (typeof a)[];
AssertType(e, "string[]");
AssertType(a, "string");

let f: (string) => string;
AssertType(f, "(any) => string");
AssertType(string, "any");

let f: (string: any) => string;
AssertType(f, "(any) => string");
AssertType(string, "any");

let g: [string, string];
AssertType(g, "[string, string]");

let g: [(string), string];
AssertType(g, "[string, string]");

let g: [(string), (((typeof a)))];
AssertType(g, "[string, string]");
AssertType(a, "string");


