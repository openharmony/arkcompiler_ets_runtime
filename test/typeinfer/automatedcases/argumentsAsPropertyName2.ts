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

// === tests/cases/compiler/argumentsAsPropertyName2.ts ===
declare function AssertType(value:any, type:string):void;
// target: es5

function foo() {
    for (let x = 0; x < 1; ++x) {
AssertType(x, "number");
AssertType(0, "int");
AssertType(x < 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");
AssertType(++x, "number");
AssertType(x, "number");

        let i : number;
AssertType(i, "number");

        [].forEach(function () { i });
AssertType([].forEach(function () { i }), "void");
AssertType([].forEach, "((any, number, any[]) => void, ?any) => void");
AssertType([], "undefined[]");
AssertType(function () { i }, "() => void");
AssertType(i, "number");

        ({ arguments: 0 });
AssertType(({ arguments: 0 }), "{ arguments: number; }");
AssertType({ arguments: 0 }, "{ arguments: number; }");
AssertType(arguments, "number");
AssertType(0, "int");

        ({ arguments });
AssertType(({ arguments }), "{ arguments: IArguments; }");
AssertType({ arguments }, "{ arguments: IArguments; }");
AssertType(arguments, "IArguments");

        ({ arguments: arguments });
AssertType(({ arguments: arguments }), "{ arguments: IArguments; }");
AssertType({ arguments: arguments }, "{ arguments: IArguments; }");
AssertType(arguments, "IArguments");
AssertType(arguments, "IArguments");
    }
}


