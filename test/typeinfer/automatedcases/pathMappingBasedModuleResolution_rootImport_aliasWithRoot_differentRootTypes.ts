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

// === /root/a.ts ===
declare function AssertType(value:any, type:string):void;

import { foo as foo1 } from "/foo";
import { bar as bar1 } from "/bar";
import { foo as foo2 } from "c:/foo";
import { bar as bar2 } from "c:/bar";
import { foo as foo3 } from "c:\\foo";
import { bar as bar3 } from "c:\\bar";
import { foo as foo4 } from "//server/foo";
import { bar as bar4 } from "//server/bar";
import { foo as foo5 } from "\\\\server\\foo";
import { bar as bar5 } from "\\\\server\\bar";
import { foo as foo6 } from "file:///foo";
import { bar as bar6 } from "file:///bar";
import { foo as foo7 } from "file://c:/foo";
import { bar as bar7 } from "file://c:/bar";
import { foo as foo8 } from "file://server/foo";
import { bar as bar8 } from "file://server/bar";
import { foo as foo9 } from "http://server/foo";
import { bar as bar9 } from "http://server/bar";


// === /root/src/foo.ts ===
declare function AssertType(value:any, type:string):void;

export function foo() {}


// === /root/src/bar.js ===
declare function AssertType(value:any, type:string):void;

export function bar() {}


