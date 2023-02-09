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

// === tests/cases/compiler/uncaughtCompilerError1.ts ===
declare function AssertType(value:any, type:string):void;
declare let index, lineTokens, token, tokens;
AssertType(index, "any");
AssertType(lineTokens, "any");
AssertType(token, "any");
AssertType(tokens, "any");

function f() {
    if (lineTokens[index].trim() === '=' && index > 0 && token.type === '' && tokens[index - 1].type === 'attribute.name.html') {
AssertType(lineTokens[index].trim() === '=' && index > 0 && token.type === '' && tokens[index - 1].type === 'attribute.name.html', "boolean");
AssertType(lineTokens[index].trim() === '=' && index > 0 && token.type === '', "boolean");
AssertType(lineTokens[index].trim() === '=' && index > 0, "boolean");
AssertType(lineTokens[index].trim() === '=', "boolean");
AssertType(lineTokens[index].trim(), "any");
AssertType(lineTokens[index].trim, "any");
AssertType(lineTokens[index], "any");
AssertType(lineTokens, "any");
AssertType(index, "any");
AssertType('=', "string");
AssertType(index > 0, "boolean");
AssertType(index, "any");
AssertType(0, "int");
AssertType(token.type === '', "boolean");
AssertType(token.type, "any");
AssertType('', "string");
AssertType(tokens[index - 1].type === 'attribute.name.html', "boolean");
AssertType(tokens[index - 1].type, "any");
AssertType(tokens[index - 1], "any");
AssertType(tokens, "any");
AssertType(index - 1, "number");
AssertType(index, "any");
AssertType(1, "int");
AssertType('attribute.name.html', "string");

        if (index === (tokens.length - 1)) {
AssertType(index === (tokens.length - 1), "boolean");
AssertType(index, "any");
AssertType((tokens.length - 1), "number");
AssertType(tokens.length - 1, "number");
AssertType(tokens.length, "any");
AssertType(1, "int");

AssertType({ appendText: '\"\"', advanceCount: 1 }, "{ appendText: string; advanceCount: number; }");
AssertType(appendText, "string");
AssertType('\"\"', "string");
AssertType(advanceCount, "number");
AssertType(1, "int");
            return { appendText: '\"\"', advanceCount: 1 };
        }
        else if (tokens[index + 1].type !== 'attribute.value.html' && tokens[index + 1].type !== '') {
AssertType(tokens[index + 1].type !== 'attribute.value.html' && tokens[index + 1].type !== '', "boolean");
AssertType(tokens[index + 1].type !== 'attribute.value.html', "boolean");
AssertType(tokens[index + 1].type, "any");
AssertType(tokens[index + 1], "any");
AssertType(tokens, "any");
AssertType(index + 1, "any");
AssertType(index, "any");
AssertType(1, "int");
AssertType('attribute.value.html', "string");
AssertType(tokens[index + 1].type !== '', "boolean");
AssertType(tokens[index + 1].type, "any");
AssertType(tokens[index + 1], "any");
AssertType(tokens, "any");
AssertType(index + 1, "any");
AssertType(index, "any");
AssertType(1, "int");
AssertType('', "string");

AssertType({ appendText: '\"\"', advanceCount: 1 }, "{ appendText: string; advanceCount: number; }");
AssertType(appendText, "string");
AssertType('\"\"', "string");
AssertType(advanceCount, "number");
AssertType(1, "int");
            return { appendText: '\"\"', advanceCount: 1 };
        }
AssertType(null, "null");
        return null;
    }
}


