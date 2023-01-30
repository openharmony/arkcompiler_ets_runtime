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

// === tests/cases/compiler/reverseMappedTypeAssignableToIndex.ts ===
declare function AssertType(value:any, type:string):void;

// Simple mapped type and inferrence
type Mapped<T> = { [K in keyof T]: { name: T[K] } };
type InferFromMapped<T> = T extends Mapped<infer R> ? R : never;

// Object literal type and associated mapped type
// Note that in the real code we don't have a direct reference to LiteralType
type LiteralType = {
	first: "first";
	second: "second";
}
type MappedLiteralType = {
	first: { name: "first" },
	second: { name: "second" },
};

type Inferred = InferFromMapped<MappedLiteralType>;

// UNEXPECTED resolves to false
type Test1 = Inferred extends Record<any, string> ? true : false;

