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

// === tests/cases/compiler/doubleUnderscoreLabels.ts ===
declare function AssertType(value:any, type:string):void;
function doThing() {
    __call: while (true) {
AssertType(__call, "any");
AssertType(true, "boolean");

        aLabel: for (let i = 0; i < 10; i++) {
AssertType(aLabel, "any");
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 10, "boolean");
AssertType(i, "number");
AssertType(10, "int");
AssertType(i++, "number");
AssertType(i, "number");

            if (i === 3) {
AssertType(i === 3, "boolean");
AssertType(i, "number");
AssertType(3, "int");

                break __call;
AssertType(__call, "any");
            }
            if (i === 5) {
AssertType(i === 5, "boolean");
AssertType(i, "number");
AssertType(5, "int");

                break aLabel;
AssertType(aLabel, "any");
            }
        }
    }
}
doThing();
AssertType(doThing(), "void");
AssertType(doThing, "() => void");


