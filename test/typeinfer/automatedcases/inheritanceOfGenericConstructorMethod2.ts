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

// === tests/cases/compiler/inheritanceOfGenericConstructorMethod2.ts ===
declare function AssertType(value:any, type:string):void;
module M {
   export class C1 { }
   export class C2<T> { }
}
module N {
   export class D1 extends M.C1 { }
   export class D2<T> extends M.C2<T> { }
}

let c = new M.C2<number>(); // no error
AssertType(c, "M.C2<number>");
AssertType(new M.C2<number>(), "M.C2<number>");
AssertType(M.C2, "typeof M.C2");

let n = new N.D1(); // no error
AssertType(n, "N.D1");
AssertType(new N.D1(), "N.D1");
AssertType(N.D1, "typeof N.D1");

let n2 = new N.D2<number>(); // error
AssertType(n2, "N.D2<number>");
AssertType(new N.D2<number>(), "N.D2<number>");
AssertType(N.D2, "typeof N.D2");

let n3 = new N.D2(); // no error, D2<any>
AssertType(n3, "N.D2<unknown>");
AssertType(new N.D2(), "N.D2<unknown>");
AssertType(N.D2, "typeof N.D2");


