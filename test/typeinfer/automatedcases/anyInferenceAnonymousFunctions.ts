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

// === tests/cases/compiler/anyInferenceAnonymousFunctions.ts ===
declare function AssertType(value:any, type:string):void;
let paired: any[];
AssertType(paired, "any[]");

paired.reduce(function (a1, a2) {
AssertType(paired.reduce, "{ ((any, any, number, any[]) => any): any; ((any, any, number, any[]) => any, any): any; <U>((U, any, number, any[]) => U, U): U; }");
AssertType(function (a1, a2) {    return a1.concat({});}, "(any, any) => any");
AssertType(a1, "any");
AssertType(a2, "any");
AssertType(paired.reduce(function (a1, a2) {    return a1.concat({});} , []), "any");

AssertType(a1.concat({}), "any");
AssertType(a1.concat, "any");
AssertType({}, "{}");
    return a1.concat({});

} , []);
AssertType([], "undefined[]");

paired.reduce((b1, b2) => {
AssertType(paired.reduce, "{ ((any, any, number, any[]) => any): any; ((any, any, number, any[]) => any, any): any; <U>((U, any, number, any[]) => U, U): U; }");
AssertType((b1, b2) => {    return b1.concat({});}, "(any, any) => any");
AssertType(b1, "any");
AssertType(b2, "any");
AssertType(paired.reduce((b1, b2) => {    return b1.concat({});} , []), "any");

AssertType(b1.concat({}), "any");
AssertType(b1.concat, "any");
AssertType({}, "{}");
    return b1.concat({});

} , []);
AssertType([], "undefined[]");

paired.reduce((b3, b4) => b3.concat({}), []);
AssertType(paired.reduce((b3, b4) => b3.concat({}), []), "any");
AssertType(paired.reduce, "{ ((any, any, number, any[]) => any): any; ((any, any, number, any[]) => any, any): any; <U>((U, any, number, any[]) => U, U): U; }");
AssertType((b3, b4) => b3.concat({}), "(any, any) => any");
AssertType(b3, "any");
AssertType(b4, "any");
AssertType(b3.concat({}), "any");
AssertType(b3.concat, "any");
AssertType({}, "{}");
AssertType([], "undefined[]");

paired.map((c1) => c1.count);
AssertType(paired.map((c1) => c1.count), "any[]");
AssertType(paired.map, "<U>((any, number, any[]) => U, ?any) => U[]");
AssertType((c1) => c1.count, "(any) => any");
AssertType(c1, "any");
AssertType(c1.count, "any");

paired.map(function (c2) { 
AssertType(paired.map(function (c2) { return c2.count; }), "any[]");
AssertType(paired.map, "<U>((any, number, any[]) => U, ?any) => U[]");
AssertType(function (c2) { return c2.count; }, "(any) => any");
AssertType(c2, "any");
AssertType(c2.count, "any");
return c2.count; });


