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

// === tests/cases/conformance/types/contextualTypes/partiallyAnnotatedFunction/partiallyAnnotatedFunctionInferenceWithTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
class C {
  test: string
}

class D extends C {
  test2: string
}

declare function test<T extends C>(a: (t: T, t1: T) => void): T

declare function testRest<T extends C>(a: (t: T, t1: T, ...ts: T[]) => void): T


// exactly
test((t1: D, t2) => { t2.test2 })
AssertType(test((t1: D, t2) => { t2.test2 }), "D");
AssertType(test, "<T extends C>((T, T) => void) => T");
AssertType((t1: D, t2) => { t2.test2 }, "(D, D) => void");
AssertType(t1, "D");
AssertType(t2, "D");
AssertType(t2.test2, "string");

test((t1, t2: D) => { t2.test2 })
AssertType(test((t1, t2: D) => { t2.test2 }), "D");
AssertType(test, "<T extends C>((T, T) => void) => T");
AssertType((t1, t2: D) => { t2.test2 }, "(D, D) => void");
AssertType(t1, "D");
AssertType(t2, "D");
AssertType(t2.test2, "string");

// zero arg
test(() => {})
AssertType(test(() => {}), "C");
AssertType(test, "<T extends C>((T, T) => void) => T");
AssertType(() => {}, "() => void");

// fewer args
test((t1: D) => {})
AssertType(test((t1: D) => {}), "D");
AssertType(test, "<T extends C>((T, T) => void) => T");
AssertType((t1: D) => {}, "(D) => void");
AssertType(t1, "D");

// rest arg
test((...ts: D[]) => {})
AssertType(test((...ts: D[]) => {}), "D");
AssertType(test, "<T extends C>((T, T) => void) => T");
AssertType((...ts: D[]) => {}, "(...D[]) => void");
AssertType(ts, "D[]");

// source function has rest arg
testRest((t1: D) => {})
AssertType(testRest((t1: D) => {}), "D");
AssertType(testRest, "<T extends C>((T, T, ...T[]) => void) => T");
AssertType((t1: D) => {}, "(D) => void");
AssertType(t1, "D");

testRest((t1, t2, t3) => {})
AssertType(testRest((t1, t2, t3) => {}), "C");
AssertType(testRest, "<T extends C>((T, T, ...T[]) => void) => T");
AssertType((t1, t2, t3) => {}, "(C, C, C) => void");
AssertType(t1, "C");
AssertType(t2, "C");
AssertType(t3, "C");

testRest((t1: D, t2, t3) => {})
AssertType(testRest((t1: D, t2, t3) => {}), "D");
AssertType(testRest, "<T extends C>((T, T, ...T[]) => void) => T");
AssertType((t1: D, t2, t3) => {}, "(D, D, D) => void");
AssertType(t1, "D");
AssertType(t2, "D");
AssertType(t3, "D");

testRest((t1, t2: D, t3) => {})
AssertType(testRest((t1, t2: D, t3) => {}), "D");
AssertType(testRest, "<T extends C>((T, T, ...T[]) => void) => T");
AssertType((t1, t2: D, t3) => {}, "(D, D, D) => void");
AssertType(t1, "D");
AssertType(t2, "D");
AssertType(t3, "D");

testRest((t2: D, ...t3) => {})
AssertType(testRest((t2: D, ...t3) => {}), "D");
AssertType(testRest, "<T extends C>((T, T, ...T[]) => void) => T");
AssertType((t2: D, ...t3) => {}, "(D, D, ...D[]) => void");
AssertType(t2, "D");
AssertType(t3, "[t1: D, ...ts: D[]]");

testRest((t2, ...t3: D[]) => {})
AssertType(testRest((t2, ...t3: D[]) => {}), "C");
AssertType(testRest, "<T extends C>((T, T, ...T[]) => void) => T");
AssertType((t2, ...t3: D[]) => {}, "(C, ...D[]) => void");
AssertType(t2, "C");
AssertType(t3, "D[]");


