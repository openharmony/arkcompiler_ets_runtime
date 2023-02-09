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

// === tests/cases/conformance/parser/ecmascript5/Generics/parserAmbiguityWithBinaryOperator2.ts ===
declare function AssertType(value:any, type:string):void;
function f() {
    let a, b, c;
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

    if (a < b && b > (c + 1)) { 
AssertType(a < b && b > (c + 1), "boolean");

AssertType(a < b, "boolean");

AssertType(a, "any");

AssertType(b, "any");

AssertType(b > (c + 1), "boolean");

AssertType(b, "any");

AssertType((c + 1), "any");

AssertType(c + 1, "any");

AssertType(c, "any");

AssertType(1, "int");
}
}

