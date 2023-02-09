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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithOptionalProperties.ts ===
declare function AssertType(value:any, type:string):void;
// subtyping is not transitive due to optional properties but the subtyping algorithm assumes it is for the 99% case

// returns { s?: number; }
function f<T>(a: T) {
    let b: { s?: number } = a;
AssertType(b, "{ s?: number; }");
AssertType(s, "number");
AssertType(a, "T");

AssertType(b, "{ s?: number; }");
    return b;
}

let r = f({ s: new Object() }); // ok
AssertType(r, "{ s?: number; }");
AssertType(f({ s: new Object() }), "{ s?: number; }");
AssertType(f, "<T>(T) => { s?: number; }");
AssertType({ s: new Object() }, "{ s: Object; }");
AssertType(s, "Object");
AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");

r.s && r.s.toFixed(); // would blow up at runtime
AssertType(r.s && r.s.toFixed(), "string");
AssertType(r.s, "number");
AssertType(r.s.toFixed(), "string");
AssertType(r.s.toFixed, "(?number) => string");


