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

// === tests/cases/compiler/fatarrowfunctionsInFunctions.ts ===
declare function AssertType(value:any, type:string):void;
declare function setTimeout(expression: any, msec?: number, language?: any): number;

let messenger = {
AssertType(messenger, "{ message: string; start: () => void; }");
AssertType({    message: "Hello World",    start: function() {        let _self = this;        setTimeout(function() {            _self.message.toString();         }, 3000);     }}, "{ message: string; start: () => void; }");

    message: "Hello World",
AssertType(message, "string");
AssertType("Hello World", "string");

    start: function() {
AssertType(start, "() => void");
AssertType(function() {        let _self = this;        setTimeout(function() {            _self.message.toString();         }, 3000);     }, "() => void");

        let _self = this;
AssertType(_self, "any");
AssertType(this, "any");

        setTimeout(function() {
AssertType(setTimeout(function() {            _self.message.toString();         }, 3000), "number");
AssertType(setTimeout, "{ (TimerHandler, ?number, ...any[]): number; (any, ?number, ?any): number; }");
AssertType(function() {            _self.message.toString();         }, "() => void");

            _self.message.toString(); 
AssertType(_self.message.toString(), "any");
AssertType(_self.message.toString, "any");
AssertType(_self.message, "any");

        }, 3000); 
AssertType(3000, "int");
    }
}; 
messenger.start(); 
AssertType(messenger.start(), "void");
AssertType(messenger.start, "() => void");


