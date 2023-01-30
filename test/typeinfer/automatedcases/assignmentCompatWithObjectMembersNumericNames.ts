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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/assignmentCompatWithObjectMembersNumericNames.ts ===
declare function AssertType(value:any, type:string):void;
// members N and M of types S and T have the same name, same accessibility, same optionality, and N is assignable M
// numeric named properties work correctly, no errors expected

class S { 1: string; }
class T { 1.: string; }
let s: S;
AssertType(s, "S");

let t: T;
AssertType(t, "T");

interface S2 { 1: string; bar?: string }
interface T2 { 1.0: string; baz?: string }
let s2: S2;
AssertType(s2, "S2");

let t2: T2;
AssertType(t2, "T2");

let a: { 1.: string; bar?: string 
AssertType(a, "{ 1: string; bar?: string; }");

AssertType(1., "string");

AssertType(bar, "string");
}

let b: { 1.0: string; baz?: string 
AssertType(b, "{ 1: string; baz?: string; }");

AssertType(1.0, "string");

AssertType(baz, "string");
}

let a2 = { 1.0: '' };
AssertType(a2, "{ 1: string; }");
AssertType({ 1.0: '' }, "{ 1: string; }");
AssertType(1.0, "string");
AssertType('', "string");

let b2 = { 1: '' };
AssertType(b2, "{ 1: string; }");
AssertType({ 1: '' }, "{ 1: string; }");
AssertType(1, "string");
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
AssertType(s = a2, "{ 1: string; }");
AssertType(s, "S");
AssertType(a2, "{ 1: string; }");

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
AssertType(s2 = b, "{ 1: string; baz?: string; }");
AssertType(s2, "S2");
AssertType(b, "{ 1: string; baz?: string; }");

s2 = a2;
AssertType(s2 = a2, "{ 1: string; }");
AssertType(s2, "S2");
AssertType(a2, "{ 1: string; }");

a = b;
AssertType(a = b, "{ 1: string; baz?: string; }");
AssertType(a, "{ 1: string; bar?: string; }");
AssertType(b, "{ 1: string; baz?: string; }");

b = a;
AssertType(b = a, "{ 1: string; bar?: string; }");
AssertType(b, "{ 1: string; baz?: string; }");
AssertType(a, "{ 1: string; bar?: string; }");

a = s;
AssertType(a = s, "S");
AssertType(a, "{ 1: string; bar?: string; }");
AssertType(s, "S");

a = s2;
AssertType(a = s2, "S2");
AssertType(a, "{ 1: string; bar?: string; }");
AssertType(s2, "S2");

a = a2;
AssertType(a = a2, "{ 1: string; }");
AssertType(a, "{ 1: string; bar?: string; }");
AssertType(a2, "{ 1: string; }");

a2 = b2;
AssertType(a2 = b2, "{ 1: string; }");
AssertType(a2, "{ 1: string; }");
AssertType(b2, "{ 1: string; }");

b2 = a2;
AssertType(b2 = a2, "{ 1: string; }");
AssertType(b2, "{ 1: string; }");
AssertType(a2, "{ 1: string; }");

a2 = b;
AssertType(a2 = b, "{ 1: string; baz?: string; }");
AssertType(a2, "{ 1: string; }");
AssertType(b, "{ 1: string; baz?: string; }");

a2 = t2;
AssertType(a2 = t2, "T2");
AssertType(a2, "{ 1: string; }");
AssertType(t2, "T2");

a2 = t;
AssertType(a2 = t, "T");
AssertType(a2, "{ 1: string; }");
AssertType(t, "T");


