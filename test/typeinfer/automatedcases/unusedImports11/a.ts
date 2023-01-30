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

// === tests/cases/compiler/a.ts ===
declare function AssertType(value:any, type:string):void;
import { Member } from './b';
import d, { Member as M } from './b';
import * as ns from './b';
import * as r from "./b";

new Member();
AssertType(new Member(), "Member");
AssertType(Member, "typeof Member");

new d();
AssertType(new d(), "Member");
AssertType(d, "typeof Member");

new M();
AssertType(new M(), "Member");
AssertType(M, "typeof Member");

new ns.Member();
AssertType(new ns.Member(), "Member");
AssertType(ns.Member, "typeof Member");

new r.Member();
AssertType(new r.Member(), "Member");
AssertType(r.Member, "typeof Member");


