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

// === tests/cases/compiler/targetTypeArgs.ts ===
declare function AssertType(value:any, type:string):void;
function foo(callback: (x: string) => void) {
    callback("hello");   
AssertType(callback("hello"), "void");
AssertType(callback, "(string) => void");
AssertType("hello", "string");
}

foo(function(x) { x });
AssertType(foo(function(x) { x }), "void");
AssertType(foo, "((string) => void) => void");
AssertType(function(x) { x }, "(string) => void");
AssertType(x, "string");
AssertType(x, "string");

[1].forEach(function(v,i,a) { v });
AssertType([1].forEach(function(v,i,a) { v }), "void");
AssertType([1].forEach, "((number, number, number[]) => void, ?any) => void");
AssertType(function(v,i,a) { v }, "(number, number, number[]) => void");
AssertType(v, "number");
AssertType(i, "number");
AssertType(a, "number[]");
AssertType(v, "number");

["hello"].every(function(v,i,a) {
AssertType(["hello"].every(function(v,i,a) {return true;}), "boolean");
AssertType(["hello"].every, "{ <S extends string>((string, number, string[]) => value is S, ?any): this is S[]; ((string, number, string[]) => unknown, ?any): boolean; }");
AssertType(function(v,i,a) {return true;}, "(string, number, string[]) => true");
AssertType(v, "string");
AssertType(i, "number");
AssertType(a, "string[]");
AssertType(true, "boolean");
return true;});

[1].every(function(v,i,a) {
AssertType([1].every(function(v,i,a) {return true;}), "boolean");
AssertType([1].every, "{ <S extends number>((number, number, number[]) => value is S, ?any): this is S[]; ((number, number, number[]) => unknown, ?any): boolean; }");
AssertType(function(v,i,a) {return true;}, "(number, number, number[]) => true");
AssertType(v, "number");
AssertType(i, "number");
AssertType(a, "number[]");
AssertType(true, "boolean");
return true;});

[1].every(function(v,i,a) {
AssertType([1].every(function(v,i,a) {return true;}), "boolean");
AssertType([1].every, "{ <S extends number>((number, number, number[]) => value is S, ?any): this is S[]; ((number, number, number[]) => unknown, ?any): boolean; }");
AssertType(function(v,i,a) {return true;}, "(number, number, number[]) => true");
AssertType(v, "number");
AssertType(i, "number");
AssertType(a, "number[]");
AssertType(true, "boolean");
return true;});

["s"].every(function(v,i,a) {
AssertType(["s"].every(function(v,i,a) {return true;}), "boolean");
AssertType(["s"].every, "{ <S extends string>((string, number, string[]) => value is S, ?any): this is S[]; ((string, number, string[]) => unknown, ?any): boolean; }");
AssertType(function(v,i,a) {return true;}, "(string, number, string[]) => true");
AssertType(v, "string");
AssertType(i, "number");
AssertType(a, "string[]");
AssertType(true, "boolean");
return true;});

["s"].forEach(function(v,i,a) { v });
AssertType(["s"].forEach(function(v,i,a) { v }), "void");
AssertType(["s"].forEach, "((string, number, string[]) => void, ?any) => void");
AssertType(function(v,i,a) { v }, "(string, number, string[]) => void");
AssertType(v, "string");
AssertType(i, "number");
AssertType(a, "string[]");
AssertType(v, "string");



