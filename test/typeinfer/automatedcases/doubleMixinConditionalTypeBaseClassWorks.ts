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

// === tests/cases/compiler/doubleMixinConditionalTypeBaseClassWorks.ts ===
declare function AssertType(value:any, type:string):void;
type Constructor = new (...args: any[]) => {};

const Mixin1 = <C extends Constructor>(Base: C) => class extends Base { private _fooPrivate: {}; 
AssertType(Mixin1, "<C extends Constructor>(C) => { new (...any[]): (Anonymous class); prototype: Mixin1<any>.(Anonymous class); } & C");

AssertType(<C extends Constructor>(Base: C) => class extends Base { private _fooPrivate: {}; }, "<C extends Constructor>(C) => { new (...any[]): (Anonymous class); prototype: Mixin1<any>.(Anonymous class); } & C");

AssertType(Base, "C");

AssertType(class extends Base { private _fooPrivate: {}; }, "{ new (...any[]): (Anonymous class); prototype: Mixin1<any>.(Anonymous class); } & C");

AssertType(_fooPrivate, "{}");
}

type FooConstructor = typeof Mixin1 extends (a: Constructor) => infer Cls ? Cls : never;
const Mixin2 = <C extends FooConstructor>(Base: C) => class extends Base {};
AssertType(Mixin2, "<C extends { new (...any[]): Mixin1<Constructor>.(Anonymous class); prototype: Mixin1<any>.(Anonymous class); } & Constructor>(C) => { new (...any[]): (Anonymous class); prototype: Mixin2<any>.(Anonymous class); } & C");
AssertType(<C extends FooConstructor>(Base: C) => class extends Base {}, "<C extends { new (...any[]): Mixin1<Constructor>.(Anonymous class); prototype: Mixin1<any>.(Anonymous class); } & Constructor>(C) => { new (...any[]): (Anonymous class); prototype: Mixin2<any>.(Anonymous class); } & C");
AssertType(Base, "C");
AssertType(class extends Base {}, "{ new (...any[]): (Anonymous class); prototype: Mixin2<any>.(Anonymous class); } & C");

class C extends Mixin2(Mixin1(Object)) {}

