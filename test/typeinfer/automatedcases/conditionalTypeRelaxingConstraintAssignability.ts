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

// === tests/cases/compiler/conditionalTypeRelaxingConstraintAssignability.ts ===
declare function AssertType(value:any, type:string):void;
export type ElChildren =
  | ElChildren.Void
  | ElChildren.Text;
export namespace ElChildren {
  export type Void = undefined;
  export type Text = string;
}

type Relax<C extends ElChildren> = C extends ElChildren.Text ? ElChildren.Text : C;

export class Elem<
  C extends ElChildren,
  > {
  constructor(
    private children_: Relax<C>,
  ) {
  }
}

new Elem(undefined as ElChildren.Void);
AssertType(new Elem(undefined as ElChildren.Void), "Elem<undefined>");
AssertType(Elem, "typeof Elem");
AssertType(undefined as ElChildren.Void, "undefined");
AssertType(undefined, "undefined");
AssertType(ElChildren, "any");

new Elem('' as ElChildren.Text);
AssertType(new Elem('' as ElChildren.Text), "Elem<string>");
AssertType(Elem, "typeof Elem");
AssertType('' as ElChildren.Text, "string");
AssertType('', "string");
AssertType(ElChildren, "any");

new Elem('' as ElChildren.Void | ElChildren.Text); // error
AssertType(new Elem('' as ElChildren.Void | ElChildren.Text), "Elem<union>");
AssertType(Elem, "typeof Elem");
AssertType('' as ElChildren.Void | ElChildren.Text, "union");
AssertType('', "string");
AssertType(ElChildren, "any");
AssertType(ElChildren, "any");

new Elem('' as ElChildren); // error
AssertType(new Elem('' as ElChildren), "Elem<ElChildren>");
AssertType(Elem, "typeof Elem");
AssertType('' as ElChildren, "ElChildren");
AssertType('', "string");

// Repro from #31766

interface I { a: string }

type DeepPartial<T> =
    T extends object ? {[K in keyof T]?: DeepPartial<T[K]>} : T;

declare function f<T>(t: T, partial: DeepPartial<T>): T;

function g(p1: I, p2: Partial<I>): I {
AssertType(f(p1, p2), "I");
AssertType(f, "<T>(T, DeepPartial<T>) => T");
AssertType(p1, "I");
AssertType(p2, "Partial<I>");
  return f(p1, p2);
}


