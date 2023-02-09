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

// === tests/cases/conformance/es6/shorthandPropertyAssignment/objectLiteralShorthandProperties.ts ===
declare function AssertType(value:any, type:string):void;
let a, b, c;
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

let x1 = {
AssertType(x1, "{ a: any; }");
AssertType({    a}, "{ a: any; }");

    a
AssertType(a, "any");

};

let x2 = {
AssertType(x2, "{ a: any; }");
AssertType({    a,}, "{ a: any; }");

    a,
AssertType(a, "any");
}

let x3 = {
AssertType(x3, "any");
AssertType({    a: 0,    b,    c,    d() { },    x3,    parent: x3}, "{ a: number; b: any; c: any; d(): void; x3: any; parent: any; }");

    a: 0,
AssertType(a, "number");
AssertType(0, "int");

    b,
AssertType(b, "any");

    c,
AssertType(c, "any");

    d() { },
AssertType(d, "() => void");

    x3,
AssertType(x3, "any");

    parent: x3
AssertType(parent, "any");
AssertType(x3, "any");

};



