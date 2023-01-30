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

// === tests/cases/compiler/test.ts ===
declare function AssertType(value:any, type:string):void;
import { Cls } from "./m1";
import "m2";
import "m4";
let c: Cls;
AssertType(c, "Cls");

c.foo().toExponential();
AssertType(c.foo().toExponential(), "string");
AssertType(c.foo().toExponential, "(?number) => string");

c.bar().toLowerCase();
AssertType(c.bar().toLowerCase(), "string");
AssertType(c.bar().toLowerCase, "() => string");

c.baz1().x.toExponential();
AssertType(c.baz1().x.toExponential(), "string");
AssertType(c.baz1().x.toExponential, "(?number) => string");

c.baz2().x.toLowerCase();
AssertType(c.baz2().x.toLowerCase(), "string");
AssertType(c.baz2().x.toLowerCase, "() => string");


