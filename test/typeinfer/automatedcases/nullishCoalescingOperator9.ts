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

// === tests/cases/conformance/expressions/nullishCoalescingOperator/nullishCoalescingOperator9.ts ===
declare function AssertType(value:any, type:string):void;
declare let f: null | ((x: string) => void);
AssertType(f, "union");
AssertType(null, "null");
AssertType(x, "string");

let g = f || (abc => { void abc.toLowerCase() })
AssertType(g, "(string) => void");
AssertType(f || (abc => { void abc.toLowerCase() }), "(string) => void");
AssertType(f, "union");
AssertType((abc => { void abc.toLowerCase() }), "(string) => void");
AssertType(abc => { void abc.toLowerCase() }, "(string) => void");
AssertType(abc, "string");
AssertType(void abc.toLowerCase(), "undefined");
AssertType(abc.toLowerCase(), "string");
AssertType(abc.toLowerCase, "() => string");

let gg = f ?? (abc => { void abc.toLowerCase() })
AssertType(gg, "(string) => void");
AssertType(f ?? (abc => { void abc.toLowerCase() }), "(string) => void");
AssertType(f, "union");
AssertType((abc => { void abc.toLowerCase() }), "(string) => void");
AssertType(abc => { void abc.toLowerCase() }, "(string) => void");
AssertType(abc, "string");
AssertType(void abc.toLowerCase(), "undefined");
AssertType(abc.toLowerCase(), "string");
AssertType(abc.toLowerCase, "() => string");


