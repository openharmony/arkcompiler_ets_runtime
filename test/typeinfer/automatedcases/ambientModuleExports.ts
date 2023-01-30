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

// === tests/cases/compiler/ambientModuleExports.ts ===
declare function AssertType(value:any, type:string):void;
declare module Foo {
	function a():void;
	let b:number;
	class C {}
}

Foo.a();
AssertType(Foo.a(), "void");
AssertType(Foo.a, "() => void");

Foo.b;
AssertType(Foo.b, "number");

let c = new Foo.C();
AssertType(c, "Foo.C");
AssertType(new Foo.C(), "Foo.C");
AssertType(Foo.C, "typeof Foo.C");

declare module Foo2 {
    export function a(): void;
    export let b: number;
    export class C { }
}

Foo2.a();
AssertType(Foo2.a(), "void");
AssertType(Foo2.a, "() => void");

Foo2.b;
AssertType(Foo2.b, "number");

let c2 = new Foo2.C();
AssertType(c2, "Foo2.C");
AssertType(new Foo2.C(), "Foo2.C");
AssertType(Foo2.C, "typeof Foo2.C");


