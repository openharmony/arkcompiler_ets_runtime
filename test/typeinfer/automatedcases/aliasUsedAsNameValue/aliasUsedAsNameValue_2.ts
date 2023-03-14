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

// === tests/cases/compiler/aliasUsedAsNameValue_2.ts ===
declare function AssertType(value:any, type:string):void;
///<reference path='aliasUsedAsNameValue_0.ts' />
///<reference path='aliasUsedAsNameValue_1.ts' />
import * as mod from "./aliasUsedAsNameValue_0";
import * as b from "./aliasUsedAsNameValue_1";
 
export let a = function () {
AssertType(a, "() => void");
AssertType(function () {    //let x = mod.id; // TODO needed hack that mod is loaded    b.b(mod);}, "() => void");

    b.b(mod);
AssertType(b.b(mod), "any");
AssertType(b.b, "(any) => any");
AssertType(mod, "typeof mod");
}


