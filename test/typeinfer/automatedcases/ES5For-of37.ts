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

// === tests/cases/conformance/statements/for-ofStatements/ES5For-of37.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/30083

for (const i of [0, 1, 2, 3, 4]) {
    try {
        // Ensure catch binding for the following loop is reset per iteration:
        for (const j of [1, 2, 3]) {
AssertType(j, "number");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

            if (i === 2) {
AssertType(i === 2, "boolean");
AssertType(i, "number");
AssertType(2, "int");

                throw new Error('ERR');
AssertType(new Error('ERR'), "Error");
AssertType(Error, "ErrorConstructor");
AssertType('ERR', "string");
            }
        }
        console.log(i);
AssertType(console.log(i), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(i, "number");

    } catch (err) {
AssertType(err, "any");

        console.log('E %s %s', i, err);
AssertType(console.log('E %s %s', i, err), "void");
AssertType(console.log, "(...any[]) => void");
AssertType('E %s %s', "string");
AssertType(i, "number");
AssertType(err, "any");
    }
}

