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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/assignmentCompatWithObjectMembers2.ts ===
declare function AssertType(value:any, type:string):void;
// members N and M of types S and T have the same name, same accessibility, same optionality, and N is assignable M
// additional optional properties do not cause errors

class S { foo: string; }
class T { foo: string; }
let s: S;
AssertType(s, "S");

let t: T;
AssertType(t, "T");

interface S2 { foo: string; bar?: string }
interface T2 { foo: string; baz?: string }
let s2: S2;
AssertType(s2, "S2");

let t2: T2;
AssertType(t2, "T2");

let a: { foo: string; bar?: string 
AssertType(a, "{ foo: string; bar?: string; }");

AssertType(foo, "string");

AssertType(bar, "string");
}

let b: { foo: string; baz?: string 
AssertType(b, "{ foo: string; baz?: string; }");

AssertType(foo, "string");

AssertType(baz, "string");
}

let a2 = { foo: '' };
AssertType(a2, "{ foo: string; }");
AssertType({ foo: '' }, "{ foo: string; }");
AssertType(foo, "string");
AssertType('', "string");

let b2 = { foo: '' };
AssertType(b2, "{ foo: string; }");
AssertType({ foo: '' }, "{ foo: string; }");
AssertType(foo, "string");
AssertType('', "string");

s = t;
AssertType(s = t, "T");
AssertType(s, "S");
AssertType(t, "T");

t = s;
AssertType(t = s, "S");
AssertType(t, "T");
AssertType(s, "S");

s = s2;
AssertType(s = s2, "S2");
AssertType(s, "S");
AssertType(s2, "S2");

s = a2;
AssertType(s = a2, "{ foo: string; }");
AssertType(s, "S");
AssertType(a2, "{ foo: string; }");

s2 = t2;
AssertType(s2 = t2, "T2");
AssertType(s2, "S2");
AssertType(t2, "T2");

t2 = s2;
AssertType(t2 = s2, "S2");
AssertType(t2, "T2");
AssertType(s2, "S2");

s2 = t;
AssertType(s2 = t, "T");
AssertType(s2, "S2");
AssertType(t, "T");

s2 = b;
AssertType(s2 = b, "{ foo: string; baz?: string; }");
AssertType(s2, "S2");
AssertType(b, "{ foo: string; baz?: string; }");

s2 = a2;
AssertType(s2 = a2, "{ foo: string; }");
AssertType(s2, "S2");
AssertType(a2, "{ foo: string; }");

a = b;
AssertType(a = b, "{ foo: string; baz?: string; }");
AssertType(a, "{ foo: string; bar?: string; }");
AssertType(b, "{ foo: string; baz?: string; }");

b = a;
AssertType(b = a, "{ foo: string; bar?: string; }");
AssertType(b, "{ foo: string; baz?: string; }");
AssertType(a, "{ foo: string; bar?: string; }");

a = s;
AssertType(a = s, "S");
AssertType(a, "{ foo: string; bar?: string; }");
AssertType(s, "S");

a = s2;
AssertType(a = s2, "S2");
AssertType(a, "{ foo: string; bar?: string; }");
AssertType(s2, "S2");

a = a2;
AssertType(a = a2, "{ foo: string; }");
AssertType(a, "{ foo: string; bar?: string; }");
AssertType(a2, "{ foo: string; }");

a2 = b2;
AssertType(a2 = b2, "{ foo: string; }");
AssertType(a2, "{ foo: string; }");
AssertType(b2, "{ foo: string; }");

b2 = a2;
AssertType(b2 = a2, "{ foo: string; }");
AssertType(b2, "{ foo: string; }");
AssertType(a2, "{ foo: string; }");

a2 = b;
AssertType(a2 = b, "{ foo: string; baz?: string; }");
AssertType(a2, "{ foo: string; }");
AssertType(b, "{ foo: string; baz?: string; }");

a2 = t2;
AssertType(a2 = t2, "T2");
AssertType(a2, "{ foo: string; }");
AssertType(t2, "T2");

a2 = t;
AssertType(a2 = t, "T");
AssertType(a2, "{ foo: string; }");
AssertType(t, "T");


