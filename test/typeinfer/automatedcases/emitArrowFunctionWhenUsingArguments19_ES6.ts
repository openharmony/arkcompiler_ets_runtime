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

// === tests/cases/conformance/es6/arrowFunction/emitArrowFunctionWhenUsingArguments19_ES6.ts ===
declare function AssertType(value:any, type:string):void;
function f() {
    function g() {
AssertType(g, "() => void");

        let _arguments = 10;                // No capture in 'g', so no conflict.
AssertType(_arguments, "number");
AssertType(10, "int");

        function h() {
AssertType(h, "() => void");

            let capture = () => arguments;  // Should trigger an '_arguments' capture into function 'h'
AssertType(capture, "() => IArguments");
AssertType(() => arguments, "() => IArguments");
AssertType(arguments, "IArguments");

            foo(_arguments);                // Error as this does not resolve to the user defined '_arguments'
AssertType(foo(_arguments), "number");
AssertType(foo, "(any) => number");
AssertType(_arguments, "number");
        }
    }

    function foo(x: any) {
AssertType(foo, "(any) => number");
AssertType(x, "any");

AssertType(100, "int");
        return 100;
    }
}

