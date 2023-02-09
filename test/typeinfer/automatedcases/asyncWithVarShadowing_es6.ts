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

// === tests/cases/conformance/async/es6/asyncWithVarShadowing_es6.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/Microsoft/TypeScript/issues/20461
declare const y: any;
AssertType(y, "any");

async function fn1(x) {
    let x;
AssertType(x, "any");
}

async function fn2(x) {
    let x, z;
AssertType(x, "any");
AssertType(z, "any");
}

async function fn3(x) {
    let z;
AssertType(z, "any");
}

async function fn4(x) {
    let x = y;
AssertType(x, "any");
AssertType(y, "any");
}

async function fn5(x) {
    let { x } = y;
AssertType(x, "any");
AssertType(y, "any");
}

async function fn6(x) {
    let { x, z } = y;
AssertType(x, "any");
AssertType(z, "any");
AssertType(y, "any");
}

async function fn7(x) {
    let { x = y } = y;
AssertType(x, "any");
AssertType(y, "any");
AssertType(y, "any");
}

async function fn8(x) {
    let { z: x } = y;
AssertType(z, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function fn9(x) {
    let { z: { x } } = y;
AssertType(z, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function fn10(x) {
    let { z: { x } = y } = y;
AssertType(z, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(y, "any");
}

async function fn11(x) {
    let { ...x } = y;
AssertType(x, "any");
AssertType(y, "any");
}

async function fn12(x) {
    let [x] = y;
AssertType(x, "any");
AssertType(y, "any");
}

async function fn13(x) {
    let [x = y] = y;
AssertType(x, "any");
AssertType(y, "any");
AssertType(y, "any");
}

async function fn14(x) {
    let [, x] = y;
AssertType(, "undefined");
AssertType(x, "any");
AssertType(y, "any");
}

async function fn15(x) {
    let [...x] = y;
AssertType(x, "any");
AssertType(y, "any");
}

async function fn16(x) {
    let [[x]] = y;
AssertType(x, "any");
AssertType(y, "any");
}

async function fn17(x) {
    let [[x] = y] = y;
AssertType(x, "any");
AssertType(y, "any");
AssertType(y, "any");
}

async function fn18({ x }) {
    let x;
AssertType(x, "any");
}

async function fn19([x]) {
    let x;
AssertType(x, "any");
}

async function fn20(x) {
    {
        let x;
AssertType(x, "any");
    }
}

async function fn21(x) {
    if (y) {
AssertType(y, "any");

        let x;
AssertType(x, "any");
    }
}

async function fn22(x) {
    if (y) {
AssertType(y, "any");
    }
    else {
        let x;
AssertType(x, "any");
    }
}

async function fn23(x) {
    try {
        let x;
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");
    }
}

async function fn24(x) {
    try {

    }
    catch (e) {
AssertType(e, "any");

        let x;
AssertType(x, "any");
    }
}

async function fn25(x) {
    try {

    }
    catch (x) {
AssertType(x, "any");

        let x;
AssertType(x, "any");
    }
}

async function fn26(x) {
    try {

    }
    catch ({ x }) {
AssertType(x, "any");

        let x;
AssertType(x, "any");
    }
}

async function fn27(x) {
    try {
    }
    finally {
        let x;
AssertType(x, "any");
    }
}

async function fn28(x) {
    while (y) {
AssertType(y, "any");

        let x;
AssertType(x, "any");
    }
}

async function fn29(x) {
    do {
        let x;
AssertType(x, "any");
    }
    while (y);
AssertType(y, "any");
}

async function fn30(x) {
    for (let x = y;;) {
AssertType(x, "any");
AssertType(y, "any");

    }
}

async function fn31(x) {
    for (let { x } = y;;) {
AssertType(x, "any");
AssertType(y, "any");
    }
}

async function fn32(x) {
    for (;;) {
        let x;
AssertType(x, "any");
    }
}

async function fn33(x: string) {
    for (let x in y) {
AssertType(x, "string");
AssertType(y, "any");
    }
}

async function fn34(x) {
    for (let z in y) {
AssertType(z, "string");
AssertType(y, "any");

        let x;
AssertType(x, "any");
    }
}

async function fn35(x) {
    for (let x of y) {
AssertType(x, "any");
AssertType(y, "any");
    }
}

async function fn36(x) {
    for (let { x } of y) {
AssertType(x, "any");
AssertType(y, "any");
    }
}

async function fn37(x) {
    for (let z of y) {
AssertType(z, "any");
AssertType(y, "any");

        let x;
AssertType(x, "any");
    }
}

async function fn38(x) {
    switch (y) {
AssertType(y, "any");

        case y:
AssertType(y, "any");

            let x;
AssertType(x, "any");
    }
}

async function fn39(x) {
    foo: {
AssertType(foo, "any");

        let x;
AssertType(x, "any");

        break foo;
AssertType(foo, "any");
    }
}

async function fn40(x) {
    try {

    }
    catch {
        let x;
AssertType(x, "any");
    }
}


