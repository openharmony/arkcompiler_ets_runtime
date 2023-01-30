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

// === tests/cases/compiler/inferentialTypingObjectLiteralMethod1.ts ===
declare function AssertType(value:any, type:string):void;
interface Int<T, U> {
    method(x: T): U;
}
declare function foo<T, U>(x: T, y: Int<T, U>, z: Int<U, T>): T;
foo("", { method(p1) { 
AssertType(foo("", { method(p1) { return p1.length } }, { method(p2) { return undefined } }), "string");
AssertType(foo, "<T, U>(T, Int<T, U>, Int<U, T>) => T");
AssertType("", "string");
AssertType({ method(p1) { return p1.length } }, "{ method(string): number; }");
AssertType(method, "(string) => number");
AssertType(p1, "string");
AssertType(p1.length, "number");
AssertType({ method(p2) { return undefined } }, "{ method(number): any; }");
AssertType(method, "(number) => any");
AssertType(p2, "number");
AssertType(undefined, "undefined");
return p1.length } }, { method(p2) { return undefined } });


