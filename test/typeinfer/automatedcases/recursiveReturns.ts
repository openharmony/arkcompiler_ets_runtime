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

// === tests/cases/compiler/recursiveReturns.ts ===
declare function AssertType(value:any, type:string):void;
function R1() {
	R1();
AssertType(R1(), "void");
AssertType(R1, "() => void");

	return;
}

function R2() { R2(); 
AssertType(R2(), "void");

AssertType(R2, "() => void");
}

function R3(n:number) {
	if (n == 0) {
AssertType(n == 0, "boolean");
AssertType(n, "number");
AssertType(0, "int");
	}
	else {
		R3(n--);
AssertType(R3(n--), "void");
AssertType(R3, "(number) => void");
AssertType(n--, "number");
AssertType(n, "number");
	}
}

