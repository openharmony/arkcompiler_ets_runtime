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

// === tests/cases/conformance/types/conditional/inferTypes2.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #22755

export declare function foo<T>(obj: T): T extends () => infer P ? P : never;
export function bar<T>(obj: T) {
AssertType(foo(obj), "T extends () => infer P ? P : never");
AssertType(foo, "<T>(T) => T extends () => infer P ? P : never");
AssertType(obj, "T");
    return foo(obj);
}

export type BadNested<T> = { x: T extends number ? T : string };

export declare function foo2<T>(obj: T): T extends { [K in keyof BadNested<infer P>]: BadNested<infer P>[K] } ? P : never;
export function bar2<T>(obj: T) {
AssertType(foo2(obj), "T extends { x: infer P extends number ? infer P : string; } ? P : never");
AssertType(foo2, "<T>(T) => T extends { x: infer P extends number ? infer P : string; } ? P : never");
AssertType(obj, "T");
    return foo2(obj);
}

// Repros from #31099

type Weird = any extends infer U ? U : never;
type AlsoWeird = unknown extends infer U ? U : never;

const a: Weird = null;
AssertType(a, "any");
AssertType(null, "null");

const b: string = a;
AssertType(b, "string");
AssertType(a, "any");


