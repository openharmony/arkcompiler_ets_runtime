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

// === tests/cases/compiler/instanceOfAssignability.ts ===
declare function AssertType(value:any, type:string):void;
interface Base {
	foo: string|number;
	optional?: number;
}

// Derived1 is assignable to, but not a subtype of, Base
class Derived1 implements Base {
	foo: string;
}
// Derived2 is a subtype of Base that is not assignable to Derived1
class Derived2 implements Base {
	foo: number;
	optional: number;
}

class Animal {
	move;
}
class Mammal extends Animal { milk; }
class Giraffe extends Mammal { neck; }

function fn1(x: Array<number>|Array<string>|boolean) {
	if(x instanceof Array) {
AssertType(x instanceof Array, "boolean");
AssertType(x, "union");
AssertType(Array, "ArrayConstructor");

		// 1.5: y: Array<number>|Array<string>
		// Want: y: Array<number>|Array<string>
		let y = x;
AssertType(y, "union");
AssertType(x, "union");
	}
}

function fn2(x: Base) {
	if(x instanceof Derived1) {
AssertType(x instanceof Derived1, "boolean");
AssertType(x, "Base");
AssertType(Derived1, "typeof Derived1");

		// 1.5: y: Base
		// Want: y: Derived1
		let y = x;
AssertType(y, "Derived1");
AssertType(x, "Derived1");
	}
}

function fn3(x: Base|Derived1) {
	if(x instanceof Derived2) {
AssertType(x instanceof Derived2, "boolean");
AssertType(x, "union");
AssertType(Derived2, "typeof Derived2");

		// 1.5: y: Derived2
		// Want: Derived2
		let y = x;
AssertType(y, "Derived2");
AssertType(x, "Derived2");
	}
}

function fn4(x: Base|Derived2) {
	if(x instanceof Derived1) {
AssertType(x instanceof Derived1, "boolean");
AssertType(x, "union");
AssertType(Derived1, "typeof Derived1");

		// 1.5: y: {}
		// Want: Derived1
		let y = x;
AssertType(y, "Derived1");
AssertType(x, "Derived1");
	}
}

function fn5(x: Derived1) {
	if(x instanceof Derived2) {
AssertType(x instanceof Derived2, "boolean");
AssertType(x, "Derived1");
AssertType(Derived2, "typeof Derived2");

		// 1.5: y: Derived1
		// Want: ???
		let y = x;
AssertType(y, "Derived1 & Derived2");
AssertType(x, "Derived1 & Derived2");
	}
}

function fn6(x: Animal|Mammal) {
	if(x instanceof Giraffe) {
AssertType(x instanceof Giraffe, "boolean");
AssertType(x, "union");
AssertType(Giraffe, "typeof Giraffe");

		// 1.5: y: Derived1
		// Want: ???
		let y = x;
AssertType(y, "Giraffe");
AssertType(x, "Giraffe");
	}
}

function fn7(x: Array<number>|Array<string>) {
	if(x instanceof Array) {
AssertType(x instanceof Array, "boolean");
AssertType(x, "union");
AssertType(Array, "ArrayConstructor");

		// 1.5: y: Array<number>|Array<string>
		// Want: y: Array<number>|Array<string>
		let y = x;
AssertType(y, "union");
AssertType(x, "union");
	}
}

interface Alpha { a }
interface Beta { b }
interface Gamma { c }
class ABC { a; b; c; }
function fn8(x: Alpha|Beta|Gamma) {
	if(x instanceof ABC) {
AssertType(x instanceof ABC, "boolean");
AssertType(x, "union");
AssertType(ABC, "typeof ABC");

		let y = x;
AssertType(y, "ABC");
AssertType(x, "ABC");
	}
}




