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

// === tests/cases/conformance/internalModules/codeGeneration/importStatements.ts ===
declare function AssertType(value:any, type:string):void;
module A {
    export class Point {
        constructor(public x: number, public y: number) { }
    }

    export let Origin = new Point(0, 0);
}

// no code gen expected
module B {
    import a = A; //Error generates 'let <Alias> = <EntityName>;'
}

// no code gen expected
module C {
    import a = A; //Error generates 'let <Alias> = <EntityName>;'
    let m: typeof a;
    let p: a.Point;
    let p = {x:0, y:0 };
}

// code gen expected
module D {
    import a = A;

    let p = new a.Point(1, 1);
}

module E {
    import a = A;
    export function xDist(x: a.Point) {
AssertType((a.Origin.x - x.x), "number");
AssertType(a.Origin.x - x.x, "number");
AssertType(a.Origin.x, "number");
AssertType(a.Origin, "a.Point");
AssertType(x.x, "number");
        return (a.Origin.x - x.x);
    }
}

