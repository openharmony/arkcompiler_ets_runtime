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

// === tests/cases/compiler/convertClassExpressionToFunctionFromObjectProperty1.ts ===
declare function AssertType(value:any, type:string):void;
const foo: any = {};
AssertType(foo, "any");
AssertType({}, "{}");

// properties
foo.x = class {
AssertType(foo.x = class {    constructor () {}}, "typeof x");
AssertType(foo.x, "any");
AssertType(class {    constructor () {}}, "typeof x");

    constructor () {}
}
foo.y = class {
AssertType(foo.y = class {    constructor () {}}, "typeof y");
AssertType(foo.y, "any");
AssertType(class {    constructor () {}}, "typeof y");

    constructor () {}
}

// keywords
foo.break = class {
AssertType(foo.break = class {    constructor () {}}, "typeof break");
AssertType(foo.break, "any");
AssertType(class {    constructor () {}}, "typeof break");

    constructor () {}
}
foo.case = class {
AssertType(foo.case = class {    constructor () {}}, "typeof case");
AssertType(foo.case, "any");
AssertType(class {    constructor () {}}, "typeof case");

    constructor () {}
}
foo.catch = class {
AssertType(foo.catch = class {    constructor () {}}, "typeof catch");
AssertType(foo.catch, "any");
AssertType(class {    constructor () {}}, "typeof catch");

    constructor () {}
}
foo.class = class {
AssertType(foo.class = class {    constructor () {}}, "typeof class");
AssertType(foo.class, "any");
AssertType(class {    constructor () {}}, "typeof class");

    constructor () {}
}
foo.const = class {
AssertType(foo.const = class {    constructor () {}}, "typeof const");
AssertType(foo.const, "any");
AssertType(class {    constructor () {}}, "typeof const");

    constructor () {}
}
foo.continue = class {
AssertType(foo.continue = class {    constructor () {}}, "typeof continue");
AssertType(foo.continue, "any");
AssertType(class {    constructor () {}}, "typeof continue");

    constructor () {}
}
foo.debugger = class {
AssertType(foo.debugger = class {    constructor () {}}, "typeof debugger");
AssertType(foo.debugger, "any");
AssertType(class {    constructor () {}}, "typeof debugger");

    constructor () {}
}
foo.default = class {
AssertType(foo.default = class {    constructor () {}}, "typeof default");
AssertType(foo.default, "any");
AssertType(class {    constructor () {}}, "typeof default");

    constructor () {}
}
foo.delete = class {
AssertType(foo.delete = class {    constructor () {}}, "typeof delete");
AssertType(foo.delete, "any");
AssertType(class {    constructor () {}}, "typeof delete");

    constructor () {}
}
foo.do = class {
AssertType(foo.do = class {    constructor () {}}, "typeof do");
AssertType(foo.do, "any");
AssertType(class {    constructor () {}}, "typeof do");

    constructor () {}
}
foo.else = class {
AssertType(foo.else = class {    constructor () {}}, "typeof else");
AssertType(foo.else, "any");
AssertType(class {    constructor () {}}, "typeof else");

    constructor () {}
}
foo.enum = class {
AssertType(foo.enum = class {    constructor () {}}, "typeof enum");
AssertType(foo.enum, "any");
AssertType(class {    constructor () {}}, "typeof enum");

    constructor () {}
}
foo.export = class {
AssertType(foo.export = class {    constructor () {}}, "typeof export");
AssertType(foo.export, "any");
AssertType(class {    constructor () {}}, "typeof export");

    constructor () {}
}
foo.extends = class {
AssertType(foo.extends = class {    constructor () {}}, "typeof extends");
AssertType(foo.extends, "any");
AssertType(class {    constructor () {}}, "typeof extends");

    constructor () {}
}
foo.false = class {
AssertType(foo.false = class {    constructor () {}}, "typeof false");
AssertType(foo.false, "any");
AssertType(class {    constructor () {}}, "typeof false");

    constructor () {}
}
foo.finally = class {
AssertType(foo.finally = class {    constructor () {}}, "typeof finally");
AssertType(foo.finally, "any");
AssertType(class {    constructor () {}}, "typeof finally");

    constructor () {}
}
foo.for = class {
AssertType(foo.for = class {    constructor () {}}, "typeof for");
AssertType(foo.for, "any");
AssertType(class {    constructor () {}}, "typeof for");

    constructor () {}
}
foo.function = class {
AssertType(foo.function = class {    constructor () {}}, "typeof function");
AssertType(foo.function, "any");
AssertType(class {    constructor () {}}, "typeof function");

    constructor () {}
}
foo.if = class {
AssertType(foo.if = class {    constructor () {}}, "typeof if");
AssertType(foo.if, "any");
AssertType(class {    constructor () {}}, "typeof if");

    constructor () {}
}
foo.import = class {
AssertType(foo.import = class {    constructor () {}}, "typeof import");
AssertType(foo.import, "any");
AssertType(class {    constructor () {}}, "typeof import");

    constructor () {}
}
foo.in = class {
AssertType(foo.in = class {    constructor () {}}, "typeof in");
AssertType(foo.in, "any");
AssertType(class {    constructor () {}}, "typeof in");

    constructor () {}
}
foo.instanceof = class {
AssertType(foo.instanceof = class {    constructor () {}}, "typeof instanceof");
AssertType(foo.instanceof, "any");
AssertType(class {    constructor () {}}, "typeof instanceof");

    constructor () {}
}
foo.new = class {
AssertType(foo.new = class {    constructor () {}}, "typeof new");
AssertType(foo.new, "any");
AssertType(class {    constructor () {}}, "typeof new");

    constructor () {}
}
foo.null = class {
AssertType(foo.null = class {    constructor () {}}, "typeof null");
AssertType(foo.null, "any");
AssertType(class {    constructor () {}}, "typeof null");

    constructor () {}
}
foo.
AssertType(foo.return = class {    constructor () {}}, "typeof return");
AssertType(foo.return, "any");
AssertType(class {    constructor () {}}, "typeof return");
return = class {

    constructor () {}
}
foo.super = class {
AssertType(foo.super = class {    constructor () {}}, "typeof super");
AssertType(foo.super, "any");
AssertType(class {    constructor () {}}, "typeof super");

    constructor () {}
}
foo.switch = class {
AssertType(foo.switch = class {    constructor () {}}, "typeof switch");
AssertType(foo.switch, "any");
AssertType(class {    constructor () {}}, "typeof switch");

    constructor () {}
}
foo.this = class {
AssertType(foo.this = class {    constructor () {}}, "typeof this");
AssertType(foo.this, "any");
AssertType(class {    constructor () {}}, "typeof this");

    constructor () {}
}
foo.throw = class {
AssertType(foo.throw = class {    constructor () {}}, "typeof throw");
AssertType(foo.throw, "any");
AssertType(class {    constructor () {}}, "typeof throw");

    constructor () {}
}
foo.true = class {
AssertType(foo.true = class {    constructor () {}}, "typeof true");
AssertType(foo.true, "any");
AssertType(class {    constructor () {}}, "typeof true");

    constructor () {}
}
foo.try = class {
AssertType(foo.try = class {    constructor () {}}, "typeof try");
AssertType(foo.try, "any");
AssertType(class {    constructor () {}}, "typeof try");

    constructor () {}
}
foo.typeof = class {
AssertType(foo.typeof = class {    constructor () {}}, "typeof typeof");
AssertType(foo.typeof, "any");
AssertType(class {    constructor () {}}, "typeof typeof");

    constructor () {}
}
foo.let = class {
AssertType(foo.let = class {    constructor () {}}, "typeof let");
AssertType(foo.let, "any");
AssertType(class {    constructor () {}}, "typeof let");

    constructor () {}
}
foo.void = class {
AssertType(foo.void = class {    constructor () {}}, "typeof void");
AssertType(foo.void, "any");
AssertType(class {    constructor () {}}, "typeof void");

    constructor () {}
}
foo.while = class {
AssertType(foo.while = class {    constructor () {}}, "typeof while");
AssertType(foo.while, "any");
AssertType(class {    constructor () {}}, "typeof while");

    constructor () {}
}
foo.with = class {
AssertType(foo.with = class {    constructor () {}}, "typeof with");
AssertType(foo.with, "any");
AssertType(class {    constructor () {}}, "typeof with");

    constructor () {}
}
foo.implements = class {
AssertType(foo.implements = class {    constructor () {}}, "typeof implements");
AssertType(foo.implements, "any");
AssertType(class {    constructor () {}}, "typeof implements");

    constructor () {}
}
foo.interface = class {
AssertType(foo.interface = class {    constructor () {}}, "typeof interface");
AssertType(foo.interface, "any");
AssertType(class {    constructor () {}}, "typeof interface");

    constructor () {}
}
foo.let = class {
AssertType(foo.let = class {    constructor () {}}, "typeof let");
AssertType(foo.let, "any");
AssertType(class {    constructor () {}}, "typeof let");

    constructor () {}
}
foo.package = class {
AssertType(foo.package = class {    constructor () {}}, "typeof package");
AssertType(foo.package, "any");
AssertType(class {    constructor () {}}, "typeof package");

    constructor () {}
}
foo.private = class {
AssertType(foo.private = class {    constructor () {}}, "typeof private");
AssertType(foo.private, "any");
AssertType(class {    constructor () {}}, "typeof private");

    constructor () {}
}
foo.protected = class {
AssertType(foo.protected = class {    constructor () {}}, "typeof protected");
AssertType(foo.protected, "any");
AssertType(class {    constructor () {}}, "typeof protected");

    constructor () {}
}
foo.public = class {
AssertType(foo.public = class {    constructor () {}}, "typeof public");
AssertType(foo.public, "any");
AssertType(class {    constructor () {}}, "typeof public");

    constructor () {}
}
foo.static = class {
AssertType(foo.static = class {    constructor () {}}, "typeof static");
AssertType(foo.static, "any");
AssertType(class {    constructor () {}}, "typeof static");

    constructor () {}
}
foo.yield = class {
AssertType(foo.yield = class {    constructor () {}}, "typeof yield");
AssertType(foo.yield, "any");
AssertType(class {    constructor () {}}, "typeof yield");

    constructor () {}
}
foo.abstract = class {
AssertType(foo.abstract = class {    constructor () {}}, "typeof abstract");
AssertType(foo.abstract, "any");
AssertType(class {    constructor () {}}, "typeof abstract");

    constructor () {}
}
foo.as = class {
AssertType(foo.as = class {    constructor () {}}, "typeof as");
AssertType(foo.as, "any");
AssertType(class {    constructor () {}}, "typeof as");

    constructor () {}
}
foo.asserts = class {
AssertType(foo.asserts = class {    constructor () {}}, "typeof asserts");
AssertType(foo.asserts, "any");
AssertType(class {    constructor () {}}, "typeof asserts");

    constructor () {}
}
foo.any = class {
AssertType(foo.any = class {    constructor () {}}, "typeof any");
AssertType(foo.any, "any");
AssertType(class {    constructor () {}}, "typeof any");

    constructor () {}
}
foo.async = class {
AssertType(foo.async = class {    constructor () {}}, "typeof async");
AssertType(foo.async, "any");
AssertType(class {    constructor () {}}, "typeof async");

    constructor () {}
}
foo.await = class {
AssertType(foo.await = class {    constructor () {}}, "typeof await");
AssertType(foo.await, "any");
AssertType(class {    constructor () {}}, "typeof await");

    constructor () {}
}
foo.boolean = class {
AssertType(foo.boolean = class {    constructor () {}}, "typeof boolean");
AssertType(foo.boolean, "any");
AssertType(class {    constructor () {}}, "typeof boolean");

    constructor () {}
}
foo.constructor = class {
AssertType(foo.constructor = class {    constructor () {}}, "typeof constructor");
AssertType(foo.constructor, "any");
AssertType(class {    constructor () {}}, "typeof constructor");

    constructor () {}
}
foo.declare = class {
AssertType(foo.declare = class {    constructor () {}}, "typeof declare");
AssertType(foo.declare, "any");
AssertType(class {    constructor () {}}, "typeof declare");

    constructor () {}
}
foo.get = class {
AssertType(foo.get = class {    constructor () {}}, "typeof get");
AssertType(foo.get, "any");
AssertType(class {    constructor () {}}, "typeof get");

    constructor () {}
}
foo.infer = class {
AssertType(foo.infer = class {    constructor () {}}, "typeof infer");
AssertType(foo.infer, "any");
AssertType(class {    constructor () {}}, "typeof infer");

    constructor () {}
}
foo.is = class {
AssertType(foo.is = class {    constructor () {}}, "typeof is");
AssertType(foo.is, "any");
AssertType(class {    constructor () {}}, "typeof is");

    constructor () {}
}
foo.keyof = class {
AssertType(foo.keyof = class {    constructor () {}}, "typeof keyof");
AssertType(foo.keyof, "any");
AssertType(class {    constructor () {}}, "typeof keyof");

    constructor () {}
}
foo.module = class {
AssertType(foo.module = class {    constructor () {}}, "typeof module");
AssertType(foo.module, "any");
AssertType(class {    constructor () {}}, "typeof module");

    constructor () {}
}
foo.namespace = class {
AssertType(foo.namespace = class {    constructor () {}}, "typeof namespace");
AssertType(foo.namespace, "any");
AssertType(class {    constructor () {}}, "typeof namespace");

    constructor () {}
}
foo.never = class {
AssertType(foo.never = class {    constructor () {}}, "typeof never");
AssertType(foo.never, "any");
AssertType(class {    constructor () {}}, "typeof never");

    constructor () {}
}
foo.readonly = class {
AssertType(foo.readonly = class {    constructor () {}}, "typeof readonly");
AssertType(foo.readonly, "any");
AssertType(class {    constructor () {}}, "typeof readonly");

    constructor () {}
}
foo.require = class {
AssertType(foo.require = class {    constructor () {}}, "typeof require");
AssertType(foo.require, "any");
AssertType(class {    constructor () {}}, "typeof require");

    constructor () {}
}
foo.number = class {
AssertType(foo.number = class {    constructor () {}}, "typeof number");
AssertType(foo.number, "any");
AssertType(class {    constructor () {}}, "typeof number");

    constructor () {}
}
foo.object = class {
AssertType(foo.object = class {    constructor () {}}, "typeof object");
AssertType(foo.object, "any");
AssertType(class {    constructor () {}}, "typeof object");

    constructor () {}
}
foo.set = class {
AssertType(foo.set = class {    constructor () {}}, "typeof set");
AssertType(foo.set, "any");
AssertType(class {    constructor () {}}, "typeof set");

    constructor () {}
}
foo.string = class {
AssertType(foo.string = class {    constructor () {}}, "typeof string");
AssertType(foo.string, "any");
AssertType(class {    constructor () {}}, "typeof string");

    constructor () {}
}
foo.symbol = class {
AssertType(foo.symbol = class {    constructor () {}}, "typeof symbol");
AssertType(foo.symbol, "any");
AssertType(class {    constructor () {}}, "typeof symbol");

    constructor () {}
}
foo.type = class {
AssertType(foo.type = class {    constructor () {}}, "typeof type");
AssertType(foo.type, "any");
AssertType(class {    constructor () {}}, "typeof type");

    constructor () {}
}
foo.undefined = class {
AssertType(foo.undefined = class {    constructor () {}}, "typeof undefined");
AssertType(foo.undefined, "any");
AssertType(class {    constructor () {}}, "typeof undefined");

    constructor () {}
}
foo.unique = class {
AssertType(foo.unique = class {    constructor () {}}, "typeof unique");
AssertType(foo.unique, "any");
AssertType(class {    constructor () {}}, "typeof unique");

    constructor () {}
}
foo.unknown = class {
AssertType(foo.unknown = class {    constructor () {}}, "typeof unknown");
AssertType(foo.unknown, "any");
AssertType(class {    constructor () {}}, "typeof unknown");

    constructor () {}
}
foo.from = class {
AssertType(foo.from = class {    constructor () {}}, "typeof from");
AssertType(foo.from, "any");
AssertType(class {    constructor () {}}, "typeof from");

    constructor () {}
}
foo.global = class {
AssertType(foo.global = class {    constructor () {}}, "typeof global");
AssertType(foo.global, "any");
AssertType(class {    constructor () {}}, "typeof global");

    constructor () {}
}
foo.bigint = class {
AssertType(foo.bigint = class {    constructor () {}}, "typeof bigint");
AssertType(foo.bigint, "any");
AssertType(class {    constructor () {}}, "typeof bigint");

    constructor () {}
}
foo.of = class {
AssertType(foo.of = class {    constructor () {}}, "typeof of");
AssertType(foo.of, "any");
AssertType(class {    constructor () {}}, "typeof of");

    constructor () {}
}


