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

// === tests/cases/compiler/voidReturnIndexUnionInference.ts ===
declare function AssertType(value:any, type:string):void;
// repro from https://github.com/Microsoft/TypeScript/issues/25274
export function safeInvoke<A1, R>(
    func: ((arg1: A1) => R) | null | undefined,
    arg1: A1
): R | undefined {
    if (func) {
AssertType(func, "union");

AssertType(func(arg1), "R");
AssertType(func, "(A1) => R");
AssertType(arg1, "A1");
        return func(arg1);

    } else {
AssertType(undefined, "undefined");
        return undefined;
    }
}

interface Props {
    onFoo?(value: string): boolean;
    onBar?(value: string): void;
}

function bad<P extends Props>(props: Readonly<P>) {
    safeInvoke(props.onFoo, "blah");
AssertType(safeInvoke(props.onFoo, "blah"), "union");
AssertType(safeInvoke, "<A1, R>(union, A1) => union");
AssertType(props.onFoo, "union");
AssertType("blah", "string");

    // ERROR HERE!!!
    // Type R in signature of safeInvoke incorrectly inferred as {} instead of void!
    safeInvoke(props.onBar, "blah");
AssertType(safeInvoke(props.onBar, "blah"), "union");
AssertType(safeInvoke, "<A1, R>(union, A1) => union");
AssertType(props.onBar, "union");
AssertType("blah", "string");
}


