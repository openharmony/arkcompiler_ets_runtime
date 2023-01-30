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

// === tests/cases/conformance/es6/restParameters/emitRestParametersFunctionExpression.ts ===
declare function AssertType(value:any, type:string):void;
let funcExp = (...rest) => { 
AssertType(funcExp, "(...any[]) => void");

AssertType((...rest) => { }, "(...any[]) => void");

AssertType(rest, "any[]");
}

let funcExp1 = (X: number, ...rest) => { 
AssertType(funcExp1, "(number, ...any[]) => void");

AssertType((X: number, ...rest) => { }, "(number, ...any[]) => void");

AssertType(X, "number");

AssertType(rest, "any[]");
}

let funcExp2 = function (...rest) { 
AssertType(funcExp2, "(...any[]) => void");

AssertType(function (...rest) { }, "(...any[]) => void");

AssertType(rest, "any[]");
}

let funcExp3 = (function (...rest) { })()
AssertType(funcExp3, "void");
AssertType((function (...rest) { })(), "void");
AssertType((function (...rest) { }), "() => void");
AssertType(function (...rest) { }, "() => void");
AssertType(rest, "[]");


