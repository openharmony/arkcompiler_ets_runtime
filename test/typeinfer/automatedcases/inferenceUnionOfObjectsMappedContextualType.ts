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

// === tests/cases/compiler/inferenceUnionOfObjectsMappedContextualType.ts ===
declare function AssertType(value:any, type:string):void;
type Entity = {
    someDate: Date | null;
} & ({ id: string; } | { id: number; })

type RowRendererMeta<TInput extends {}> = {
    [key in keyof TInput]: { key: key; caption: string; formatter?: (value: TInput[key]) => string; };
}

type RowRenderer<TInput extends {}> = RowRendererMeta<TInput>[keyof RowRendererMeta<TInput>];

const test: RowRenderer<Entity> = {
AssertType(test, "RowRenderer<Entity>");
AssertType({    key: 'someDate',    caption: 'My Date',    formatter: (value) => value ? value.toString() : '-' // value: any}, "{ key: "someDate"; caption: string; formatter: (union) => string; }");

    key: 'someDate',
AssertType(key, "string");
AssertType('someDate', "string");

    caption: 'My Date',
AssertType(caption, "string");
AssertType('My Date', "string");

    formatter: (value) => value ? value.toString() : '-' // value: any
AssertType(formatter, "(union) => string");
AssertType((value) => value ? value.toString() : '-', "(union) => string");
AssertType(value, "union");
AssertType(value ? value.toString() : '-', "string");
AssertType(value, "union");
AssertType(value.toString(), "string");
AssertType(value.toString, "() => string");
AssertType('-', "string");
}


