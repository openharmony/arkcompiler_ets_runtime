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

// === tests/cases/compiler/overloadResolutionOverNonCTLambdas.ts ===
declare function AssertType(value:any, type:string):void;
module Bugs {
  class A {
  }
  
  // replace(searchValue: RegExp, replaceValue: (substring: string, ...args: any[]) => string): string;
  function bug2(message:string, ...args:any[]):string {
    let result= message.replace(/\{(\d+)\}/g, function(match, ...rest) {
AssertType(result, "string");
AssertType(message.replace, "{ (union, string): string; (union, (string, ...any[]) => string): string; }");
AssertType(/\{(\d+)\}/g, "RegExp");
AssertType(function(match, ...rest) {      let index= rest[0];      return typeof args[index] !== 'undefined'        ? args[index]        : match;    }, "(string, ...any[]) => any");
AssertType(match, "string");
AssertType(rest, "any[]");
AssertType(message.replace(/\{(\d+)\}/g, function(match, ...rest) {      let index= rest[0];      return typeof args[index] !== 'undefined'        ? args[index]        : match;    }), "string");

      let index= rest[0];
AssertType(index, "any");
AssertType(rest[0], "any");
AssertType(rest, "any[]");
AssertType(0, "int");

AssertType(typeof args[index] !== 'undefined'        ? args[index]        : match, "any");
AssertType(typeof args[index] !== 'undefined', "boolean");
AssertType(typeof args[index], "union");
AssertType(args[index], "any");
AssertType(args, "any[]");
AssertType(index, "any");
AssertType('undefined', "string");
      return typeof args[index] !== 'undefined'

        ? args[index]
AssertType(args[index], "any");
AssertType(args, "any[]");
AssertType(index, "any");

        : match;
AssertType(match, "string");

    });
AssertType(result, "string");
    return result;
  }
}

function bug3(f:(x:string)=>string) { 
AssertType(f("s"), "string");
return f("s") 

AssertType(f, "(string) => string");

AssertType("s", "string");
}

function fprime(x:string):string { 
AssertType(x, "string");
return x; 
}

bug3(fprime);
AssertType(bug3(fprime), "string");
AssertType(bug3, "((string) => string) => string");
AssertType(fprime, "(string) => string");

bug3(function(x:string):string { 
AssertType(bug3(function(x:string):string { return x; }), "string");
AssertType(bug3, "((string) => string) => string");
AssertType(function(x:string):string { return x; }, "(string) => string");
AssertType(x, "string");
AssertType(x, "string");
return x; });


