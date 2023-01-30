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

// === tests/cases/compiler/externModuleClobber.ts ===
declare function AssertType(value:any, type:string):void;
declare module EM {
	export class Position { }

	export class EC {
		public getPosition() : EM.Position;
	}
}

let x:EM.Position;
AssertType(x, "EM.Position");
AssertType(EM, "any");

let ec:EM.EC = new EM.EC();
AssertType(ec, "EM.EC");
AssertType(EM, "any");
AssertType(new EM.EC(), "EM.EC");
AssertType(EM.EC, "typeof EM.EC");

x = ec.getPosition();
AssertType(x = ec.getPosition(), "EM.Position");
AssertType(x, "EM.Position");
AssertType(ec.getPosition(), "EM.Position");
AssertType(ec.getPosition, "() => EM.Position");


