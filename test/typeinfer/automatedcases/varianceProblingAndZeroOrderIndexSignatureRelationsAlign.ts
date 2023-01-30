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

// === tests/cases/compiler/varianceProblingAndZeroOrderIndexSignatureRelationsAlign.ts ===
declare function AssertType(value:any, type:string):void;
type Either<L, A> = Left<L, A> | Right<L, A>;

class Left<L, A> {
    readonly _tag: 'Left' = 'Left'
    readonly _A!: A
    readonly _L!: L
    constructor(readonly value: L) {}
    /** The given function is applied if this is a `Right` */
    map<B>(f: (a: A) => B): Either<L, B> {
AssertType(this as any, "any");
AssertType(this, "this");
      return this as any
    }
    ap<B>(fab: Either<L, (a: A) => B>): Either<L, B> {
AssertType(null as any, "any");
AssertType(null, "null");
      return null as any
    }
}

class Right<L, A> {
    readonly _tag: 'Right' = 'Right'
    readonly _A!: A
    readonly _L!: L
    constructor(readonly value: A) {}
    map<B>(f: (a: A) => B): Either<L, B> {
AssertType(new Right(f(this.value)), "Right<L, B>");
AssertType(Right, "typeof Right");
AssertType(f(this.value), "B");
AssertType(f, "(A) => B");
AssertType(this.value, "A");
AssertType(this, "this");
      return new Right(f(this.value))
    }
    ap<B>(fab: Either<L, (a: A) => B>): Either<L, B> {
AssertType(null as any, "any");
AssertType(null, "null");
      return null as any;
    }
}

class Type<A, O = A, I = unknown> {
  readonly _A!: A;
  readonly _O!: O;
  readonly _I!: I;
  constructor(
    /** a unique name for this codec */
    readonly name: string,
    /** a custom type guard */
    readonly is: (u: unknown) => u is A,
    /** succeeds if a value of type I can be decoded to a value of type A */
    readonly validate: (input: I, context: {}[]) => Either<{}[], A>,
    /** converts a value of type A to a value of type O */
    readonly encode: (a: A) => O
  ) {}
  /** a version of `validate` with a default context */
  decode(i: I): Either<{}[], A> { 
AssertType(null as any, "any");
return null as any; 

AssertType(null, "null");
}
}

interface Any extends Type<any, any, any> {}

type TypeOf<C extends Any> = C["_A"];

type ToB<S extends {[_ in string | number | symbol]: Any}> = { [k in keyof S]: TypeOf<S[k]> };
type ToA<S> = { [k in keyof S]: Type<S[k]> };

type NeededInfo<MyNamespaceSchema = {}> = {
  ASchema: ToA<MyNamespaceSchema>;
};

export type MyInfo = NeededInfo<ToB<{ initialize: any }>>;

const tmp1: MyInfo = null!;
AssertType(tmp1, "MyInfo");
AssertType(null!, "never");
AssertType(null, "null");

function tmp2<N extends NeededInfo>(n: N) {}
tmp2(tmp1); // uncommenting this line removes a type error from a completely unrelated line ??
AssertType(tmp2(tmp1), "void");
AssertType(tmp2, "<N extends NeededInfo<{}>>(N) => void");
AssertType(tmp1, "MyInfo");

class Server<X extends NeededInfo> {}
export class MyServer extends Server<MyInfo> {} // not assignable error at `MyInfo`

