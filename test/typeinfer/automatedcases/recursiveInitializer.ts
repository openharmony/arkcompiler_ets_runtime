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

// === tests/cases/conformance/statements/VariableStatements/recursiveInitializer.ts ===
declare function AssertType(value:any, type:string):void;
// number unless otherwise specified
let n1 = n1++;
AssertType(n1, "any");
AssertType(n1++, "number");
AssertType(n1, "any");

let n2: number = n2 + n2;
AssertType(n2, "number");
AssertType(n2 + n2, "number");
AssertType(n2, "number");
AssertType(n2, "number");

let n3 /* any */ = n3 + n3;
AssertType(n3, "any");
AssertType(n3 + n3, "any");
AssertType(n3, "any");
AssertType(n3, "any");

// string unless otherwise specified
let s1 = s1 + '';
AssertType(s1, "any");
AssertType(s1 + '', "string");
AssertType(s1, "any");
AssertType('', "string");

let s2 /* any */ = s2 + s2;
AssertType(s2, "any");
AssertType(s2 + s2, "any");
AssertType(s2, "any");
AssertType(s2, "any");

let s3 : string = s3 + s3;
AssertType(s3, "string");
AssertType(s3 + s3, "string");
AssertType(s3, "string");
AssertType(s3, "string");

let s4 = '' + s4;
AssertType(s4, "any");
AssertType('' + s4, "string");
AssertType('', "string");
AssertType(s4, "any");

// boolean unless otherwise specified
let b1 = !b1;
AssertType(b1, "any");
AssertType(!b1, "boolean");
AssertType(b1, "any");

let b2 = !!b2;
AssertType(b2, "any");
AssertType(!!b2, "boolean");
AssertType(!b2, "boolean");
AssertType(b2, "any");

let b3 = !b3 || b3; // expected boolean here. actually 'any'
AssertType(b3, "any");
AssertType(!b3 || b3, "any");
AssertType(!b3, "boolean");
AssertType(b3, "any");
AssertType(b3, "any");

let b4 = (!b4) && b4; // expected boolean here. actually 'any'
AssertType(b4, "any");
AssertType((!b4) && b4, "any");
AssertType((!b4), "boolean");
AssertType(!b4, "boolean");
AssertType(b4, "any");
AssertType(b4, "any");

// (x:string) => any
let f = (x: string) => f(x);
AssertType(f, "(string) => any");
AssertType((x: string) => f(x), "(string) => any");
AssertType(x, "string");
AssertType(f(x), "any");
AssertType(f, "(string) => any");
AssertType(x, "string");


