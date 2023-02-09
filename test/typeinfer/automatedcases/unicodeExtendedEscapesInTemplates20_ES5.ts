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

// === tests/cases/conformance/es6/unicodeExtendedEscapes/unicodeExtendedEscapesInTemplates20_ES5.ts ===
declare function AssertType(value:any, type:string):void;
let x = `\u{48}\u{65}\u{6c}\u{6c}\u{6f}${`\u{20}\u{020}\u{0020}\u{000020}`}\u{77}\u{6f}\u{72}\u{6c}\u{64}`;
AssertType(x, "string");
AssertType(`\u{48}\u{65}\u{6c}\u{6c}\u{6f}${`\u{20}\u{020}\u{0020}\u{000020}`}\u{77}\u{6f}\u{72}\u{6c}\u{64}`, "string");
AssertType(`\u{20}\u{020}\u{0020}\u{000020}`, "string");


