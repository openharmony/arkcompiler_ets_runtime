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

// === tests/cases/compiler/recursiveReverseMappedType.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #38198

type Recur<T> = (
    T extends (unknown[]) ? {} : { [K in keyof T]?: Recur<T[K]> }
) | ['marker', ...Recur<T>[]];

function join<T>(l: Recur<T>[]): Recur<T> {
AssertType(['marker', ...l], "["marker", ...Recur<T>[]]");
AssertType('marker', "string");
AssertType(...l, "Recur<T>");
AssertType(l, "Recur<T>[]");
    return ['marker', ...l];
}

function a<T>(l: Recur<T>[]): void {
    const x: Recur<T> | undefined = join(l);
AssertType(x, "union");
AssertType(join(l), "Recur<T>");
AssertType(join, "<T>(Recur<T>[]) => Recur<T>");
AssertType(l, "Recur<T>[]");
}


