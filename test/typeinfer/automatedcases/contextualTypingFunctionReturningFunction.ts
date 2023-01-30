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

// === tests/cases/compiler/contextualTypingFunctionReturningFunction.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
	a(s: string): void;
	b(): (n: number) => void;
}

declare function f(i: I): void;

f({
AssertType(f({	a: s => {},	b: () => n => {},}), "void");
AssertType(f, "(I) => void");
AssertType({	a: s => {},	b: () => n => {},}, "{ a: (string) => void; b: () => (number) => void; }");

	a: s => {},
AssertType(a, "(string) => void");
AssertType(s => {}, "(string) => void");
AssertType(s, "string");

	b: () => n => {},
AssertType(b, "() => (number) => void");
AssertType(() => n => {}, "() => (number) => void");
AssertType(n => {}, "(number) => void");
AssertType(n, "number");

});


