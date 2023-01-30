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

// === tests/cases/compiler/instanceofTypeAliasToGenericClass.ts ===
declare function AssertType(value:any, type:string):void;
declare class TableClass<S = any> {
    _field: S;
}

export type Table = TableClass;

function fn<T extends Table>(o: T) {
AssertType(o instanceof TableClass, "boolean");
AssertType(o, "T");
AssertType(TableClass, "typeof TableClass");
    return o instanceof TableClass;
}

function fn2<T extends TableClass>(o: T) {
AssertType(o instanceof TableClass, "boolean");
AssertType(o, "T");
AssertType(TableClass, "typeof TableClass");
    return o instanceof TableClass;
}

declare const o: Table;
AssertType(o, "Table");

o instanceof TableClass;
AssertType(o instanceof TableClass, "boolean");
AssertType(o, "Table");
AssertType(TableClass, "typeof TableClass");


