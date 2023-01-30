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

// === tests/cases/compiler/spreadTypeRemovesReadonly.ts ===
declare function AssertType(value:any, type:string):void;
interface ReadonlyData {
    readonly value: string;
}

const data: ReadonlyData = { value: 'foo' };
AssertType(data, "ReadonlyData");
AssertType({ value: 'foo' }, "{ value: string; }");
AssertType(value, "string");
AssertType('foo', "string");

const clone = { ...data };
AssertType(clone, "{ value: string; }");
AssertType({ ...data }, "{ value: string; }");
AssertType(data, "ReadonlyData");

clone.value = 'bar';
AssertType(clone.value = 'bar', "string");
AssertType(clone.value, "string");
AssertType('bar', "string");


