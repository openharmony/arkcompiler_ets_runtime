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

// === tests/cases/conformance/declarationEmit/nullPropertyName.ts ===
declare function AssertType(value:any, type:string):void;
function foo() {}
// properties
foo.x = 1;
AssertType(foo.x = 1, "int");
AssertType(foo.x, "number");
AssertType(1, "int");

foo.y = 1;
AssertType(foo.y = 1, "int");
AssertType(foo.y, "number");
AssertType(1, "int");

// keywords
foo.break = 1;
AssertType(foo.break = 1, "int");
AssertType(foo.break, "number");
AssertType(1, "int");

foo.case = 1;
AssertType(foo.case = 1, "int");
AssertType(foo.case, "number");
AssertType(1, "int");

foo.catch = 1;
AssertType(foo.catch = 1, "int");
AssertType(foo.catch, "number");
AssertType(1, "int");

foo.class = 1;
AssertType(foo.class = 1, "int");
AssertType(foo.class, "number");
AssertType(1, "int");

foo.const = 1;
AssertType(foo.const = 1, "int");
AssertType(foo.const, "number");
AssertType(1, "int");

foo.continue = 1;
AssertType(foo.continue = 1, "int");
AssertType(foo.continue, "number");
AssertType(1, "int");

foo.debugger = 1;
AssertType(foo.debugger = 1, "int");
AssertType(foo.debugger, "number");
AssertType(1, "int");

foo.default = 1;
AssertType(foo.default = 1, "int");
AssertType(foo.default, "number");
AssertType(1, "int");

foo.delete = 1;
AssertType(foo.delete = 1, "int");
AssertType(foo.delete, "number");
AssertType(1, "int");

foo.do = 1;
AssertType(foo.do = 1, "int");
AssertType(foo.do, "number");
AssertType(1, "int");

foo.else = 1;
AssertType(foo.else = 1, "int");
AssertType(foo.else, "number");
AssertType(1, "int");

foo.enum = 1;
AssertType(foo.enum = 1, "int");
AssertType(foo.enum, "number");
AssertType(1, "int");

foo.export = 1;
AssertType(foo.export = 1, "int");
AssertType(foo.export, "number");
AssertType(1, "int");

foo.extends = 1;
AssertType(foo.extends = 1, "int");
AssertType(foo.extends, "number");
AssertType(1, "int");

foo.false = 1;
AssertType(foo.false = 1, "int");
AssertType(foo.false, "number");
AssertType(1, "int");

foo.finally = 1;
AssertType(foo.finally = 1, "int");
AssertType(foo.finally, "number");
AssertType(1, "int");

foo.for = 1;
AssertType(foo.for = 1, "int");
AssertType(foo.for, "number");
AssertType(1, "int");

foo.function = 1;
AssertType(foo.function = 1, "int");
AssertType(foo.function, "number");
AssertType(1, "int");

foo.if = 1;
AssertType(foo.if = 1, "int");
AssertType(foo.if, "number");
AssertType(1, "int");

foo.import = 1;
AssertType(foo.import = 1, "int");
AssertType(foo.import, "number");
AssertType(1, "int");

foo.in = 1;
AssertType(foo.in = 1, "int");
AssertType(foo.in, "number");
AssertType(1, "int");

foo.instanceof = 1;
AssertType(foo.instanceof = 1, "int");
AssertType(foo.instanceof, "number");
AssertType(1, "int");

foo.new = 1;
AssertType(foo.new = 1, "int");
AssertType(foo.new, "number");
AssertType(1, "int");

foo.null = 1;
AssertType(foo.null = 1, "int");
AssertType(foo.null, "number");
AssertType(1, "int");

foo.
AssertType(foo.return = 1, "int");
AssertType(foo.return, "number");
AssertType(1, "int");
return = 1;

foo.super = 1;
AssertType(foo.super = 1, "int");
AssertType(foo.super, "number");
AssertType(1, "int");

foo.switch = 1;
AssertType(foo.switch = 1, "int");
AssertType(foo.switch, "number");
AssertType(1, "int");

foo.this = 1;
AssertType(foo.this = 1, "int");
AssertType(foo.this, "number");
AssertType(1, "int");

foo.throw = 1;
AssertType(foo.throw = 1, "int");
AssertType(foo.throw, "number");
AssertType(1, "int");

foo.true = 1;
AssertType(foo.true = 1, "int");
AssertType(foo.true, "number");
AssertType(1, "int");

foo.try = 1;
AssertType(foo.try = 1, "int");
AssertType(foo.try, "number");
AssertType(1, "int");

foo.typeof = 1;
AssertType(foo.typeof = 1, "int");
AssertType(foo.typeof, "number");
AssertType(1, "int");

foo.let = 1;
AssertType(foo.let = 1, "int");
AssertType(foo.let, "number");
AssertType(1, "int");

foo.void = 1;
AssertType(foo.void = 1, "int");
AssertType(foo.void, "number");
AssertType(1, "int");

foo.while = 1;
AssertType(foo.while = 1, "int");
AssertType(foo.while, "number");
AssertType(1, "int");

foo.with = 1;
AssertType(foo.with = 1, "int");
AssertType(foo.with, "number");
AssertType(1, "int");

foo.implements = 1;
AssertType(foo.implements = 1, "int");
AssertType(foo.implements, "number");
AssertType(1, "int");

foo.interface = 1;
AssertType(foo.interface = 1, "int");
AssertType(foo.interface, "number");
AssertType(1, "int");

foo.let = 1;
AssertType(foo.let = 1, "int");
AssertType(foo.let, "number");
AssertType(1, "int");

foo.package = 1;
AssertType(foo.package = 1, "int");
AssertType(foo.package, "number");
AssertType(1, "int");

foo.private = 1;
AssertType(foo.private = 1, "int");
AssertType(foo.private, "number");
AssertType(1, "int");

foo.protected = 1;
AssertType(foo.protected = 1, "int");
AssertType(foo.protected, "number");
AssertType(1, "int");

foo.public = 1;
AssertType(foo.public = 1, "int");
AssertType(foo.public, "number");
AssertType(1, "int");

foo.static = 1;
AssertType(foo.static = 1, "int");
AssertType(foo.static, "number");
AssertType(1, "int");

foo.yield = 1;
AssertType(foo.yield = 1, "int");
AssertType(foo.yield, "number");
AssertType(1, "int");

foo.abstract = 1;
AssertType(foo.abstract = 1, "int");
AssertType(foo.abstract, "number");
AssertType(1, "int");

foo.as = 1;
AssertType(foo.as = 1, "int");
AssertType(foo.as, "number");
AssertType(1, "int");

foo.asserts = 1;
AssertType(foo.asserts = 1, "int");
AssertType(foo.asserts, "number");
AssertType(1, "int");

foo.any = 1;
AssertType(foo.any = 1, "int");
AssertType(foo.any, "number");
AssertType(1, "int");

foo.async = 1;
AssertType(foo.async = 1, "int");
AssertType(foo.async, "number");
AssertType(1, "int");

foo.await = 1;
AssertType(foo.await = 1, "int");
AssertType(foo.await, "number");
AssertType(1, "int");

foo.boolean = 1;
AssertType(foo.boolean = 1, "int");
AssertType(foo.boolean, "number");
AssertType(1, "int");

foo.constructor = 1;
AssertType(foo.constructor = 1, "int");
AssertType(foo.constructor, "number");
AssertType(1, "int");

foo.declare = 1;
AssertType(foo.declare = 1, "int");
AssertType(foo.declare, "number");
AssertType(1, "int");

foo.get = 1;
AssertType(foo.get = 1, "int");
AssertType(foo.get, "number");
AssertType(1, "int");

foo.infer = 1;
AssertType(foo.infer = 1, "int");
AssertType(foo.infer, "number");
AssertType(1, "int");

foo.is = 1;
AssertType(foo.is = 1, "int");
AssertType(foo.is, "number");
AssertType(1, "int");

foo.keyof = 1;
AssertType(foo.keyof = 1, "int");
AssertType(foo.keyof, "number");
AssertType(1, "int");

foo.module = 1;
AssertType(foo.module = 1, "int");
AssertType(foo.module, "number");
AssertType(1, "int");

foo.namespace = 1;
AssertType(foo.namespace = 1, "int");
AssertType(foo.namespace, "number");
AssertType(1, "int");

foo.never = 1;
AssertType(foo.never = 1, "int");
AssertType(foo.never, "number");
AssertType(1, "int");

foo.readonly = 1;
AssertType(foo.readonly = 1, "int");
AssertType(foo.readonly, "number");
AssertType(1, "int");

foo.require = 1;
AssertType(foo.require = 1, "int");
AssertType(foo.require, "number");
AssertType(1, "int");

foo.number = 1;
AssertType(foo.number = 1, "int");
AssertType(foo.number, "number");
AssertType(1, "int");

foo.object = 1;
AssertType(foo.object = 1, "int");
AssertType(foo.object, "number");
AssertType(1, "int");

foo.set = 1;
AssertType(foo.set = 1, "int");
AssertType(foo.set, "number");
AssertType(1, "int");

foo.string = 1;
AssertType(foo.string = 1, "int");
AssertType(foo.string, "number");
AssertType(1, "int");

foo.symbol = 1;
AssertType(foo.symbol = 1, "int");
AssertType(foo.symbol, "number");
AssertType(1, "int");

foo.type = 1;
AssertType(foo.type = 1, "int");
AssertType(foo.type, "number");
AssertType(1, "int");

foo.undefined = 1;
AssertType(foo.undefined = 1, "int");
AssertType(foo.undefined, "number");
AssertType(1, "int");

foo.unique = 1;
AssertType(foo.unique = 1, "int");
AssertType(foo.unique, "number");
AssertType(1, "int");

foo.unknown = 1;
AssertType(foo.unknown = 1, "int");
AssertType(foo.unknown, "number");
AssertType(1, "int");

foo.from = 1;
AssertType(foo.from = 1, "int");
AssertType(foo.from, "number");
AssertType(1, "int");

foo.global = 1;
AssertType(foo.global = 1, "int");
AssertType(foo.global, "number");
AssertType(1, "int");

foo.bigint = 1;
AssertType(foo.bigint = 1, "int");
AssertType(foo.bigint, "number");
AssertType(1, "int");

foo.of = 1;
AssertType(foo.of = 1, "int");
AssertType(foo.of, "number");
AssertType(1, "int");


