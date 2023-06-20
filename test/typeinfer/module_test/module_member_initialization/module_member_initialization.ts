/*
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
 */

declare function AssertType(value:any, type:string):void;

import {foo1, foo2} from "./export"

let f1 = new foo1();
let f2 = new foo2();
AssertType(f1.b1, "(string, string) => string");
AssertType(f1.b2, "number");
AssertType(f1.b1("a", "b"), "string");

AssertType(f2.a1, "number");
AssertType(f2.a2, "string");
AssertType(f2.a3, "foo1");
AssertType(f2.a3.b2, "number");
AssertType(f2.a4, "(number, number) => number");