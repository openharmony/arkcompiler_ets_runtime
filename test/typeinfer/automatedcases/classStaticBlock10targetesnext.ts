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

// === tests/cases/conformance/classes/classStaticBlock/classStaticBlock10.ts ===
declare function AssertType(value:any, type:string):void;
let a1 = 1;
AssertType(a1, "number");
AssertType(1, "int");

let a2 = 1;
AssertType(a2, "number");
AssertType(1, "int");

const b1 = 2;
AssertType(b1, "int");
AssertType(2, "int");

const b2 = 2;
AssertType(b2, "int");
AssertType(2, "int");

function f () {
    let a1 = 11;
AssertType(a1, "number");
AssertType(11, "int");

    const b1 = 22;
AssertType(b1, "int");
AssertType(22, "int");

    class C1 {
        static {
            let a1 = 111;
AssertType(a1, "number");
AssertType(111, "int");

            let a2 = 111;
AssertType(a2, "number");
AssertType(111, "int");

            const b1 = 222;
AssertType(b1, "int");
AssertType(222, "int");

            const b2 = 222;
AssertType(b2, "int");
AssertType(222, "int");
        }
    }
}

class C2 {
    static {
        let a1 = 111;
AssertType(a1, "number");
AssertType(111, "int");

        let a2 = 111;
AssertType(a2, "number");
AssertType(111, "int");

        const b1 = 222;
AssertType(b1, "int");
AssertType(222, "int");

        const b2 = 222;
AssertType(b2, "int");
AssertType(222, "int");
    }
}


