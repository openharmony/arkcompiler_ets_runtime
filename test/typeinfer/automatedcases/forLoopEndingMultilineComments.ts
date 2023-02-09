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

// === tests/cases/compiler/forLoopEndingMultilineComments.ts ===
declare function AssertType(value:any, type:string):void;
declare let a: any;
AssertType(a, "any");

export function consoleTestResultHandler(testResult: any): boolean {
    // needed to get colors to show up when passing through Grunt
    void a;
AssertType(void a, "undefined");
AssertType(a, "any");

    for (const q of a) {
AssertType(q, "any");
AssertType(a, "any");

        void a;
AssertType(void a, "undefined");
AssertType(a, "any");

        /* eslint-disable no-console */
        if (a) {
AssertType(a, "any");

        } else {
        }
        /* eslint-enable no-console */
    }

AssertType(true, "boolean");
    return true;
}

