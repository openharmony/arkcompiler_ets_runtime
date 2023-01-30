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

// === tests/cases/compiler/checkSwitchStatementIfCaseTypeIsString.ts ===
declare function AssertType(value:any, type:string):void;
declare function use(a: any): void;

class A {
    doIt(x: Array<string>): void {
        x.forEach((v) => {
AssertType(x.forEach((v) => {            switch(v) {                case "test": use(this);            }        }), "void");
AssertType(x.forEach, "((string, number, string[]) => void, ?any) => void");
AssertType((v) => {            switch(v) {                case "test": use(this);            }        }, "(string) => void");
AssertType(v, "string");

            switch(v) {
AssertType(v, "string");

                case "test": use(this);
AssertType("test", "string");
AssertType(use(this), "void");
AssertType(use, "(any) => void");
AssertType(this, "this");
            }
        });
    }
}

