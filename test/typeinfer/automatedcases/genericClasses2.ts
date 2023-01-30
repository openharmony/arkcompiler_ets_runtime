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

// === tests/cases/compiler/genericClasses2.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo<T> {
	a: T;
}

class C<T> {
	public x: T;
	public y: Foo<T>;
	public z: Foo<number>;
}

let v1 : C<string>;
AssertType(v1, "C<string>");

let y = v1.x; // should be 'string'
AssertType(y, "string");
AssertType(v1.x, "string");

let w = v1.y.a; // should be 'string'
AssertType(w, "string");
AssertType(v1.y.a, "string");

let z = v1.z.a; // should be 'number'
AssertType(z, "number");
AssertType(v1.z.a, "number");


