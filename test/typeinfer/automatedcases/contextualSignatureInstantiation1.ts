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

// === tests/cases/compiler/contextualSignatureInstantiation1.ts ===
declare function AssertType(value:any, type:string):void;
declare function map<S, T>(f: (x: S) => T): (a: S[]) => T[];
let e = <K>(x: string, y?: K) => x.length;
AssertType(e, "<K>(string, ?K) => number");
AssertType(<K>(x: string, y?: K) => x.length, "<K>(string, ?K) => number");
AssertType(x, "string");
AssertType(y, "K");
AssertType(x.length, "number");

let r99 = map(e); // should be {}[] for S since a generic lambda is not inferentially typed
AssertType(r99, "<K>(string[]) => number[]");
AssertType(map(e), "<K>(string[]) => number[]");
AssertType(map, "<S, T>((S) => T) => (S[]) => T[]");
AssertType(e, "<K>(string, ?K) => number");

declare function map2<S extends { length: number }, T>(f: (x: S) => T): (a: S[]) => T[];
let e2 = <K>(x: string, y?: K) => x.length;
AssertType(e2, "<K>(string, ?K) => number");
AssertType(<K>(x: string, y?: K) => x.length, "<K>(string, ?K) => number");
AssertType(x, "string");
AssertType(y, "K");
AssertType(x.length, "number");

let r100 = map2(e2); // type arg inference should fail for S since a generic lambda is not inferentially typed. Falls back to { length: number 
AssertType(r100, "<K>(string[]) => number[]");

AssertType(map2(e2), "<K>(string[]) => number[]");

AssertType(map2, "<S extends { length: number; }, T>((S) => T) => (S[]) => T[]");

AssertType(e2, "<K>(string, ?K) => number");
}


