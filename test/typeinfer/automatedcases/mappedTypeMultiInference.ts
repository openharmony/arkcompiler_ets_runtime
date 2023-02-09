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

// === tests/cases/compiler/mappedTypeMultiInference.ts ===
declare function AssertType(value:any, type:string):void;
interface Style {
    flashy: any;
}

declare function mergeStyleSets<K extends string>(
    ...cssSets: { [P in K]?: Style }[]): { [P in K]: Style };

// Expected:
//   let x: {
//       a: Style;
//       b: Style;
//   }
let x = mergeStyleSets(
AssertType(x, "{ a: Style; b: Style; }");
AssertType(mergeStyleSets(    {},    {        a: { flashy: true },    },    {        b: { flashy: true },    },), "{ a: Style; b: Style; }");
AssertType(mergeStyleSets, "<K extends string>(...{ [P in K]?: Style; }[]) => { [P in K]: Style; }");

    {},
AssertType({}, "{}");
    {
AssertType({        a: { flashy: true },    }, "{ a: { flashy: boolean; }; }");

        a: { flashy: true },
AssertType(a, "{ flashy: boolean; }");
AssertType({ flashy: true }, "{ flashy: boolean; }");
AssertType(flashy, "boolean");
AssertType(true, "boolean");

    },
    {
AssertType({        b: { flashy: true },    }, "{ b: { flashy: boolean; }; }");

        b: { flashy: true },
AssertType(b, "{ flashy: boolean; }");
AssertType({ flashy: true }, "{ flashy: boolean; }");
AssertType(flashy, "boolean");
AssertType(true, "boolean");

    },
)

