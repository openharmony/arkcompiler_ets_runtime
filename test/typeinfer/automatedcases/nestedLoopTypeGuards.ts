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

// === tests/cases/compiler/nestedLoopTypeGuards.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #10378

function f1() {
    let a: boolean | number | string;
AssertType(a, "union");

    if (typeof a !== 'boolean') {
AssertType(typeof a !== 'boolean', "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType('boolean', "string");

        // a is narrowed to "number | string"
        for (let i = 0; i < 1; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 1, "boolean");
AssertType(i, "number");
AssertType(1, "int");
AssertType(i++, "number");
AssertType(i, "number");

            for (let j = 0; j < 1; j++) {
AssertType(j, "number");

AssertType(0, "int");

AssertType(j < 1, "boolean");

AssertType(j, "number");

AssertType(1, "int");

AssertType(j++, "number");

AssertType(j, "number");
}

            if (typeof a === 'string') {
AssertType(typeof a === 'string', "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType('string', "string");

                // a is narrowed to "string'
                for (let j = 0; j < 1; j++) {
AssertType(j, "number");
AssertType(0, "int");
AssertType(j < 1, "boolean");
AssertType(j, "number");
AssertType(1, "int");
AssertType(j++, "number");
AssertType(j, "number");

                    a.length; // Should not error here
AssertType(a.length, "number");
                }
            }
        }
    }
}

function f2() {
    let a: string | number;
AssertType(a, "union");

    if (typeof a === 'string') {
AssertType(typeof a === 'string', "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType('string', "string");

        while (1) {
AssertType(1, "int");

            while (1) {
AssertType(1, "int");
}

            if (typeof a === 'string') {
AssertType(typeof a === 'string', "boolean");
AssertType(typeof a, "union");
AssertType(a, "string");
AssertType('string', "string");

                while (1) {
AssertType(1, "int");

                    a.length; // Should not error here
AssertType(a.length, "number");
                }
            }
        }
    }
}

