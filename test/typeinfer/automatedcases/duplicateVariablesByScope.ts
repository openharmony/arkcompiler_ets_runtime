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

// === tests/cases/compiler/duplicateVariablesByScope.ts ===
declare function AssertType(value:any, type:string):void;
// duplicate local letiables are only reported at global scope

module M {
    for (let j = 0; j < 10; j++) {
    }

    for (let j = 0; j < 10; j++) {
    }
}

function foo() {
    let x = 2;
AssertType(x, "number");
AssertType(2, "int");

    let x = 1;
AssertType(x, "number");
AssertType(1, "int");

    if (true) {
AssertType(true, "boolean");

        let result = 1;
AssertType(result, "number");
AssertType(1, "int");
    }
    else {
        let result = 2;
AssertType(result, "number");
AssertType(2, "int");
    }
}

class C {
    foo() {
        try {
            let x = 1;
AssertType(x, "number");
AssertType(1, "int");
        }
        catch (e) {
AssertType(e, "any");

            let x = 2;
AssertType(x, "number");
AssertType(2, "int");
        }
    }
}

