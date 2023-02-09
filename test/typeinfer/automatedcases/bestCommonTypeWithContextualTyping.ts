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

// === tests/cases/compiler/bestCommonTypeWithContextualTyping.ts ===
declare function AssertType(value:any, type:string):void;
interface Contextual {
    dummy;
    p?: number;
}

interface Ellement {
    dummy;
    p: any;
}

let e: Ellement;
AssertType(e, "Ellement");

// All of these should pass. Neither type is a supertype of the other, but the RHS should
// always use Ellement in these examples (not Contextual). Because Ellement is assignable
// to Contextual, no errors.
let arr: Contextual[] = [e]; // Ellement[]
AssertType(arr, "Contextual[]");
AssertType([e], "Ellement[]");
AssertType(e, "Ellement");

let obj: { [s: string]: Contextual } = { s: e }; // { s: Ellement; [s: string]: Ellement 
AssertType(obj, "{ [string]: Contextual; }");

AssertType(s, "string");

AssertType({ s: e }, "{ s: Ellement; }");

AssertType(s, "Ellement");

AssertType(e, "Ellement");
}

let conditional: Contextual = null ? e : e; // Ellement
AssertType(conditional, "Contextual");
AssertType(null ? e : e, "Ellement");
AssertType(null, "null");
AssertType(e, "Ellement");
AssertType(e, "Ellement");

let contextualOr: Contextual = e || e; // Ellement
AssertType(contextualOr, "Contextual");
AssertType(e || e, "Ellement");
AssertType(e, "Ellement");
AssertType(e, "Ellement");


