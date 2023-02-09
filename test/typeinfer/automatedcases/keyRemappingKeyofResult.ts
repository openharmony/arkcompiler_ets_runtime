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

// === tests/cases/compiler/keyRemappingKeyofResult.ts ===
declare function AssertType(value:any, type:string):void;
const sym = Symbol("")
AssertType(sym, "unique symbol");
AssertType(Symbol(""), "unique symbol");
AssertType(Symbol, "SymbolConstructor");
AssertType("", "string");

type Orig = { [k: string]: any, str: any, [sym]: any }

type Okay = Exclude<keyof Orig, never>
// type Okay = string | number | typeof sym

type Remapped = { [K in keyof Orig as {} extends Record<K, any> ? never : K]: any }
/* type Remapped = {
    str: any;
    [sym]: any;
} */
// no string index signature, right?

type Oops = Exclude<keyof Remapped, never>
declare let x: Oops;
AssertType(x, "Oops");

x = sym;
AssertType(x = sym, "unique symbol");
AssertType(x, "Oops");
AssertType(sym, "unique symbol");

x = "str";
AssertType(x = "str", "string");
AssertType(x, "Oops");
AssertType("str", "string");

// type Oops = typeof sym <-- what happened to "str"?

// equivalently, with an unresolved generic (no `exclude` shenanigans, since conditions won't execute):
function f<T>() {
    type Orig = { [k: string]: any, str: any, [sym]: any } & T;
AssertType(Orig, "{ [string]: any; str: any; [sym]: any; } & T");
AssertType(k, "string");
AssertType(str, "any");
AssertType([sym], "any");
AssertType(sym, "unique symbol");
    
    type Okay = keyof Orig;
AssertType(Okay, "union");

    let a: Okay;
AssertType(a, "union");

    a = "str";
AssertType(a = "str", "string");
AssertType(a, "union");
AssertType("str", "string");

    a = sym;
AssertType(a = sym, "unique symbol");
AssertType(a, "union");
AssertType(sym, "unique symbol");

    a = "whatever";
AssertType(a = "whatever", "string");
AssertType(a, "union");
AssertType("whatever", "string");

    // type Okay = string | number | typeof sym
    
    type Remapped = { [K in keyof Orig as {} extends Record<K, any> ? never : K]: any 
AssertType(Remapped, "{ [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as {} extends Record<K, any> ? never : K]: any; }");
}

    /* type Remapped = {
        str: any;
        [sym]: any;
    } */
    // no string index signature, right?
    
    type Oops = keyof Remapped;
AssertType(Oops, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as {} extends Record<K, any> ? never : K]: any; }");

    let x: Oops;
AssertType(x, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as {} extends Record<K, any> ? never : K]: any; }");

    x = sym;
AssertType(x = sym, "unique symbol");
AssertType(x, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as {} extends Record<K, any> ? never : K]: any; }");
AssertType(sym, "unique symbol");

    x = "str";
AssertType(x = "str", "string");
AssertType(x, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as {} extends Record<K, any> ? never : K]: any; }");
AssertType("str", "string");
}

// and another generic case with a _distributive_ mapping, to trigger a different branch in `getIndexType`
function g<T>() {
    type Orig = { [k: string]: any, str: any, [sym]: any } & T;
AssertType(Orig, "{ [string]: any; str: any; [sym]: any; } & T");
AssertType(k, "string");
AssertType(str, "any");
AssertType([sym], "any");
AssertType(sym, "unique symbol");
    
    type Okay = keyof Orig;
AssertType(Okay, "union");

    let a: Okay;
AssertType(a, "union");

    a = "str";
AssertType(a = "str", "string");
AssertType(a, "union");
AssertType("str", "string");

    a = sym;
AssertType(a = sym, "unique symbol");
AssertType(a, "union");
AssertType(sym, "unique symbol");

    a = "whatever";
AssertType(a = "whatever", "string");
AssertType(a, "union");
AssertType("whatever", "string");

    // type Okay = string | number | typeof sym

    type NonIndex<T extends PropertyKey> = {} extends Record<T, any> ? never : T;
AssertType(NonIndex, "{} extends Record<T, any> ? never : T");

    type DistributiveNonIndex<T extends PropertyKey> = T extends unknown ? NonIndex<T> : never;
AssertType(DistributiveNonIndex, "T extends unknown ? {} extends Record<T, any> ? never : T : never");
    
    type Remapped = { [K in keyof Orig as DistributiveNonIndex<K>]: any 
AssertType(Remapped, "{ [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as K extends unknown ? {} extends Record<K, any> ? never : K : never]: any; }");
}

    /* type Remapped = {
        str: any;
        [sym]: any;
    } */
    // no string index signature, right?
    
    type Oops = keyof Remapped;
AssertType(Oops, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as K extends unknown ? {} extends Record<K, any> ? never : K : never]: any; }");

    let x: Oops;
AssertType(x, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as K extends unknown ? {} extends Record<K, any> ? never : K : never]: any; }");

    x = sym;
AssertType(x = sym, "unique symbol");
AssertType(x, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as K extends unknown ? {} extends Record<K, any> ? never : K : never]: any; }");
AssertType(sym, "unique symbol");

    x = "str";
AssertType(x = "str", "string");
AssertType(x, "keyof { [K in keyof ({ [string]: any; str: any; [sym]: any; } & T) as K extends unknown ? {} extends Record<K, any> ? never : K : never]: any; }");
AssertType("str", "string");
}

export {};

