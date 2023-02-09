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

// === tests/cases/conformance/types/uniqueSymbol/uniqueSymbolsDeclarationsErrors.ts ===
declare function AssertType(value:any, type:string):void;
declare const s: unique symbol;
AssertType(s, "unique symbol");

interface I { readonly readonlyType: unique symbol; }

// not allowed when emitting declarations

export const obj = {
AssertType(obj, "{ method1(typeof s): typeof s; method2(I["readonlyType"]): I["readonlyType"]; }");
AssertType({    method1(p: typeof s): typeof s {        return p;    },    method2(p: I["readonlyType"]): I["readonlyType"] {        return p;    }}, "{ method1(typeof s): typeof s; method2(I["readonlyType"]): I["readonlyType"]; }");

    method1(p: typeof s): typeof s {
AssertType(method1, "(typeof s) => typeof s");
AssertType(p, "unique symbol");
AssertType(s, "unique symbol");
AssertType(s, "unique symbol");

AssertType(p, "unique symbol");
        return p;

    },
    method2(p: I["readonlyType"]): I["readonlyType"] {
AssertType(method2, "(I["readonlyType"]) => I["readonlyType"]");
AssertType(p, "unique symbol");

AssertType(p, "unique symbol");
        return p;
    }
};

export const classExpression = class {
AssertType(classExpression, "typeof classExpression");
AssertType(class {    method1(p: typeof s): typeof s {        return p;    }    method2(p: I["readonlyType"]): I["readonlyType"] {        return p;    }}, "typeof classExpression");

    method1(p: typeof s): typeof s {
AssertType(method1, "(typeof s) => typeof s");
AssertType(p, "unique symbol");
AssertType(s, "unique symbol");
AssertType(s, "unique symbol");

AssertType(p, "unique symbol");
        return p;
    }
    method2(p: I["readonlyType"]): I["readonlyType"] {
AssertType(method2, "(I["readonlyType"]) => I["readonlyType"]");
AssertType(p, "unique symbol");

AssertType(p, "unique symbol");
        return p;
    }
};

export function funcInferredReturnType(obj: { method(p: typeof s): void }) {
AssertType(obj, "{ method(unique symbol): void; }");
    return obj;
}

export interface InterfaceWithPrivateNamedProperties {
    [s]: any;
}

export interface InterfaceWithPrivateNamedMethods {
    [s](): any;
}

export type TypeLiteralWithPrivateNamedProperties = {
    [s]: any;
}

export type TypeLiteralWithPrivateNamedMethods = {
    [s](): any;
}

export class ClassWithPrivateNamedProperties {
    [s]: any;
    static [s]: any;
}

export class ClassWithPrivateNamedMethods {
    [s]() {}
    static [s]() {}
}

export class ClassWithPrivateNamedAccessors {
    get [s](): any { 
AssertType(undefined, "undefined");
return undefined; 
}

    set [s](v: any) { }
    static get [s](): any { 
AssertType(undefined, "undefined");
return undefined; 
}

    static set [s](v: any) { }
}

