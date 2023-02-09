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

// === tests/cases/compiler/controlFlowCommaExpressionAssertionWithinTernary.ts ===
declare function AssertType(value:any, type:string):void;
declare function assert(value: any): asserts value;

function foo2(param: number | null | undefined): number | null {
    const val = param !== undefined;
AssertType(val, "boolean");
AssertType(param !== undefined, "boolean");
AssertType(param, "union");
AssertType(undefined, "undefined");

AssertType(val ? (assert(param !== undefined), param) : null, "union");
AssertType(val, "boolean");
AssertType((assert(param !== undefined), param), "union");
AssertType(assert(param !== undefined), param, "union");
AssertType(assert(param !== undefined), "void");
AssertType(assert, "(any) => asserts value");
AssertType(param !== undefined, "boolean");
AssertType(param, "union");
AssertType(undefined, "undefined");
AssertType(param, "union");
AssertType(null, "null");
    return val ? (assert(param !== undefined), param) : null;

    // ^^^^^ Still typed as number | null | undefined
}

