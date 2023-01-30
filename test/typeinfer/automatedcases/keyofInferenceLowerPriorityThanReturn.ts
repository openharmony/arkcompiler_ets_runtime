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

// === tests/cases/conformance/types/typeRelationships/typeInference/keyofInferenceLowerPriorityThanReturn.ts ===
declare function AssertType(value:any, type:string):void;
// #22736
declare class Write {
    protected dummy: Write;
}

declare class Col<s, a> {
    protected dummy: [Col<s, a>, s, a];
}

declare class Table<Req, Def> {
    protected dummy: [Table<Req, Def>, Req, Def];
}

type MakeTable<T1 extends object, T2 extends object> = {
    [P in keyof T1]: Col<Write, T1[P]>;
} & {
        [P in keyof T2]: Col<Write, T2[P]>;
    };

declare class ConflictTarget<Cols> {
    public static tableColumns<Cols>(cols: (keyof Cols)[]): ConflictTarget<Cols>;
    protected dummy: [ConflictTarget<Cols>, Cols];
}



const bookTable: Table<BookReq, BookDef> = null as any
AssertType(bookTable, "Table<BookReq, BookDef>");
AssertType(null as any, "any");
AssertType(null, "null");

interface BookReq {
    readonly title: string;
    readonly serial: number;
}

interface BookDef {
    readonly author: string;
    readonly numPages: number | null;
}


function insertOnConflictDoNothing<Req extends object, Def extends object>(_table: Table<Req, Def>, _conflictTarget: ConflictTarget<Req & Def>): boolean {
    throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
}

function f() {
    insertOnConflictDoNothing(bookTable, ConflictTarget.tableColumns(["serial"]));  // <-- No error here; should use the type inferred for the 
AssertType(insertOnConflictDoNothing(bookTable, ConflictTarget.tableColumns(["serial"])), "boolean");
AssertType(insertOnConflictDoNothing, "<Req extends object, Def extends object>(Table<Req, Def>, ConflictTarget<Req & Def>) => boolean");
AssertType(bookTable, "Table<BookReq, BookDef>");
AssertType(ConflictTarget.tableColumns(["serial"]), "ConflictTarget<BookReq & BookDef>");
AssertType(ConflictTarget.tableColumns, "<Cols>((keyof Cols)[]) => ConflictTarget<Cols>");
AssertType(["serial"], ""serial"[]");
AssertType("serial", "string");
return type of `tableColumns`
}


