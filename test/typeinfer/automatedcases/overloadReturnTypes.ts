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

// === tests/cases/compiler/overloadReturnTypes.ts ===
declare function AssertType(value:any, type:string):void;
class Accessor {}

function attr(name: string): string;
function attr(name: string, value: string): Accessor;
function attr(map: any): Accessor;
function attr(nameOrMap: any, value?: string): any {
    if (nameOrMap && typeof nameOrMap === "object") {
AssertType(nameOrMap && typeof nameOrMap === "object", "boolean");
AssertType(nameOrMap, "any");
AssertType(typeof nameOrMap === "object", "boolean");
AssertType(typeof nameOrMap, "union");
AssertType(nameOrMap, "any");
AssertType("object", "string");

        // handle map case
AssertType(new Accessor, "Accessor");
AssertType(Accessor, "typeof Accessor");
        return new Accessor;
    }
    else {
        // handle string case
AssertType("s", "string");
        return "s";
    }
}


interface IFace {
	attr(name:string):string;
	attr(name: string, value: string): Accessor;
	attr(map: any): Accessor;
}


