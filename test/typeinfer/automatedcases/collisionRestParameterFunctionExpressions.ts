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

// === tests/cases/compiler/collisionRestParameterFunctionExpressions.ts ===
declare function AssertType(value:any, type:string):void;
function foo() {
    function f1(_i: number, ...restParameters) { //_i is error
AssertType(f1, "(number, ...any[]) => void");
AssertType(_i, "number");
AssertType(restParameters, "any[]");

        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
    function f1NoError(_i: number) { // no error
AssertType(f1NoError, "(number) => void");
AssertType(_i, "number");

        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
    function f3(...restParameters) {
AssertType(f3, "(...any[]) => void");
AssertType(restParameters, "any[]");

        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
    function f3NoError() {
AssertType(f3NoError, "() => void");

        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }

    function f4(_i: number, ...rest); // no codegen no error
AssertType(f4, "{ (number, ...any[]): any; (string, ...any[]): any; }");
AssertType(_i, "number");
AssertType(rest, "any[]");

    function f4(_i: string, ...rest); // no codegen no error
AssertType(f4, "{ (number, ...any[]): any; (string, ...any[]): any; }");
AssertType(_i, "string");
AssertType(rest, "any[]");

    function f4(_i: any, ...rest) { // error
AssertType(f4, "{ (number, ...any[]): any; (string, ...any[]): any; }");
AssertType(_i, "any");
AssertType(rest, "any[]");
    }

    function f4NoError(_i: number); // no error
AssertType(f4NoError, "{ (number): any; (string): any; }");
AssertType(_i, "number");

    function f4NoError(_i: string); // no error
AssertType(f4NoError, "{ (number): any; (string): any; }");
AssertType(_i, "string");

    function f4NoError(_i: any) { // no error
AssertType(f4NoError, "{ (number): any; (string): any; }");
AssertType(_i, "any");
    }
}

