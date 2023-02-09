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

// === tests/cases/compiler/2dArrays.ts ===
declare function AssertType(value:any, type:string):void;
class Cell {
}

class Ship {
    isSunk: boolean;
}

class Board {
    ships: Ship[];
    cells: Cell[];

    private allShipsSunk() {
AssertType(this.ships.every(function (val) { return val.isSunk; }), "boolean");
AssertType(this.ships.every, "{ <S extends Ship>((Ship, number, Ship[]) => value is S, ?any): this is S[]; ((Ship, number, Ship[]) => unknown, ?any): boolean; }");
AssertType(this.ships, "Ship[]");
AssertType(this, "this");
AssertType(function (val) { return val.isSunk; }, "(Ship) => boolean");
AssertType(val, "Ship");
AssertType(val.isSunk, "boolean");
        return this.ships.every(function (val) { return val.isSunk; });
    }    
}

