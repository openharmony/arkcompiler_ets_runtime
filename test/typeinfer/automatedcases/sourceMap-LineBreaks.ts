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

// === tests/cases/compiler/sourceMap-LineBreaks.ts ===
declare function AssertType(value:any, type:string):void;
let endsWithlineSeparator = 10; 
AssertType(endsWithlineSeparator, "number");
AssertType(10, "int");

let endsWithParagraphSeparator = 10; 
AssertType(endsWithParagraphSeparator, "number");
AssertType(10, "int");

let endsWithNextLine = 1;let endsWithLineFeed = 1;
AssertType(endsWithNextLine, "number");
AssertType(1, "int");
AssertType(endsWithLineFeed, "number");
AssertType(1, "int");

let endsWithCarriageReturnLineFeed = 1;
AssertType(endsWithCarriageReturnLineFeed, "number");
AssertType(1, "int");

let endsWithCarriageReturn = 1;
AssertType(endsWithCarriageReturn, "number");
AssertType(1, "int");

let endsWithLineFeedCarriageReturn = 1;
AssertType(endsWithLineFeedCarriageReturn, "number");
AssertType(1, "int");

let endsWithLineFeedCarriageReturnLineFeed = 1;
AssertType(endsWithLineFeedCarriageReturnLineFeed, "number");
AssertType(1, "int");

let stringLiteralWithLineFeed = "line 1\
AssertType(stringLiteralWithLineFeed, "string");
AssertType("line 1\line 2", "string");

line 2";
let stringLiteralWithCarriageReturnLineFeed = "line 1\
AssertType(stringLiteralWithCarriageReturnLineFeed, "string");
AssertType("line 1\line 2", "string");

line 2";
let stringLiteralWithCarriageReturn = "line 1\
AssertType(stringLiteralWithCarriageReturn, "string");
AssertType("line 1\
line 2", "string");

line 2";

let stringLiteralWithLineSeparator = "line 1\
AssertType(stringLiteralWithLineSeparator, "string");
AssertType("line 1\ line 2", "string");

line 2";
let stringLiteralWithParagraphSeparator = "line 1\
AssertType(stringLiteralWithParagraphSeparator, "string");
AssertType("line 1\ line 2", "string");

line 2";
let stringLiteralWithNextLine = "line 1\line 2";
AssertType(stringLiteralWithNextLine, "string");
AssertType("line 1\line 2", "string");


