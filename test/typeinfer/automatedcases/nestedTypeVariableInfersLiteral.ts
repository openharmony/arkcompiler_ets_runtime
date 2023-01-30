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

// === tests/cases/compiler/nestedTypeVariableInfersLiteral.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/Microsoft/TypeScript/issues/19632
declare function direct<A extends string>(a: A | A[]): Record<A, string>
declare function nested<A extends string>(a: { fields: A }): Record<A, string>
declare function nestedUnion<A extends string>(a: { fields: A | A[] }): Record<A, string>

const directUnionSingle = direct("z")
AssertType(directUnionSingle, "Record<"z", string>");
AssertType(direct("z"), "Record<"z", string>");
AssertType(direct, "<A extends string>(union) => Record<A, string>");
AssertType("z", "string");

const directUnionArray = direct(["z", "y"])
AssertType(directUnionArray, "Record<union, string>");
AssertType(direct(["z", "y"]), "Record<union, string>");
AssertType(direct, "<A extends string>(union) => Record<A, string>");
AssertType(["z", "y"], "(union)[]");
AssertType("z", "string");
AssertType("y", "string");

const nestedSingle = nested({fields: "z"})
AssertType(nestedSingle, "Record<"z", string>");
AssertType(nested({fields: "z"}), "Record<"z", string>");
AssertType(nested, "<A extends string>({ fields: A; }) => Record<A, string>");
AssertType({fields: "z"}, "{ fields: "z"; }");
AssertType(fields, "string");
AssertType("z", "string");

const nestedUnionSingle = nestedUnion({fields: "z"})
AssertType(nestedUnionSingle, "Record<"z", string>");
AssertType(nestedUnion({fields: "z"}), "Record<"z", string>");
AssertType(nestedUnion, "<A extends string>({ fields: union; }) => Record<A, string>");
AssertType({fields: "z"}, "{ fields: "z"; }");
AssertType(fields, "string");
AssertType("z", "string");

const nestedUnionArray = nestedUnion({fields: ["z", "y"]})
AssertType(nestedUnionArray, "Record<union, string>");
AssertType(nestedUnion({fields: ["z", "y"]}), "Record<union, string>");
AssertType(nestedUnion, "<A extends string>({ fields: union; }) => Record<A, string>");
AssertType({fields: ["z", "y"]}, "{ fields: (union)[]; }");
AssertType(fields, "(union)[]");
AssertType(["z", "y"], "(union)[]");
AssertType("z", "string");
AssertType("y", "string");

declare function hasZField(arg: { z: string }): void

hasZField(directUnionSingle) // ok
AssertType(hasZField(directUnionSingle), "void");
AssertType(hasZField, "({ z: string; }) => void");
AssertType(directUnionSingle, "Record<"z", string>");

hasZField(directUnionArray) // ok
AssertType(hasZField(directUnionArray), "void");
AssertType(hasZField, "({ z: string; }) => void");
AssertType(directUnionArray, "Record<union, string>");

hasZField(nestedSingle) // ok
AssertType(hasZField(nestedSingle), "void");
AssertType(hasZField, "({ z: string; }) => void");
AssertType(nestedSingle, "Record<"z", string>");

hasZField(nestedUnionSingle) // ok
AssertType(hasZField(nestedUnionSingle), "void");
AssertType(hasZField, "({ z: string; }) => void");
AssertType(nestedUnionSingle, "Record<"z", string>");

hasZField(nestedUnionArray) // ok
AssertType(hasZField(nestedUnionArray), "void");
AssertType(hasZField, "({ z: string; }) => void");
AssertType(nestedUnionArray, "Record<union, string>");


