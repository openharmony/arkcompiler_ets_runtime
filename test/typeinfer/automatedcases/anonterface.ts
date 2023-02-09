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

// === tests/cases/compiler/anonterface.ts ===
declare function AssertType(value:any, type:string):void;
module M {
    export class C {
        m(fn:{ (n:number):string; },n2:number):string {
AssertType(fn(n2), "string");
AssertType(fn, "(number) => string");
AssertType(n2, "number");
            return fn(n2);
        }
    }
}

let c=new M.C();
AssertType(c, "M.C");
AssertType(new M.C(), "M.C");
AssertType(M.C, "typeof M.C");

c.m(function(n) { 
AssertType(c.m(function(n) { return "hello: "+n; },18), "string");
AssertType(c.m, "((number) => string, number) => string");
AssertType(function(n) { return "hello: "+n; }, "(number) => string");
AssertType(n, "number");
AssertType("hello: "+n, "string");
AssertType("hello: ", "string");
AssertType(n, "number");
AssertType(18, "int");
return "hello: "+n; },18);





