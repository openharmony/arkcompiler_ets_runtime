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

// === tests/cases/compiler/genericObjectLitReturnType.ts ===
declare function AssertType(value:any, type:string):void;
class X<T>
{
    f(t: T) { 
AssertType({ a: t }, "{ a: T; }");
return { a: t }; 

AssertType(a, "T");

AssertType(t, "T");
}
}

 
let x: X<number>;
AssertType(x, "X<number>");

let t1 = x.f(5);
AssertType(t1, "{ a: number; }");
AssertType(x.f(5), "{ a: number; }");
AssertType(x.f, "(number) => { a: number; }");
AssertType(5, "int");

t1.a = 5; // Should not error: t1 should have type {a: number}, instead has type {a: T
AssertType(t1.a = 5, "int");

AssertType(t1.a, "number");

AssertType(5, "int");
}
 


