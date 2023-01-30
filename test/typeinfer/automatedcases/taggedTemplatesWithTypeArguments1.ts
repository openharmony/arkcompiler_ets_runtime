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

// === tests/cases/conformance/es6/templates/taggedTemplatesWithTypeArguments1.ts ===
declare function AssertType(value:any, type:string):void;
declare function f<T>(strs: TemplateStringsArray, ...callbacks: Array<(x: T) => any>): void;

interface Stuff {
    x: number;
    y: string;
    z: boolean;
}

export const a = f<Stuff> `
AssertType(a, "void");
AssertType(f<Stuff> `    hello    ${stuff => stuff.x}    brave    ${stuff => stuff.y}    world    ${stuff => stuff.z}`, "void");
AssertType(f, "<T>(TemplateStringsArray, ...((T) => any)[]) => void");
AssertType(`    hello    ${stuff => stuff.x}    brave    ${stuff => stuff.y}    world    ${stuff => stuff.z}`, "string");

    hello
    ${stuff => stuff.x
AssertType(stuff => stuff.x, "(Stuff) => number");

AssertType(stuff, "Stuff");

AssertType(stuff.x, "number");
}

    brave
    ${stuff => stuff.y
AssertType(stuff => stuff.y, "(Stuff) => string");

AssertType(stuff, "Stuff");

AssertType(stuff.y, "string");
}

    world
    ${stuff => stuff.z
AssertType(stuff => stuff.z, "(Stuff) => boolean");

AssertType(stuff, "Stuff");

AssertType(stuff.z, "boolean");
}

`;

declare function g<Input, T, U, V>(
    strs: TemplateStringsArray,
    t: (i: Input) => T, u: (i: Input) => U, v: (i: Input) => V): T | U | V;

export const b = g<Stuff, number, string, boolean> `
AssertType(b, "union");
AssertType(g<Stuff, number, string, boolean> `    hello    ${stuff => stuff.x}    brave    ${stuff => stuff.y}    world    ${stuff => stuff.z}`, "union");
AssertType(g, "<Input, T, U, V>(TemplateStringsArray, (Input) => T, (Input) => U, (Input) => V) => union");
AssertType(`    hello    ${stuff => stuff.x}    brave    ${stuff => stuff.y}    world    ${stuff => stuff.z}`, "string");

    hello
    ${stuff => stuff.x
AssertType(stuff => stuff.x, "(Stuff) => number");

AssertType(stuff, "Stuff");

AssertType(stuff.x, "number");
}

    brave
    ${stuff => stuff.y
AssertType(stuff => stuff.y, "(Stuff) => string");

AssertType(stuff, "Stuff");

AssertType(stuff.y, "string");
}

    world
    ${stuff => stuff.z
AssertType(stuff => stuff.z, "(Stuff) => boolean");

AssertType(stuff, "Stuff");

AssertType(stuff.z, "boolean");
}

`;

declare let obj: {
AssertType(obj, "{ prop: <T>(TemplateStringsArray, (T) => T) => { returnedObjProp: T; }; }");

    prop: <T>(strs: TemplateStringsArray, x: (input: T) => T) => {
AssertType(strs, "TemplateStringsArray");
AssertType(x, "(T) => T");
AssertType(input, "T");
AssertType(prop, "<T>(TemplateStringsArray, (T) => T) => { returnedObjProp: T; }");

AssertType(returnedObjProp, "T");
        returnedObjProp: T
    }
}

export let c = obj["prop"]<Stuff> `${(input) => ({ ...input })}`
AssertType(obj["prop"]<Stuff> `${(input) => ({ ...input })}`, "{ returnedObjProp: Stuff; }");
AssertType(obj["prop"], "<T>(TemplateStringsArray, (T) => T) => { returnedObjProp: T; }");
AssertType(obj, "{ prop: <T>(TemplateStringsArray, (T) => T) => { returnedObjProp: T; }; }");
AssertType("prop", "string");
AssertType(`${(input) => ({ ...input })}`, "string");
AssertType((input) => ({ ...input }), "(Stuff) => { x: number; y: string; z: boolean; }");
AssertType(input, "Stuff");
AssertType(({ ...input }), "{ x: number; y: string; z: boolean; }");
AssertType({ ...input }, "{ x: number; y: string; z: boolean; }");
AssertType(input, "Stuff");
AssertType(c, "{ returnedObjProp: Stuff; }");

c.
AssertType(c.returnedObjProp.x, "number");
returnedObjProp.x;

c.
AssertType(c.returnedObjProp.y, "string");
returnedObjProp.y;

c.
AssertType(c.returnedObjProp.z, "boolean");
returnedObjProp.z;

c = obj.prop<Stuff> `${(input) => ({ ...input })}`
AssertType(c, "{ returnedObjProp: Stuff; }");
AssertType(obj.prop<Stuff> `${(input) => ({ ...input })}`, "{ returnedObjProp: Stuff; }");
AssertType(obj.prop, "<T>(TemplateStringsArray, (T) => T) => { returnedObjProp: T; }");
AssertType(`${(input) => ({ ...input })}`, "string");
AssertType((input) => ({ ...input }), "(Stuff) => { x: number; y: string; z: boolean; }");
AssertType(input, "Stuff");
AssertType(({ ...input }), "{ x: number; y: string; z: boolean; }");
AssertType({ ...input }, "{ x: number; y: string; z: boolean; }");
AssertType(input, "Stuff");
AssertType(c = obj.prop<Stuff> `${(input) => ({ ...input })}`, "{ returnedObjProp: Stuff; }");

c.
AssertType(c.returnedObjProp.x, "number");
returnedObjProp.x;

c.
AssertType(c.returnedObjProp.y, "string");
returnedObjProp.y;

c.
AssertType(c.returnedObjProp.z, "boolean");
returnedObjProp.z;


