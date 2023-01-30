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

// === tests/cases/conformance/types/intersection/intersectionTypeOverloading.ts ===
declare function AssertType(value:any, type:string):void;
// Check that order is preserved in intersection types for purposes of
// overload resolution

type F = (s: string) => string;
type G = (x: any) => any;

let fg: F & G;
AssertType(fg, "F & G");

let gf: G & F;
AssertType(gf, "G & F");

let x = fg("abc");
AssertType(x, "string");
AssertType(fg("abc"), "string");
AssertType(fg, "F & G");
AssertType("abc", "string");

let x: string;
AssertType(x, "string");

let y = gf("abc");
AssertType(y, "any");
AssertType(gf("abc"), "any");
AssertType(gf, "G & F");
AssertType("abc", "string");

let y: any;
AssertType(y, "any");


