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

// === tests/cases/compiler/discriminateObjectTypesOnly.ts ===
declare function AssertType(value:any, type:string):void;
type Thing = number | object;
const k: Thing = { toFixed: null }; // OK, satisfies object
AssertType(k, "Thing");
AssertType({ toFixed: null }, "{ toFixed: null; }");
AssertType(toFixed, "null");
AssertType(null, "null");

type Thing2 = number | { toFixed: null } | object;
const q: Thing2 = { toFixed: null };
AssertType(q, "Thing2");
AssertType({ toFixed: null }, "{ toFixed: null; }");
AssertType(toFixed, "null");
AssertType(null, "null");

const h: Thing2 = { toString: null }; // OK, satisfies object
AssertType(h, "Thing2");
AssertType({ toString: null }, "{ toString: null; }");
AssertType(toString, "null");
AssertType(null, "null");

type Thing3 = number | { toFixed: null, toString: undefined } | object;
const l: Thing3 = { toString: undefined }; // error, toFixed isn't null
AssertType(l, "Thing3");
AssertType({ toString: undefined }, "{ toString: undefined; }");
AssertType(toString, "undefined");
AssertType(undefined, "undefined");


