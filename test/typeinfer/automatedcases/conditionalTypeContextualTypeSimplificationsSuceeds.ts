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

// === tests/cases/compiler/conditionalTypeContextualTypeSimplificationsSuceeds.ts ===
declare function AssertType(value:any, type:string):void;
// repro from https://github.com/Microsoft/TypeScript/issues/26395
interface Props {
    when: (value: string) => boolean;
}

function bad<P extends Props>(
    attrs: string extends keyof P ? { [K in keyof P]: P[K] } : { [K in keyof P]: P[K] }) { }
function good1<P extends Props>(
    attrs: string extends keyof P ? P : { [K in keyof P]: P[K] }) { }
function good2<P extends Props>(
    attrs: { [K in keyof P]: P[K] }) { }

bad({ when: value => false });
AssertType(bad({ when: value => false }), "void");
AssertType(bad, "<P extends Props>(string extends keyof P ? { [K in keyof P]: P[K]; } : { [K in keyof P]: P[K]; }) => void");
AssertType({ when: value => false }, "{ when: (string) => false; }");
AssertType(when, "(string) => false");
AssertType(value => false, "(string) => false");
AssertType(value, "string");
AssertType(false, "boolean");

good1({ when: value => false });
AssertType(good1({ when: value => false }), "void");
AssertType(good1, "<P extends Props>(string extends keyof P ? P : { [K in keyof P]: P[K]; }) => void");
AssertType({ when: value => false }, "{ when: (string) => false; }");
AssertType(when, "(string) => false");
AssertType(value => false, "(string) => false");
AssertType(value, "string");
AssertType(false, "boolean");

good2({ when: value => false });
AssertType(good2({ when: value => false }), "void");
AssertType(good2, "<P extends Props>({ [K in keyof P]: P[K]; }) => void");
AssertType({ when: value => false }, "{ when: (string) => false; }");
AssertType(when, "(string) => false");
AssertType(value => false, "(string) => false");
AssertType(value, "string");
AssertType(false, "boolean");


