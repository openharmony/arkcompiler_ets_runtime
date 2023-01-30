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

// === tests/cases/compiler/nestedBlockScopedBindings16.ts ===
declare function AssertType(value:any, type:string):void;
let x;
AssertType(x, "any");

for (; false;) {
    {
        let x;
AssertType(x, "any");

        () => x;
AssertType(() => x, "() => any");
AssertType(x, "any");
    }
}

let y;
AssertType(y, "any");

for (; false;) {
    {
        let y;
AssertType(y, "any");

        y = 1;
AssertType(y = 1, "int");
AssertType(y, "any");
AssertType(1, "int");
    }
}

let z0;
AssertType(z0, "any");

for (; false;) {
    switch (1){
AssertType(1, "int");

        case 1:
AssertType(1, "int");

            let z0;
AssertType(z0, "any");

            () => z0;
AssertType(() => z0, "() => any");
AssertType(z0, "any");

            break;
    }
}

let z;
AssertType(z, "any");

for (; false;) {
    switch (1){
AssertType(1, "int");

        case 1:
AssertType(1, "int");

            let z;
AssertType(z, "any");

            z = 1;
AssertType(z = 1, "int");
AssertType(z, "any");
AssertType(1, "int");

            break;
    }
}

