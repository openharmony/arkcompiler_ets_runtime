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

// === tests/cases/compiler/switchFallThroughs.ts ===
declare function AssertType(value:any, type:string):void;
function R1(index: number) {
    switch (index) {
AssertType(index, "number");

        case 0:
AssertType(0, "int");

        case 1:
AssertType(1, "int");

        case 2:
AssertType(2, "int");

            let a = 'a';
AssertType(a, "string");
AssertType('a', "string");

AssertType(a, "string");
            return a;

        case 3:
AssertType(3, "int");

        case 4: {
AssertType(4, "int");

AssertType('b', "string");
            return 'b';
        }
		case 5:
AssertType(5, "int");

		default:
AssertType('c', "string");
			return 'c';
    }
}


