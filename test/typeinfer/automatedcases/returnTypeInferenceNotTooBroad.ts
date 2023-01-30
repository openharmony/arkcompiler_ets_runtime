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

// === tests/cases/compiler/returnTypeInferenceNotTooBroad.ts ===
declare function AssertType(value:any, type:string):void;
type Signs = { kind: 'a'; a: 3; } | { kind: 'b'; b: 2; } | { kind: 'c'; c: 1; };
interface Opts<T> {
    low?: number;
    sign?: T
}
interface Wrapper<T> {
}
declare function sepsis<T extends Signs>(opts: Opts<T>): Wrapper<T>;
declare function unwrap<T>(w: Wrapper<T>): T;
export const y = sepsis({ low: 1, sign: { kind: 'a', a: 3 }});
AssertType(y, "Wrapper<{ kind: "a"; a: 3; }>");
AssertType(sepsis({ low: 1, sign: { kind: 'a', a: 3 }}), "Wrapper<{ kind: "a"; a: 3; }>");
AssertType(sepsis, "<T extends Signs>(Opts<T>) => Wrapper<T>");
AssertType({ low: 1, sign: { kind: 'a', a: 3 }}, "{ low: number; sign: { kind: "a"; a: 3; }; }");
AssertType(low, "number");
AssertType(1, "int");
AssertType(sign, "{ kind: "a"; a: 3; }");
AssertType({ kind: 'a', a: 3 }, "{ kind: "a"; a: 3; }");
AssertType(kind, "string");
AssertType('a', "string");
AssertType(a, "int");
AssertType(3, "int");

// $ExpectType { kind: "a"; a: 3; }
export const yun = unwrap(y);
AssertType(yun, "{ kind: "a"; a: 3; }");
AssertType(unwrap(y), "{ kind: "a"; a: 3; }");
AssertType(unwrap, "<T>(Wrapper<T>) => T");
AssertType(y, "Wrapper<{ kind: "a"; a: 3; }>");

// $ExpectType { kind: "a"; a: 3; }
export const yone = unwrap(sepsis({ low: 1, sign: { kind: 'a', a: 3 }}));
AssertType(yone, "{ kind: "a"; a: 3; }");
AssertType(unwrap(sepsis({ low: 1, sign: { kind: 'a', a: 3 }})), "{ kind: "a"; a: 3; }");
AssertType(unwrap, "<T>(Wrapper<T>) => T");
AssertType(sepsis({ low: 1, sign: { kind: 'a', a: 3 }}), "Wrapper<{ kind: "a"; a: 3; }>");
AssertType(sepsis, "<T extends Signs>(Opts<T>) => Wrapper<T>");
AssertType({ low: 1, sign: { kind: 'a', a: 3 }}, "{ low: number; sign: { kind: "a"; a: 3; }; }");
AssertType(low, "number");
AssertType(1, "int");
AssertType(sign, "{ kind: "a"; a: 3; }");
AssertType({ kind: 'a', a: 3 }, "{ kind: "a"; a: 3; }");
AssertType(kind, "string");
AssertType('a', "string");
AssertType(a, "int");
AssertType(3, "int");


