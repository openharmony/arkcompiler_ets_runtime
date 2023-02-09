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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/assignmentCompatWithGenericCallSignatures.ts ===
declare function AssertType(value:any, type:string):void;
// some complex cases of assignment compat of generic signatures that stress contextual signature instantiation

let f: <S extends { p: string }[]>(x: S) => void
AssertType(f, "<S extends { p: string; }[]>(S) => void");
AssertType(p, "string");
AssertType(x, "S");

let g: <T extends { p: string }>(x: T[]) => void
AssertType(g, "<T extends { p: string; }>(T[]) => void");
AssertType(p, "string");
AssertType(x, "T[]");

f = g; // ok
AssertType(f = g, "<T extends { p: string; }>(T[]) => void");
AssertType(f, "<S extends { p: string; }[]>(S) => void");
AssertType(g, "<T extends { p: string; }>(T[]) => void");

g = f; // ok
AssertType(g = f, "<S extends { p: string; }[]>(S) => void");
AssertType(g, "<T extends { p: string; }>(T[]) => void");
AssertType(f, "<S extends { p: string; }[]>(S) => void");


