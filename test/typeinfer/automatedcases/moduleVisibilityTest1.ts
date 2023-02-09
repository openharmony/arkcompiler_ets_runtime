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

// === tests/cases/compiler/moduleVisibilityTest1.ts ===
declare function AssertType(value:any, type:string):void;
module OuterMod {
	export function someExportedOuterFunc() { 
AssertType(-1, "int");
return -1; 

AssertType(1, "int");
}

	export module OuterInnerMod {
		export function someExportedOuterInnerFunc() { 
AssertType("foo", "string");
return "foo"; 
}
	}
}

import OuterInnerAlias = OuterMod.OuterInnerMod;

module M {

	export module InnerMod {
		export function someExportedInnerFunc() { 
AssertType(-2, "int");
return -2; 

AssertType(2, "int");
}
	}

	export enum E {
		A,
		B,
		C,
	}

	export let x = 5;
	export declare let exported_let;

	let y = x + x;


	export interface I {
		someMethod():number;
	}

	 class B {public b = 0;}

	 export class C implements I {
		public someMethodThatCallsAnOuterMethod() {
AssertType(OuterInnerAlias.someExportedOuterInnerFunc(), "string");
return OuterInnerAlias.someExportedOuterInnerFunc();

AssertType(OuterInnerAlias.someExportedOuterInnerFunc, "() => string");
}

		public someMethodThatCallsAnInnerMethod() {
AssertType(InnerMod.someExportedInnerFunc(), "number");
return InnerMod.someExportedInnerFunc();

AssertType(InnerMod.someExportedInnerFunc, "() => number");
}

		public someMethodThatCallsAnOuterInnerMethod() {
AssertType(OuterMod.someExportedOuterFunc(), "number");
return OuterMod.someExportedOuterFunc();

AssertType(OuterMod.someExportedOuterFunc, "() => number");
}

		public someMethod() { 
AssertType(0, "int");
return 0; 
}

		public someProp = 1;

		constructor() {
		    function someInnerFunc() { 
AssertType(someInnerFunc, "() => number");
return 2; 

AssertType(2, "int");
}

            let someInnerVar = 3;
AssertType(someInnerVar, "number");
AssertType(3, "int");
		}
	}

	let someModuleVar = 4;

	function someModuleFunction() { 
AssertType(5, "int");
return 5;
}
}

module M {
	export let c = x;
	export let meb = M.E.B;
}

let cprime : M.I = <M.I>null;
AssertType(cprime, "M.I");
AssertType(M, "any");
AssertType(<M.I>null, "M.I");
AssertType(M, "any");
AssertType(null, "null");

let c = new M.C();
AssertType(c, "M.C");
AssertType(new M.C(), "M.C");
AssertType(M.C, "typeof M.C");

let z = M.x;
AssertType(z, "number");
AssertType(M.x, "number");

let alpha = M.E.A;
AssertType(alpha, "M.E");
AssertType(M.E.A, "M.E.A");

let omega = M.exported_let;
AssertType(omega, "any");
AssertType(M.exported_let, "any");

c.someMethodThatCallsAnOuterMethod();
AssertType(c.someMethodThatCallsAnOuterMethod(), "string");
AssertType(c.someMethodThatCallsAnOuterMethod, "() => string");


