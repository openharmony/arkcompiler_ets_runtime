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

// === tests/cases/compiler/innerOverloads.ts ===
declare function AssertType(value:any, type:string):void;
function outer() {
    function inner(x:number); // should work
AssertType(inner, "{ (number): any; (string): any; }");
AssertType(x, "number");

    function inner(x:string);
AssertType(inner, "{ (number): any; (string): any; }");
AssertType(x, "string");

    function inner(a:any) { 
AssertType(inner, "{ (number): any; (string): any; }");
return a; 

AssertType(a, "any");

AssertType(a, "any");
}

AssertType(inner(0), "any");
AssertType(inner, "{ (number): any; (string): any; }");
AssertType(0, "int");
    return inner(0);
}

let x = outer(); // should work
AssertType(x, "any");
AssertType(outer(), "any");
AssertType(outer, "() => any");



