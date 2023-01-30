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

// === tests/cases/compiler/contextualTypingWithGenericAndNonGenericSignature.ts ===
declare function AssertType(value:any, type:string):void;
//•	If e is a FunctionExpression or ArrowFunctionExpression with no type parameters and no parameter or return type annotations, and T is a function type with EXACTLY ONE non - generic call signature, then any inferences made for type parameters referenced by the parameters of T’s call signature are fixed(section 4.12.2) and e is processed with the contextual type T, as described in section 4.9.3.

let f2: {
AssertType(f2, "{ (string, number): string; <T, U>(T, U): T; }");

    (x: string, y: number): string;
AssertType(x, "string");
AssertType(y, "number");

    <T, U>(x: T, y: U): T
AssertType(x, "T");
AssertType(y, "U");

};

f2 = (x, y) => { 
AssertType(f2 = (x, y) => { return x }, "(any, any) => any");
return x 

AssertType(f2, "{ (string, number): string; <T, U>(T, U): T; }");

AssertType((x, y) => { return x }, "(any, any) => any");

AssertType(x, "any");

AssertType(y, "any");

AssertType(x, "any");
}

let f3: {
AssertType(f3, "{ <T, U>(T, U): T; (string, number): string; }");

    <T, U>(x: T, y: U): T
AssertType(x, "T");
AssertType(y, "U");

    (x: string, y: number): string;
AssertType(x, "string");
AssertType(y, "number");

};

f3 = (x, y) => { 
AssertType(f3 = (x, y) => { return x }, "(any, any) => any");
return x 

AssertType(f3, "{ <T, U>(T, U): T; (string, number): string; }");

AssertType((x, y) => { return x }, "(any, any) => any");

AssertType(x, "any");

AssertType(y, "any");

AssertType(x, "any");
}


