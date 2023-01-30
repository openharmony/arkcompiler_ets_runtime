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

// === tests/cases/compiler/tupleTypeInference2.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #22564

type A<R> = [R] | [R, string];
declare function f<T>(x: A<T>): T;
f([undefined, ''] as [never, string]); // T: never
AssertType(f([undefined, ''] as [never, string]), "never");
AssertType(f, "<T>(A<T>) => T");
AssertType([undefined, ''] as [never, string], "[never, string]");
AssertType([undefined, ''], "[undefined, string]");
AssertType(undefined, "undefined");
AssertType('', "string");

f([undefined, ''] as [void, string]); // T: void
AssertType(f([undefined, ''] as [void, string]), "void");
AssertType(f, "<T>(A<T>) => T");
AssertType([undefined, ''] as [void, string], "[void, string]");
AssertType([undefined, ''], "[undefined, string]");
AssertType(undefined, "undefined");
AssertType('', "string");

// Repro from #22563

type B<R, S> = [R] | [R, S];
declare function g<T, U>(f: B<T, U>): U;
g([[]] as [void[]]); // U: {
AssertType(g([[]] as [void[]]), "unknown");

AssertType(g, "<T, U>(B<T, U>) => U");

AssertType([[]] as [void[]], "[void[]]");

AssertType([[]], "[never[]]");

AssertType([], "never[]");
}

type C<R, S> = [R[]] | [R[], S];
declare function h<T, U>(f: C<T, U>): U;
h([[]] as [void[]]); // U: {
AssertType(h([[]] as [void[]]), "unknown");

AssertType(h, "<T, U>(C<T, U>) => U");

AssertType([[]] as [void[]], "[void[]]");

AssertType([[]], "[never[]]");

AssertType([], "never[]");
}

// Repro from #22562

type C2<R> = [R[]] | [R[], void];
declare function h2<T>(f: C2<T>): T;
h2([[]] as [never[]]); // T: never
AssertType(h2([[]] as [never[]]), "never");
AssertType(h2, "<T>(C2<T>) => T");
AssertType([[]] as [never[]], "[never[]]");
AssertType([[]], "[never[]]");
AssertType([], "never[]");

h2([[]] as [void[]]); // T: void
AssertType(h2([[]] as [void[]]), "void");
AssertType(h2, "<T>(C2<T>) => T");
AssertType([[]] as [void[]], "[void[]]");
AssertType([[]], "[never[]]");
AssertType([], "never[]");


