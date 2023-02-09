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

// === tests/cases/conformance/expressions/arrayLiterals/arrayLiteralInference.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #31204

export enum AppType {
    HeaderDetail = 'HeaderDetail',
    HeaderMultiDetail = 'HeaderMultiDetail',
    AdvancedList = 'AdvancedList',
    Standard = 'Standard',
    Relationship = 'Relationship',
    Report = 'Report',
    Composite = 'Composite',
    ListOnly = 'ListOnly',
    ModuleSettings = 'ModuleSettings'
}

export enum AppStyle {
    Tree,
    TreeEntity,
    Standard,
    MiniApp,
    PivotTable
}

const appTypeStylesWithError: Map<AppType, Array<AppStyle>> = new Map([
AssertType(appTypeStylesWithError, "Map<AppType, AppStyle[]>");
AssertType(new Map([    [AppType.Standard, [AppStyle.Standard, AppStyle.MiniApp]],    [AppType.Relationship, [AppStyle.Standard, AppStyle.Tree, AppStyle.TreeEntity]],    [AppType.AdvancedList, [AppStyle.Standard, AppStyle.MiniApp]]]), "Map<union, union>");
AssertType(Map, "MapConstructor");
AssertType([    [AppType.Standard, [AppStyle.Standard, AppStyle.MiniApp]],    [AppType.Relationship, [AppStyle.Standard, AppStyle.Tree, AppStyle.TreeEntity]],    [AppType.AdvancedList, [AppStyle.Standard, AppStyle.MiniApp]]], "(union)[]");

    [AppType.Standard, [AppStyle.Standard, AppStyle.MiniApp]],
AssertType([AppType.Standard, [AppStyle.Standard, AppStyle.MiniApp]], "[AppType.Standard, (union)[]]");
AssertType(AppType.Standard, "AppType.Standard");
AssertType([AppStyle.Standard, AppStyle.MiniApp], "(union)[]");
AssertType(AppStyle.Standard, "AppStyle.Standard");
AssertType(AppStyle.MiniApp, "AppStyle.MiniApp");

    [AppType.Relationship, [AppStyle.Standard, AppStyle.Tree, AppStyle.TreeEntity]],
AssertType([AppType.Relationship, [AppStyle.Standard, AppStyle.Tree, AppStyle.TreeEntity]], "[AppType.Relationship, (union)[]]");
AssertType(AppType.Relationship, "AppType.Relationship");
AssertType([AppStyle.Standard, AppStyle.Tree, AppStyle.TreeEntity], "(union)[]");
AssertType(AppStyle.Standard, "AppStyle.Standard");
AssertType(AppStyle.Tree, "AppStyle.Tree");
AssertType(AppStyle.TreeEntity, "AppStyle.TreeEntity");

    [AppType.AdvancedList, [AppStyle.Standard, AppStyle.MiniApp]]
AssertType([AppType.AdvancedList, [AppStyle.Standard, AppStyle.MiniApp]], "[AppType.AdvancedList, (union)[]]");
AssertType(AppType.AdvancedList, "AppType.AdvancedList");
AssertType([AppStyle.Standard, AppStyle.MiniApp], "(union)[]");
AssertType(AppStyle.Standard, "AppStyle.Standard");
AssertType(AppStyle.MiniApp, "AppStyle.MiniApp");

]);

// Repro from #31204

declare function foo<T>(...args: T[]): T[];
let b1: { x: boolean }[] = foo({ x: true }, { x: false });
AssertType(b1, "{ x: boolean; }[]");
AssertType(x, "boolean");
AssertType(foo({ x: true }, { x: false }), "(union)[]");
AssertType(foo, "<T>(...T[]) => T[]");
AssertType({ x: true }, "{ x: true; }");
AssertType(x, "boolean");
AssertType(true, "boolean");
AssertType({ x: false }, "{ x: false; }");
AssertType(x, "boolean");
AssertType(false, "boolean");

let b2: boolean[][] = foo([true], [false]);
AssertType(b2, "boolean[][]");
AssertType(foo([true], [false]), "(union)[]");
AssertType(foo, "<T>(...T[]) => T[]");
AssertType([true], "true[]");
AssertType(true, "boolean");
AssertType([false], "false[]");
AssertType(false, "boolean");


