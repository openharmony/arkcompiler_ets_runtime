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

// === tests/cases/conformance/es6/defaultParameters/emitDefaultParametersFunctionExpression.ts ===
declare function AssertType(value:any, type:string):void;
let lambda1 = (y = "hello") => { 
AssertType(lambda1, "(?string) => void");

AssertType((y = "hello") => { }, "(?string) => void");

AssertType(y, "string");

AssertType("hello", "string");
}

let lambda2 = (x: number, y = "hello") => { 
AssertType(lambda2, "(number, ?string) => void");

AssertType((x: number, y = "hello") => { }, "(number, ?string) => void");

AssertType(x, "number");

AssertType(y, "string");

AssertType("hello", "string");
}

let lambda3 = (x: number, y = "hello", ...rest) => { 
AssertType(lambda3, "(number, ?string, ...any[]) => void");

AssertType((x: number, y = "hello", ...rest) => { }, "(number, ?string, ...any[]) => void");

AssertType(x, "number");

AssertType(y, "string");

AssertType("hello", "string");

AssertType(rest, "any[]");
}

let lambda4 = (y = "hello", ...rest) => { 
AssertType(lambda4, "(?string, ...any[]) => void");

AssertType((y = "hello", ...rest) => { }, "(?string, ...any[]) => void");

AssertType(y, "string");

AssertType("hello", "string");

AssertType(rest, "any[]");
}

let x = function (str = "hello", ...rest) { 
AssertType(x, "(?string, ...any[]) => void");

AssertType(function (str = "hello", ...rest) { }, "(?string, ...any[]) => void");

AssertType(str, "string");

AssertType("hello", "string");

AssertType(rest, "any[]");
}

let y = (function (num = 10, boo = false, ...rest) { })()
AssertType(y, "void");
AssertType((function (num = 10, boo = false, ...rest) { })(), "void");
AssertType((function (num = 10, boo = false, ...rest) { }), "(?number, ?boolean) => void");
AssertType(function (num = 10, boo = false, ...rest) { }, "(?number, ?boolean) => void");
AssertType(num, "number");
AssertType(10, "int");
AssertType(boo, "boolean");
AssertType(false, "boolean");
AssertType(rest, "[]");

let z = (function (num: number, boo = false, ...rest) { })(10)
AssertType(z, "void");
AssertType((function (num: number, boo = false, ...rest) { })(10), "void");
AssertType((function (num: number, boo = false, ...rest) { }), "(number, ?boolean) => void");
AssertType(function (num: number, boo = false, ...rest) { }, "(number, ?boolean) => void");
AssertType(num, "number");
AssertType(boo, "boolean");
AssertType(false, "boolean");
AssertType(rest, "[]");
AssertType(10, "int");


