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

// === tests/cases/compiler/enumKeysQuotedAsObjectPropertiesInDeclarationEmit.ts ===
declare function AssertType(value:any, type:string):void;
export enum MouseButton {
	LEFT_BUTTON = 1,
	RIGHT_BUTTON = 2,
	MIDDLE_BUTTON = 4,
	XBUTTON1_BUTTON = 5,
	XBUTTON2_BUTTON = 6,
	NO_BUTTON = 0,
}

export const DOMMouseButton = {
AssertType(DOMMouseButton, "{ '-1': MouseButton; "0": MouseButton; "1": MouseButton; "2": MouseButton; "3": MouseButton; "4": MouseButton; }");
AssertType({	'-1': MouseButton.NO_BUTTON,	"0": MouseButton.LEFT_BUTTON,	"1": MouseButton.MIDDLE_BUTTON,	"2": MouseButton.RIGHT_BUTTON,	"3": MouseButton.XBUTTON1_BUTTON,	"4": MouseButton.XBUTTON2_BUTTON,}, "{ '-1': MouseButton; "0": MouseButton; "1": MouseButton; "2": MouseButton; "3": MouseButton; "4": MouseButton; }");

	'-1': MouseButton.NO_BUTTON,
AssertType('-1', "MouseButton");
AssertType(MouseButton.NO_BUTTON, "MouseButton.NO_BUTTON");

	"0": MouseButton.LEFT_BUTTON,
AssertType("0", "MouseButton");
AssertType(MouseButton.LEFT_BUTTON, "MouseButton.LEFT_BUTTON");

	"1": MouseButton.MIDDLE_BUTTON,
AssertType("1", "MouseButton");
AssertType(MouseButton.MIDDLE_BUTTON, "MouseButton.MIDDLE_BUTTON");

	"2": MouseButton.RIGHT_BUTTON,
AssertType("2", "MouseButton");
AssertType(MouseButton.RIGHT_BUTTON, "MouseButton.RIGHT_BUTTON");

	"3": MouseButton.XBUTTON1_BUTTON,
AssertType("3", "MouseButton");
AssertType(MouseButton.XBUTTON1_BUTTON, "MouseButton.XBUTTON1_BUTTON");

	"4": MouseButton.XBUTTON2_BUTTON,
AssertType("4", "MouseButton");
AssertType(MouseButton.XBUTTON2_BUTTON, "MouseButton.XBUTTON2_BUTTON");

};

