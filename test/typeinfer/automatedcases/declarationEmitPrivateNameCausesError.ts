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

// === tests/cases/compiler/file.ts ===
declare function AssertType(value:any, type:string):void;
const IGNORE_EXTRA_VARIABLES = Symbol(); //Notice how this is unexported
AssertType(IGNORE_EXTRA_VARIABLES, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

//This is exported
export function ignoreExtraVariables<CtorT extends {new(...args:any[]):{}}> (ctor : CtorT) {
AssertType(class extends ctor {        [IGNORE_EXTRA_VARIABLES] = true; //An unexported constant is used    }, "{ new (...any[]): (Anonymous class); prototype: ignoreExtraVariables<any>.(Anonymous class); } & CtorT");
AssertType(ctor, "{}");
    return class extends ctor {

        [IGNORE_EXTRA_VARIABLES] = true; //An unexported constant is used
AssertType([IGNORE_EXTRA_VARIABLES], "boolean");
AssertType(IGNORE_EXTRA_VARIABLES, "unique symbol");
AssertType(true, "boolean");

    };
}

