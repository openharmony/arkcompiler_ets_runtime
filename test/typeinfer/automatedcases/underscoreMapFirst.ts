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

// === tests/cases/compiler/underscoreMapFirst.ts ===
declare function AssertType(value:any, type:string):void;
declare module _ {
    interface Collection<T> { }
    interface List<T> extends Collection<T> {
        [index: number]: T;
        length: number;
    }

    interface ListIterator<T, TResult> {
        (value: T, index: number, list: T[]): TResult;
    }

    interface Dictionary<T> extends Collection<T> {
        [index: string]: T;
    }
    export function pluck<T extends {}>(
        list: Collection<T>,
        propertyName: string): any[];

    export function map<T, TResult>(
        list: List<T>,
        iterator: ListIterator<T, TResult>,
        context?: any): TResult[];

    export function first<T>(array: List<T>): T;
}

declare class View {
    model: any;
}

interface IData {
    series: ISeries[];
}

interface ISeries {
    items: any[];
    key: string;
}

class MyView extends View {
    public getDataSeries(): ISeries[] {
        let data: IData[] = this.model.get("data");
AssertType(data, "IData[]");
AssertType(this.model.get("data"), "any");
AssertType(this.model.get, "any");
AssertType(this.model, "any");
AssertType(this, "this");
AssertType("data", "string");

        let allSeries: ISeries[][] = _.pluck(data, "series");
AssertType(allSeries, "ISeries[][]");
AssertType(_.pluck(data, "series"), "any[]");
AssertType(_.pluck, "<T extends {}>(_.Collection<T>, string) => any[]");
AssertType(data, "IData[]");
AssertType("series", "string");

AssertType(_.map(allSeries, _.first), "ISeries[]");
AssertType(_.map, "<T, TResult>(_.List<T>, _.ListIterator<T, TResult>, ?any) => TResult[]");
AssertType(allSeries, "ISeries[][]");
AssertType(_.first, "<T>(_.List<T>) => T");
        return _.map(allSeries, _.first);
    }
}


