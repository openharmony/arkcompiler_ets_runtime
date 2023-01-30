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

// === tests/cases/compiler/genericInference2.ts ===
declare function AssertType(value:any, type:string):void;
    declare module ko { 
       export interface Observable<T> { 
           (): T; 
           (value: T): any; 
           N: number; 
           g: boolean; 
           r: T; 
       } 
       export function observable<T>(value: T): Observable<T>; 
    } 
    let o = { 
AssertType(o, "{ name: ko.Observable<string>; age: ko.Observable<number>; }");
AssertType({        name: ko.observable("Bob"),        age: ko.observable(37)     }, "{ name: ko.Observable<string>; age: ko.Observable<number>; }");

       name: ko.observable("Bob"), 
AssertType(name, "ko.Observable<string>");
AssertType(ko.observable("Bob"), "ko.Observable<string>");
AssertType(ko.observable, "<T>(T) => ko.Observable<T>");
AssertType("Bob", "string");

       age: ko.observable(37) 
AssertType(age, "ko.Observable<number>");
AssertType(ko.observable(37), "ko.Observable<number>");
AssertType(ko.observable, "<T>(T) => ko.Observable<T>");
AssertType(37, "int");

    }; 
    let x_v = o.name().length;  // should be 'number'
AssertType(x_v, "number");
AssertType(o.name().length, "number");

    let age_v = o.age();  // should be 'number'
AssertType(age_v, "number");
AssertType(o.age(), "number");
AssertType(o.age, "ko.Observable<number>");

    let name_v = o.name("Robert");  // should be 'any'
AssertType(name_v, "any");
AssertType(o.name("Robert"), "any");
AssertType(o.name, "ko.Observable<string>");
AssertType("Robert", "string");

    let zz_v = o.name.N;  // should be 'number'
AssertType(zz_v, "number");
AssertType(o.name.N, "number");

    let yy_v = o.name.g;  // should be 'boolean'
AssertType(yy_v, "boolean");
AssertType(o.name.g, "boolean");

    let rr_v = o.name.r;  // should be 'string'
AssertType(rr_v, "string");
AssertType(o.name.r, "string");


