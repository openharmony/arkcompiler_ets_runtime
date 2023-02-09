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

// === tests/cases/compiler/es2017basicAsync.ts ===
declare function AssertType(value:any, type:string):void;
async (): Promise<void> => {
AssertType(async (): Promise<void> => {    await 0;}, "() => Promise<void>");

    await 0;
AssertType(await 0, "int");
AssertType(0, "int");
}

async function asyncFunc() {
    await 0;
AssertType(await 0, "int");
AssertType(0, "int");
}

const asyncArrowFunc = async (): Promise<void> => {
AssertType(asyncArrowFunc, "() => Promise<void>");
AssertType(async (): Promise<void> => {    await 0;}, "() => Promise<void>");

    await 0;
AssertType(await 0, "int");
AssertType(0, "int");
}

async function asyncIIFE() {
    await 0;
AssertType(await 0, "int");
AssertType(0, "int");

    await (async function(): Promise<void> {
AssertType(await (async function(): Promise<void> {        await 1;    })(), "void");
AssertType((async function(): Promise<void> {        await 1;    })(), "Promise<void>");
AssertType((async function(): Promise<void> {        await 1;    }), "() => Promise<void>");
AssertType(async function(): Promise<void> {        await 1;    }, "() => Promise<void>");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

    })();
    
    await (async function asyncNamedFunc(): Promise<void> {
AssertType(await (async function asyncNamedFunc(): Promise<void> {        await 1;    })(), "void");
AssertType((async function asyncNamedFunc(): Promise<void> {        await 1;    })(), "Promise<void>");
AssertType((async function asyncNamedFunc(): Promise<void> {        await 1;    }), "() => Promise<void>");
AssertType(async function asyncNamedFunc(): Promise<void> {        await 1;    }, "() => Promise<void>");
AssertType(asyncNamedFunc, "() => Promise<void>");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

    })();
    
    await (async (): Promise<void> => {
AssertType(await (async (): Promise<void> => {        await 1;    })(), "void");
AssertType((async (): Promise<void> => {        await 1;    })(), "Promise<void>");
AssertType((async (): Promise<void> => {        await 1;    }), "() => Promise<void>");
AssertType(async (): Promise<void> => {        await 1;    }, "() => Promise<void>");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

    })();
}

class AsyncClass {
    asyncPropFunc = async function(): Promise<void> {
        await 2;
AssertType(await 2, "int");
AssertType(2, "int");
    }
    
    asyncPropNamedFunc = async function namedFunc(): Promise<void> {
        await 2;
AssertType(await 2, "int");
AssertType(2, "int");
    }

    asyncPropArrowFunc = async (): Promise<void> => {
        await 2;
AssertType(await 2, "int");
AssertType(2, "int");
    }

    async asyncMethod(): Promise<void> {
        await 2;
AssertType(await 2, "int");
AssertType(2, "int");
    }
}


