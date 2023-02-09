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

// === tests/cases/compiler/declFileRegressionTests.ts ===
declare function AssertType(value:any, type:string):void;
// 'null' not converted to 'any' in d.ts
// function types not piped through correctly
let n = { w: null, x: '', y: () => { }, z: 32 };
AssertType(n, "{ w: any; x: string; y: () => void; z: number; }");
AssertType({ w: null, x: '', y: () => { }, z: 32 }, "{ w: null; x: string; y: () => void; z: number; }");
AssertType(w, "null");
AssertType(null, "null");
AssertType(x, "string");
AssertType('', "string");
AssertType(y, "() => void");
AssertType(() => { }, "() => void");
AssertType(z, "number");
AssertType(32, "int");



