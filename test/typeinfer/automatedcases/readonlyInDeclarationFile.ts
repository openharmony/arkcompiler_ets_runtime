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

// === tests/cases/compiler/readonlyInDeclarationFile.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo {
    readonly x: number;
    readonly [x: string]: Object;
}

class C {
    readonly [x: string]: Object;
    private readonly a1: number;
    protected readonly a2: number;
    public readonly a3: number;
    private get b1() { 
AssertType(1, "int");
return 1 
}

    protected get b2() { 
AssertType(1, "int");
return 1 
}

    public get b3() { 
AssertType(1, "int");
return 1 
}

    private get c1() { 
AssertType(1, "int");
return 1 
}

    private set c1(value) { }
    protected get c2() { 
AssertType(1, "int");
return 1 
}

    protected set c2(value) { }
    public get c3() { 
AssertType(1, "int");
return 1 
}

    public set c3(value) { }
    private static readonly s1: number;
    protected static readonly s2: number;
    public static readonly s3: number;
    private static get t1() { 
AssertType(1, "int");
return 1 
}

    protected static get t2() { 
AssertType(1, "int");
return 1 
}

    public static get t3() { 
AssertType(1, "int");
return 1 
}

    private static get u1() { 
AssertType(1, "int");
return 1 
}

    private static set u1(value) { }
    protected static get u2() { 
AssertType(1, "int");
return 1 
}

    protected static set u2(value) { }
    public static get u3() { 
AssertType(1, "int");
return 1 
}

    public static set u3(value) { }
}

let z: {
AssertType(z, "{ readonly [string]: Object; readonly a: string; }");

    readonly a: string;
AssertType(a, "string");

    readonly [x: string]: Object;
AssertType(x, "string");
}

function f() {
AssertType({        get x() { return 1; },        get y() { return 1; },        set y(value) { }    }, "{ readonly x: number; y: number; }");
    return {

        get x() { 
AssertType(x, "number");
AssertType(1, "int");
return 1; },

        get y() { 
AssertType(y, "number");
AssertType(1, "int");
return 1; },

        set y(value) { 
AssertType(y, "number");

AssertType(value, "number");
}
    }
}

function g() {
    let x: {
AssertType(x, "{ readonly [string]: Object; readonly a: string; }");

        readonly a: string;
AssertType(a, "string");

        readonly [x: string]: Object;
AssertType(x, "string");
    }
AssertType(x, "{ readonly [string]: Object; readonly a: string; }");
    return x;
}

