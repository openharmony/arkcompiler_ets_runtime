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

// === tests/cases/compiler/arrayBindingPatternOmittedExpressions.ts ===
declare function AssertType(value:any, type:string):void;
let results: string[];
AssertType(results, "string[]");

{
    let [, b, , a] = results;
AssertType(, "undefined");
AssertType(b, "string");
AssertType(, "undefined");
AssertType(a, "string");
AssertType(results, "string[]");

    let x = {
AssertType(x, "{ a: string; b: string; }");
AssertType({        a,        b    }, "{ a: string; b: string; }");

        a,
AssertType(a, "string");

        b
AssertType(b, "string");
    }
}


function f([, a, , b, , , , s, , , ] = results) {
    a = s[1];
AssertType(a = s[1], "string");
AssertType(a, "string");
AssertType(s[1], "string");
AssertType(s, "string");
AssertType(1, "int");

    b = s[2];
AssertType(b = s[2], "string");
AssertType(b, "string");
AssertType(s[2], "string");
AssertType(s, "string");
AssertType(2, "int");
}

