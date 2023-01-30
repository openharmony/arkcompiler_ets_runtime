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

// === tests/cases/compiler/modifierParenCast.ts ===
declare function AssertType(value:any, type:string):void;
let readonly: any = undefined;
AssertType(readonly, "any");
AssertType(undefined, "undefined");

let override: any = undefined;
AssertType(override, "any");
AssertType(undefined, "undefined");

let out: any = undefined;
AssertType(out, "any");
AssertType(undefined, "undefined");

let declare: any = undefined;
AssertType(declare, "any");
AssertType(undefined, "undefined");

export const a = (readonly as number);
AssertType(a, "number");
AssertType((readonly as number), "number");
AssertType(readonly as number, "number");
AssertType(readonly, "any");

export const b = (override as number);
AssertType(b, "number");
AssertType((override as number), "number");
AssertType(override as number, "number");
AssertType(override, "any");

export const c = (out as number);
AssertType(c, "number");
AssertType((out as number), "number");
AssertType(out as number, "number");
AssertType(out, "any");

export const d = (declare as number);
AssertType(d, "number");
AssertType((declare as number), "number");
AssertType(declare as number, "number");
AssertType(declare, "any");


