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

// === tests/cases/compiler/requireEmitSemicolon_1.ts ===
declare function AssertType(value:any, type:string):void;
///<reference path='requireEmitSemicolon_0.ts'/>
import * as P from "requireEmitSemicolon_0"; // bug was we were not emitting a ; here and causing runtime failures in node

export module Database {
	export class DB {
	    public findPerson(id: number): P.Models.Person {
AssertType(new P.Models.Person("Rock"), "P.Models.Person");
AssertType(P.Models.Person, "typeof P.Models.Person");
AssertType(P.Models, "typeof P.Models");
AssertType("Rock", "string");
	        return new P.Models.Person("Rock");
	    }
	}
} 

