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

// === tests/cases/compiler/nestedLoops.ts ===
declare function AssertType(value:any, type:string):void;
export class Test  {
    constructor() {

        let outerArray: Array<number> = [1, 2, 3];
AssertType(outerArray, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

        let innerArray: Array<number> = [1, 2, 3];
AssertType(innerArray, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

        for (let outer of outerArray)
AssertType(outer, "number");
AssertType(outerArray, "number[]");

            for (let inner of innerArray) {
AssertType(inner, "number");
AssertType(innerArray, "number[]");

                this.aFunction((newValue, oldValue) => {
AssertType(this.aFunction((newValue, oldValue) => {                    let x = outer + inner + newValue;                }), "void");
AssertType(this.aFunction, "((any, any) => void) => void");
AssertType(this, "this");
AssertType((newValue, oldValue) => {                    let x = outer + inner + newValue;                }, "(any, any) => void");
AssertType(newValue, "any");
AssertType(oldValue, "any");

                    let x = outer + inner + newValue;
AssertType(x, "any");
AssertType(outer + inner + newValue, "any");
AssertType(outer + inner, "number");
AssertType(outer, "number");
AssertType(inner, "number");
AssertType(newValue, "any");

                });
            }
    }

    public aFunction(func: (newValue: any, oldValue: any) => void): void {
    }
}

