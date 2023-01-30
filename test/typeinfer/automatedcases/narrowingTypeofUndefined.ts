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

// === tests/cases/compiler/narrowingTypeofUndefined.ts ===
declare function AssertType(value:any, type:string):void;
declare const a: { error: { prop: string }, result: undefined } | { error: undefined, result: { prop: number } 
AssertType(a, "union");

AssertType(error, "{ prop: string; }");

AssertType(prop, "string");

AssertType(result, "undefined");

AssertType(error, "undefined");

AssertType(result, "{ prop: number; }");

AssertType(prop, "number");
}

if (typeof a.error === 'undefined') {
    a.result.prop; // number
AssertType(a.result.prop, "number");
AssertType(a.result, "{ prop: number; }");
}
else {
    a.error.prop; // string
AssertType(a.error.prop, "string");
AssertType(a.error, "{ prop: string; }");
}

if (typeof a.error !== 'undefined') {
    a.error.prop; // string
AssertType(a.error.prop, "string");
AssertType(a.error, "{ prop: string; }");
}
else {
    a.result.prop; // number
AssertType(a.result.prop, "number");
AssertType(a.result, "{ prop: number; }");
}


