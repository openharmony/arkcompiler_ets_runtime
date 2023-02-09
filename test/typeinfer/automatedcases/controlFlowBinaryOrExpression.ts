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

// === tests/cases/conformance/controlFlow/controlFlowBinaryOrExpression.ts ===
declare function AssertType(value:any, type:string):void;
let x: string | number | boolean;
AssertType(x, "union");

let cond: boolean;
AssertType(cond, "boolean");

(x = "") || (x = 0);
AssertType((x = "") || (x = 0), "int");
AssertType((x = ""), "string");
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");
AssertType((x = 0), "int");
AssertType(x = 0, "int");
AssertType(x, "union");
AssertType(0, "int");

x; // string | number
AssertType(x, "union");

x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

cond || (x = 0);
AssertType(cond || (x = 0), "union");
AssertType(cond, "boolean");
AssertType((x = 0), "int");
AssertType(x = 0, "int");
AssertType(x, "union");
AssertType(0, "int");

x; // string | number
AssertType(x, "union");

export interface NodeList {
    length: number;
}

export interface HTMLCollection {
    length: number;
}

declare function isNodeList(sourceObj: any): sourceObj is NodeList;
declare function isHTMLCollection(sourceObj: any): sourceObj is HTMLCollection;

type EventTargetLike = {a: string} | HTMLCollection | NodeList;

let sourceObj: EventTargetLike = <any>undefined;
AssertType(sourceObj, "EventTargetLike");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");

if (isNodeList(sourceObj)) {
    sourceObj.length;
AssertType(sourceObj.length, "number");
}

if (isHTMLCollection(sourceObj)) {
    sourceObj.length;
AssertType(sourceObj.length, "number");
}

if (isNodeList(sourceObj) || isHTMLCollection(sourceObj)) {
    sourceObj.length;
AssertType(sourceObj.length, "number");
}


