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

// === tests/cases/compiler/m4.ts ===
declare function AssertType(value:any, type:string):void;
import {Cls} from "./m1";
import {C1, C2} from "./m3";
(<any>Cls.prototype).baz1 = function() { 
AssertType((<any>Cls.prototype).baz1 = function() { return undefined }, "() => any");
AssertType((<any>Cls.prototype).baz1, "any");
AssertType(function() { return undefined }, "() => any");
AssertType(undefined, "undefined");
return undefined };

(<any>Cls.prototype).baz2 = function() { 
AssertType((<any>Cls.prototype).baz2 = function() { return undefined }, "() => any");
AssertType((<any>Cls.prototype).baz2, "any");
AssertType(function() { return undefined }, "() => any");
AssertType(undefined, "undefined");
return undefined };

declare module "./m1" {
    interface Cls {
        baz1(): C1;
    }
}

declare module "./m1" {
    interface Cls {
        baz2(): C2;
    }
}


