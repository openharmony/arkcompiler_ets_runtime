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

// === tests/cases/compiler/nestedBlockScopedBindings1.ts ===
declare function AssertType(value:any, type:string):void;
function a0() {
    {
        let x = 1;
AssertType(x, "number");
AssertType(1, "int");
    }
    {
        let x = 1;
AssertType(x, "number");
AssertType(1, "int");
    }
}

function a1() {
    {
        let x;
AssertType(x, "any");
    }
    {
        let x = 1;
AssertType(x, "number");
AssertType(1, "int");
    }
}

function a2() {
    {
        let x = 1;
AssertType(x, "number");
AssertType(1, "int");
    }
    {
        let x;
AssertType(x, "any");
    }
}

function a3() {
    {
        let x = 1;
AssertType(x, "number");
AssertType(1, "int");
    }
    switch (1) {
AssertType(1, "int");

        case 1:
AssertType(1, "int");

            let x;
AssertType(x, "any");

            break;
    }
}


function a4() {
    {
        let x;
AssertType(x, "any");
    }
    switch (1) {
AssertType(1, "int");

        case 1:
AssertType(1, "int");

            let x = 1;
AssertType(x, "number");
AssertType(1, "int");

            break;
    }
}


