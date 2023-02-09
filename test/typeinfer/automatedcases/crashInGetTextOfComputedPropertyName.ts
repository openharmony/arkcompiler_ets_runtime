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

// === tests/cases/compiler/crashInGetTextOfComputedPropertyName.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/Microsoft/TypeScript/issues/29006
export interface A { type: 'a' }
export interface B { type: 'b' }
export type AB = A | B

const itemId = 'some-id'
AssertType(itemId, "string");
AssertType('some-id', "string");

// --- test on first level ---
const items: { [id: string]: AB } = {
AssertType(items, "{ [string]: AB; }");

AssertType(id, "string");

AssertType({}, "{}");
}

const { [itemId]: itemOk1 } = items
AssertType(itemId, "string");
AssertType(itemOk1, "AB");
AssertType(items, "{ [string]: AB; }");

typeof itemOk1 // pass
AssertType(typeof itemOk1, "union");
AssertType(itemOk1, "AB");

// --- test on second level ---
interface ObjWithItems {
    items: {[s: string]: AB}
}
const objWithItems: ObjWithItems = { items: {}
AssertType(objWithItems, "ObjWithItems");

AssertType({ items: {}}, "{ items: {}; }");

AssertType(items, "{}");

AssertType({}, "{}");
}

const itemOk2 = objWithItems.items[itemId]
AssertType(itemOk2, "AB");
AssertType(objWithItems.items[itemId], "AB");
AssertType(objWithItems.items, "{ [string]: AB; }");
AssertType(itemId, "string");

typeof itemOk2 // pass
AssertType(typeof itemOk2, "union");
AssertType(itemOk2, "AB");

const {
    items: { [itemId]: itemWithTSError } = {} /*happens when default value is provided*/
AssertType(items, "any");
AssertType(itemId, "string");
AssertType(itemWithTSError, "AB");
AssertType({}, "{}");

} = objWithItems
AssertType(objWithItems, "ObjWithItems");

// in order to re-produce the error, uncomment next line:
typeof itemWithTSError // :(
AssertType(typeof itemWithTSError, "union");
AssertType(itemWithTSError, "AB");

// will result in:
// Error from compilation: TypeError: Cannot read property 'charCodeAt' of undefined  TypeError: Cannot read property 'charCodeAt' of undefined

