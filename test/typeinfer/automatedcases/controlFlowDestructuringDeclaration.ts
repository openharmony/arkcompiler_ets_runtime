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

// === tests/cases/conformance/controlFlow/controlFlowDestructuringDeclaration.ts ===
declare function AssertType(value:any, type:string):void;
function f1() {
    let x: string | number = 1;
AssertType(x, "union");
AssertType(1, "int");

    x;
AssertType(x, "number");

    let y: string | undefined = "";
AssertType(y, "union");
AssertType("", "string");

    y;
AssertType(y, "string");
}

function f2() {
    let [x]: [string | number] = [1];
AssertType(x, "union");
AssertType([1], "[number]");
AssertType(1, "int");

    x;
AssertType(x, "number");

    let [y]: [string | undefined] = [""];
AssertType(y, "union");
AssertType([""], "[string]");
AssertType("", "string");

    y;
AssertType(y, "string");

    let [z = ""]: [string | undefined] = [undefined];
AssertType(z, "string");
AssertType("", "string");
AssertType([undefined], "[undefined]");
AssertType(undefined, "undefined");

    z;
AssertType(z, "string");
}

function f3() {
    let [x]: (string | number)[] = [1];
AssertType(x, "union");
AssertType([1], "number[]");
AssertType(1, "int");

    x;
AssertType(x, "number");

    let [y]: (string | undefined)[] = [""];
AssertType(y, "union");
AssertType([""], "string[]");
AssertType("", "string");

    y;
AssertType(y, "string");

    let [z = ""]: (string | undefined)[] = [undefined];
AssertType(z, "string");
AssertType("", "string");
AssertType([undefined], "undefined[]");
AssertType(undefined, "undefined");

    z;
AssertType(z, "string");
}

function f4() {
    let { x }: { x: string | number } = { x: 1 };
AssertType(x, "union");
AssertType(x, "union");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

    x;
AssertType(x, "number");

    let { y }: { y: string | undefined } = { y: "" };
AssertType(y, "union");
AssertType(y, "union");
AssertType({ y: "" }, "{ y: string; }");
AssertType(y, "string");
AssertType("", "string");

    y;
AssertType(y, "string");

    let { z = "" }: { z: string | undefined } = { z: undefined };
AssertType(z, "string");
AssertType("", "string");
AssertType(z, "union");
AssertType({ z: undefined }, "{ z: undefined; }");
AssertType(z, "undefined");
AssertType(undefined, "undefined");

    z;
AssertType(z, "string");
}

function f5() {
    let { x }: { x?: string | number } = { x: 1 };
AssertType(x, "union");
AssertType(x, "union");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

    x;
AssertType(x, "number");

    let { y }: { y?: string | undefined } = { y: "" };
AssertType(y, "union");
AssertType(y, "union");
AssertType({ y: "" }, "{ y: string; }");
AssertType(y, "string");
AssertType("", "string");

    y;
AssertType(y, "string");

    let { z = "" }: { z?: string | undefined } = { z: undefined };
AssertType(z, "string");
AssertType("", "string");
AssertType(z, "union");
AssertType({ z: undefined }, "{ z: undefined; }");
AssertType(z, "undefined");
AssertType(undefined, "undefined");

    z;
AssertType(z, "string");
}

function f6() {
    let { x }: { x?: string | number } = {};
AssertType(x, "union");
AssertType(x, "union");
AssertType({}, "{}");

    x;
AssertType(x, "union");

    let { y }: { y?: string | undefined } = {};
AssertType(y, "union");
AssertType(y, "union");
AssertType({}, "{}");

    y;
AssertType(y, "union");

    let { z = "" }: { z?: string | undefined } = {};
AssertType(z, "string");
AssertType("", "string");
AssertType(z, "union");
AssertType({}, "{}");

    z;
AssertType(z, "string");
}

function f7() {
    let o: { [x: string]: number } = { x: 1 };
AssertType(o, "{ [string]: number; }");
AssertType(x, "string");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

    let { x }: { [x: string]: string | number } = o;
AssertType(x, "union");
AssertType(x, "string");
AssertType(o, "{ [string]: number; }");

    x;
AssertType(x, "number");
}


