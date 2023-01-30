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

// === tests/cases/compiler/optionalParameterRetainsNull.ts ===
declare function AssertType(value:any, type:string):void;
interface Bar {  bar: number; foo: object | null;  }

let a = {
AssertType(a, "{ test<K extends keyof Bar>(K, ?union): void; }");
AssertType({  test<K extends keyof Bar> (a: K,  b?: Bar[K]  |  null)  { }}, "{ test<K extends keyof Bar>(K, ?union): void; }");

  test<K extends keyof Bar> (a: K,  b?: Bar[K]  |  null)  { 
AssertType(test, "<K extends keyof Bar>(K, ?union) => void");

AssertType(a, "K");

AssertType(b, "union");

AssertType(null, "null");
}

};
a.test("bar", null); // ok, null is assignable to number | null | undefined
AssertType(a.test("bar", null), "void");
AssertType(a.test, "<K extends keyof Bar>(K, ?union) => void");
AssertType("bar", "string");
AssertType(null, "null");


