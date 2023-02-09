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

// === tests/cases/conformance/jsdoc/declarations/source.js ===
declare function AssertType(value:any, type:string):void;
class Handler {
	static get OPTIONS() {
AssertType(1, "int");
		return 1;
	}

	process() {
	}
}
Handler.statische = function() { 
AssertType(Handler.statische = function() { }, "() => void");

AssertType(Handler.statische, "() => void");

AssertType(function() { }, "() => void");
}

const Strings = {
AssertType(Strings, "{ a: string; b: string; }");
AssertType({    a: "A",    b: "B"}, "{ a: string; b: string; }");

    a: "A",
AssertType(a, "string");
AssertType("A", "string");

    b: "B"
AssertType(b, "string");
AssertType("B", "string");
}

module.exports = Handler;
AssertType(module.exports = Handler, "typeof Handler");
AssertType(module.exports, "typeof Handler");
AssertType(Handler, "typeof Handler");

module.exports.Strings = Strings
AssertType(module.exports.Strings = Strings, "{ a: string; b: string; }");
AssertType(module.exports.Strings, "{ a: string; b: string; }");
AssertType(Strings, "{ a: string; b: string; }");

/**
 * @typedef {Object} HandlerOptions
 * @property {String} name
 * Should be able to export a type alias at the same time.
 */


