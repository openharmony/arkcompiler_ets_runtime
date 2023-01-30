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

// === tests/cases/compiler/targetTypeObjectLiteralToAny.ts ===
declare function AssertType(value:any, type:string):void;
function suggest(){ 
	let TypeScriptKeywords:string[];  
AssertType(TypeScriptKeywords, "string[]");

	let result:any;	
AssertType(result, "any");
					
	TypeScriptKeywords.forEach(function(keyword) {						
AssertType(TypeScriptKeywords.forEach(function(keyword) {								result.push({text:keyword, type:"keyword"}); // this should not cause a crash - push should be typed to any	}), "void");
AssertType(TypeScriptKeywords.forEach, "((string, number, string[]) => void, ?any) => void");
AssertType(function(keyword) {								result.push({text:keyword, type:"keyword"}); // this should not cause a crash - push should be typed to any	}, "(string) => void");
AssertType(keyword, "string");

		result.push({text:keyword, type:"keyword"}); // this should not cause a crash - push should be typed to any
AssertType(result.push({text:keyword, type:"keyword"}), "any");
AssertType(result.push, "any");
AssertType({text:keyword, type:"keyword"}, "{ text: string; type: string; }");
AssertType(text, "string");
AssertType(keyword, "string");
AssertType(type, "string");
AssertType("keyword", "string");

	});			
}



