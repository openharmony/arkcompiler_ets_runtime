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

// === tests/cases/conformance/jsdoc/declarations/foo.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @typedef {{
 *   [id: string]: [Function, Function];
 * }} ResolveRejectMap
 */

let id = 0
AssertType(id, "number");
AssertType(0, "int");

/**
 * @param {ResolveRejectMap} handlers
 * @
returns {Promise<any>}
 */
const send = handlers => new Promise((resolve, reject) => {
AssertType(send, "(ResolveRejectMap) => Promise<any>");
AssertType(handlers => new Promise((resolve, reject) => {  handlers[++id] = [resolve, reject]}), "(ResolveRejectMap) => Promise<any>");
AssertType(handlers, "ResolveRejectMap");
AssertType(new Promise((resolve, reject) => {  handlers[++id] = [resolve, reject]}), "Promise<any>");
AssertType(Promise, "PromiseConstructor");
AssertType((resolve, reject) => {  handlers[++id] = [resolve, reject]}, "((any) => void, (?any) => void) => void");
AssertType(resolve, "(any) => void");
AssertType(reject, "(?any) => void");

  handlers[++id] = [resolve, reject]
AssertType(handlers[++id] = [resolve, reject], "[(any) => void, (?any) => void]");
AssertType(handlers[++id], "[Function, Function]");
AssertType(handlers, "ResolveRejectMap");
AssertType(++id, "number");
AssertType(id, "number");
AssertType([resolve, reject], "[(any) => void, (?any) => void]");
AssertType(resolve, "(any) => void");
AssertType(reject, "(?any) => void");

})

