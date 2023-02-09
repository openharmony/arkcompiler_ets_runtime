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

// === tests/cases/compiler/collisionRestParameterInType.ts ===
declare function AssertType(value:any, type:string):void;
let v1: (_i: number, ...restParameters) => void; // no error - no code gen
AssertType(v1, "(number, ...any[]) => void");
AssertType(_i, "number");
AssertType(restParameters, "any[]");

let v2: {
AssertType(v2, "{ (number, ...any[]): any; new (number, ...any[]): any; foo(number, ...any[]): any; prop: (number, ...any[]) => void; }");

    (_i: number, ...restParameters); // no error - no code gen
AssertType(_i, "number");
AssertType(restParameters, "any[]");

    new (_i: number, ...restParameters); // no error - no code gen
AssertType(_i, "number");
AssertType(restParameters, "any[]");

    foo(_i: number, ...restParameters); // no error - no code gen
AssertType(foo, "(number, ...any[]) => any");
AssertType(_i, "number");
AssertType(restParameters, "any[]");

    prop: (_i: number, ...restParameters) => void; // no error - no code gen
AssertType(prop, "(number, ...any[]) => void");
AssertType(_i, "number");
AssertType(restParameters, "any[]");
}

