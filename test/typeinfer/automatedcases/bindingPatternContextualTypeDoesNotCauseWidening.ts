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

// === tests/cases/compiler/bindingPatternContextualTypeDoesNotCauseWidening.ts ===
declare function AssertType(value:any, type:string):void;
declare function pick<O, T extends keyof O>(keys: T[], obj?: O): Pick<O, T>;
const _    = pick(['b'], { a: 'a', b: 'b' }); // T: "b"
AssertType(_, "Pick<{ a: string; b: string; }, "b">");
AssertType(pick(['b'], { a: 'a', b: 'b' }), "Pick<{ a: string; b: string; }, "b">");
AssertType(pick, "<O, T extends keyof O>(T[], ?O) => Pick<O, T>");
AssertType(['b'], ""b"[]");
AssertType('b', "string");
AssertType({ a: 'a', b: 'b' }, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType('a', "string");
AssertType(b, "string");
AssertType('b', "string");

const {  } = pick(['b'], { a: 'a', b: 'b' }); // T: "b" | "a" ??? (before fix)
AssertType(pick(['b'], { a: 'a', b: 'b' }), "Pick<{ a: string; b: string; }, "b">");
AssertType(pick, "<O, T extends keyof O>(T[], ?O) => Pick<O, T>");
AssertType(['b'], ""b"[]");
AssertType('b', "string");
AssertType({ a: 'a', b: 'b' }, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType('a', "string");
AssertType(b, "string");
AssertType('b', "string");


