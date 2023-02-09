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

// === tests/cases/compiler/declarationEmitFirstTypeArgumentGenericFunctionType.ts ===
declare function AssertType(value:any, type:string):void;
class X<A> {
}
let prop11: X< <Tany>() => Tany >; // spaces before the first type argument
AssertType(prop11, "X<(<Tany>() => Tany)>");

let prop12: X<(<Tany>() => Tany)>; // spaces before the first type argument
AssertType(prop12, "X<(<Tany>() => Tany)>");

function f1() { // Inferred 
return type
AssertType(prop11, "X<(<Tany>() => Tany)>");
    return prop11;
}
function f2() { // Inferred 
return type
AssertType(prop12, "X<(<Tany>() => Tany)>");
    return prop12;
}
function f3(): X< <Tany>() => Tany> { // written with space before type argument
AssertType(prop11, "X<(<Tany>() => Tany)>");
    return prop11;
}
function f4(): X<(<Tany>() => Tany)> { // written type with parenthesis
AssertType(prop12, "X<(<Tany>() => Tany)>");
    return prop12;
}
class Y<A, B> {
}
let prop2: Y<string[], <Tany>() => Tany>; // No space after second type argument
AssertType(prop2, "Y<string[], <Tany>() => Tany>");

let prop2: Y<string[], <Tany>() => Tany>; // space after second type argument
AssertType(prop2, "Y<string[], <Tany>() => Tany>");

let prop3: Y< <Tany>() => Tany, <Tany>() => Tany>; // space before first type argument
AssertType(prop3, "Y<(<Tany>() => Tany), <Tany>() => Tany>");

let prop4: Y<(<Tany>() => Tany), <Tany>() => Tany>; // parenthesized first type argument
AssertType(prop4, "Y<(<Tany>() => Tany), <Tany>() => Tany>");


