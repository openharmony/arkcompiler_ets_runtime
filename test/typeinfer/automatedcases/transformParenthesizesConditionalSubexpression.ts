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

// === tests/cases/compiler/transformParenthesizesConditionalSubexpression.ts ===
declare function AssertType(value:any, type:string):void;
let K = 'k'
AssertType(K, "string");
AssertType('k', "string");

let a = { p  : (true ? { [K] : 'v'}        : null) 
AssertType(a, "{ p: { [string]: string; }; }");

AssertType({ p  : (true ? { [K] : 'v'}        : null) }, "{ p: { [string]: string; }; }");

AssertType(p, "{ [string]: string; }");

AssertType((true ? { [K] : 'v'}        : null), "{ [string]: string; }");

AssertType(true ? { [K] : 'v'}        : null, "{ [string]: string; }");

AssertType(true, "boolean");

AssertType({ [K] : 'v'}, "{ [string]: string; }");

AssertType([K], "string");

AssertType(K, "string");

AssertType('v', "string");

AssertType(null, "null");
}

let b = { p  : (true ? { [K] : 'v'} as any : null) 
AssertType(b, "{ p: any; }");

AssertType({ p  : (true ? { [K] : 'v'} as any : null) }, "{ p: any; }");

AssertType(p, "any");

AssertType((true ? { [K] : 'v'} as any : null), "any");

AssertType(true ? { [K] : 'v'} as any : null, "any");

AssertType(true, "boolean");

AssertType({ [K] : 'v'} as any, "any");

AssertType({ [K] : 'v'}, "{ [string]: string; }");

AssertType([K], "string");

AssertType(K, "string");

AssertType('v', "string");

AssertType(null, "null");
}


