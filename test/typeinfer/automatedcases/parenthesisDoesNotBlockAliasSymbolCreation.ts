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

// === tests/cases/compiler/parenthesisDoesNotBlockAliasSymbolCreation.ts ===
declare function AssertType(value:any, type:string):void;
export type InvalidKeys<K extends string|number|symbol> = { [P in K]? : never };
export type InvalidKeys2<K extends string|number|symbol> = (
    { [P in K]? : never }
);

export type A<T> = (
    T & InvalidKeys<"a">
);
export type A2<T> = (
    T & InvalidKeys2<"a">
);

export const a = null as A<{ x : number }>;
AssertType(a, "A<{ x: number; }>");
AssertType(null as A<{ x : number }>, "A<{ x: number; }>");
AssertType(null, "null");
AssertType(x, "number");

export const a2 = null as A2<{ x : number }>;
AssertType(a2, "A2<{ x: number; }>");
AssertType(null as A2<{ x : number }>, "A2<{ x: number; }>");
AssertType(null, "null");
AssertType(x, "number");

export const a3 = null as { x : number } & InvalidKeys<"a">;
AssertType(a3, "{ x: number; } & InvalidKeys<"a">");
AssertType(null as { x : number } & InvalidKeys<"a">, "{ x: number; } & InvalidKeys<"a">");
AssertType(null, "null");
AssertType(x, "number");

export const a4 = null as { x : number } & InvalidKeys2<"a">;
AssertType(a4, "{ x: number; } & InvalidKeys2<"a">");
AssertType(null as { x : number } & InvalidKeys2<"a">, "{ x: number; } & InvalidKeys2<"a">");
AssertType(null, "null");
AssertType(x, "number");


