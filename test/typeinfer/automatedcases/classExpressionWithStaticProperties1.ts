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

// === tests/cases/compiler/classExpressionWithStaticProperties1.ts ===
declare function AssertType(value:any, type:string):void;
let v = class C {
AssertType(v, "typeof C");
AssertType(class C {    static a = 1;    static b = 2;    static c = C.a + C.b;}, "typeof C");
AssertType(C, "typeof C");

    static a = 1;
AssertType(a, "number");
AssertType(1, "int");

    static b = 2;
AssertType(b, "number");
AssertType(2, "int");

    static c = C.a + C.b;
AssertType(c, "number");
AssertType(C.a + C.b, "number");
AssertType(C.a, "number");
AssertType(C.b, "number");

};

