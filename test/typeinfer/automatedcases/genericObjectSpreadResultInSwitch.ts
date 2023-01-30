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

// === tests/cases/compiler/genericObjectSpreadResultInSwitch.ts ===
declare function AssertType(value:any, type:string):void;
type Params = {
    foo: string;
} & ({ tag: 'a'; type: number } | { tag: 'b'; type: string });

const getType = <P extends Params>(params: P) => {
AssertType(getType, "<P extends Params>(P) => Omit<P, "foo">");
AssertType(params, "P");
AssertType(<P extends Params>(params: P) => {    const {        // Omit        foo,        ...rest    } = params;    return rest;}, "<P extends Params>(P) => Omit<P, "foo">");

    const {
        // Omit
        foo,
AssertType(foo, "string");

        ...rest
AssertType(rest, "Omit<P, "foo">");

    } = params;
AssertType(params, "Params");

AssertType(rest, "Omit<P, "foo">");
    return rest;

};

declare const params: Params;
AssertType(params, "Params");

switch (params.tag) {
    case 'a': {
        // TS 4.2: number
        // TS 4.3: string | number
        const result = getType(params).type;
AssertType(result, "number");
AssertType(getType(params).type, "number");
AssertType(getType(params), "Omit<{ foo: string; } & { tag: "a"; type: number; }, "foo">");
AssertType(getType, "<P extends Params>(P) => Omit<P, "foo">");
AssertType(params, "{ foo: string; } & { tag: "a"; type: number; }");

        break;
    }
    case 'b': {
        // TS 4.2: string
        // TS 4.3: string | number
        const result = getType(params).type;
AssertType(result, "string");
AssertType(getType(params).type, "string");
AssertType(getType(params), "Omit<{ foo: string; } & { tag: "b"; type: string; }, "foo">");
AssertType(getType, "<P extends Params>(P) => Omit<P, "foo">");
AssertType(params, "{ foo: string; } & { tag: "b"; type: string; }");

        break;
    }
}

