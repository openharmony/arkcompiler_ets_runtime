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

// === tests/cases/compiler/recursiveMods.ts ===
declare function AssertType(value:any, type:string):void;
export module Foo {
	export class C {}
}

export module Foo {

	function Bar() : C {
		if (true) { 
AssertType(true, "boolean");
return Bar();

AssertType(Bar(), "C");

AssertType(Bar, "() => C");
}

AssertType(new C(), "C");
AssertType(C, "typeof C");
		return new C();
	}

	function Baz() : C {
		let c = Baz();
AssertType(c, "C");
AssertType(Baz(), "C");
AssertType(Baz, "() => C");

AssertType(Bar(), "C");
AssertType(Bar, "() => C");
		return Bar();
	}

	function Gar() {
		let c : C = Baz();
AssertType(c, "C");
AssertType(Baz(), "C");
AssertType(Baz, "() => C");

		return;
	}
	
}


