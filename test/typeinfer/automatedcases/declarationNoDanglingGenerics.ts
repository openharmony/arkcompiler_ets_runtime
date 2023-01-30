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

// === tests/cases/compiler/declarationNoDanglingGenerics.ts ===
declare function AssertType(value:any, type:string):void;
const kindCache: { [kind: string]: boolean } = {};
AssertType(kindCache, "{ [string]: boolean; }");
AssertType(kind, "string");
AssertType({}, "{}");

function register(kind: string): void | never {
  if (kindCache[kind]) {
AssertType(kindCache[kind], "boolean");
AssertType(kindCache, "{ [string]: boolean; }");
AssertType(kind, "string");

    throw new Error(`Class with kind "${kind}" is already registered.`);
AssertType(new Error(`Class with kind "${kind}" is already registered.`), "Error");
AssertType(Error, "ErrorConstructor");
AssertType(`Class with kind "${kind}" is already registered.`, "string");
AssertType(kind, "string");
  }
  kindCache[kind] = true;
AssertType(kindCache[kind] = true, "boolean");
AssertType(kindCache[kind], "boolean");
AssertType(kindCache, "{ [string]: boolean; }");
AssertType(kind, "string");
AssertType(true, "boolean");
}

function ClassFactory<TKind extends string>(kind: TKind) {
  register(kind);
AssertType(register(kind), "void");
AssertType(register, "(string) => void");
AssertType(kind, "TKind");

AssertType(class {    static readonly THE_KIND: TKind = kind;    readonly kind: TKind = kind;  }, "typeof (Anonymous class)");
  return class {

    static readonly THE_KIND: TKind = kind;
AssertType(THE_KIND, "TKind");
AssertType(kind, "TKind");

    readonly kind: TKind = kind;
AssertType(kind, "TKind");
AssertType(kind, "TKind");

  };
}

class Kinds {
  static readonly A = "A";
  static readonly B = "B";
  static readonly C = "C";
}

export class AKind extends ClassFactory(Kinds.A) {
}

export class BKind extends ClassFactory(Kinds.B) {
}

export class CKind extends ClassFactory(Kinds.C) {
}

