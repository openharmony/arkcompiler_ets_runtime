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

// === tests/cases/compiler/overloadOnConstNoStringImplementation.ts ===
declare function AssertType(value:any, type:string):void;
function x2(a: number, cb: (x: 'hi') => number);
function x2(a: number, cb: (x: 'bye') => number);
function x2(a: number, cb: (x: any) => number) {
    cb('hi');
AssertType(cb('hi'), "number");
AssertType(cb, "(any) => number");
AssertType('hi', "string");

    cb('bye');
AssertType(cb('bye'), "number");
AssertType(cb, "(any) => number");
AssertType('bye', "string");

    let hm = 'hm';
AssertType(hm, "string");
AssertType('hm', "string");

    cb(hm); // should this work without a string definition?
AssertType(cb(hm), "number");
AssertType(cb, "(any) => number");
AssertType(hm, "string");

    cb('uh');
AssertType(cb('uh'), "number");
AssertType(cb, "(any) => number");
AssertType('uh', "string");

    cb(1); 
AssertType(cb(1), "number");
AssertType(cb, "(any) => number");
AssertType(1, "int");
}

let cb: (number) => number = (x: number) => 1;
AssertType(cb, "(any) => number");
AssertType(number, "any");
AssertType((x: number) => 1, "(number) => number");
AssertType(x, "number");
AssertType(1, "int");

x2(1, cb); // error
AssertType(x2(1, cb), "any");
AssertType(x2, "{ (number, ("hi") => number): any; (number, ("bye") => number): any; }");
AssertType(1, "int");
AssertType(cb, "(any) => number");

x2(1, (x: 'hi') => 1); // error
AssertType(x2(1, (x: 'hi') => 1), "any");
AssertType(x2, "{ (number, ("hi") => number): any; (number, ("bye") => number): any; }");
AssertType(1, "int");
AssertType((x: 'hi') => 1, "('hi') => number");
AssertType(x, "string");
AssertType(1, "int");

x2(1, (x: string) => 1);
AssertType(x2(1, (x: string) => 1), "any");
AssertType(x2, "{ (number, ("hi") => number): any; (number, ("bye") => number): any; }");
AssertType(1, "int");
AssertType((x: string) => 1, "(string) => number");
AssertType(x, "string");
AssertType(1, "int");


