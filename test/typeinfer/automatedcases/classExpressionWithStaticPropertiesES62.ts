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

// === tests/cases/compiler/classExpressionWithStaticPropertiesES62.ts ===
declare function AssertType(value:any, type:string):void;
let v = class C {
AssertType(v, "typeof C");
AssertType(class C {    static a = 1;    static b    static c = {        x: "hi"    }    static d = C.c.x + " world"; }, "typeof C");
AssertType(C, "typeof C");

    static a = 1;
AssertType(a, "number");
AssertType(1, "int");

    static b
AssertType(b, "any");

    static c = {
AssertType(c, "{ x: string; }");
AssertType({        x: "hi"    }, "{ x: string; }");

        x: "hi"
AssertType(x, "string");
AssertType("hi", "string");
    }
    static d = C.c.x + " world";
AssertType(d, "string");
AssertType(C.c.x + " world", "string");
AssertType(C.c.x, "string");
AssertType(" world", "string");

 };

