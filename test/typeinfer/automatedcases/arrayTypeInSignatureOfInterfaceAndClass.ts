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

// === tests/cases/compiler/arrayTypeInSignatureOfInterfaceAndClass.ts ===
declare function AssertType(value:any, type:string):void;

declare module WinJS {
    class Promise<T> {
        then<U>(success?: (value: T) => Promise<U>, error?: (error: any) => Promise<U>, progress?: (progress: any) => void): Promise<U>;
    }
}
declare module Data {
    export interface IListItem<T> {
        itemIndex: number;
        key: any;
        data: T;
        group: any;
        isHeader: boolean;
        cached: boolean;
        isNonSourceData: boolean;
        preventAugmentation: boolean;
    }
    export interface IVirtualList<T> {
        //removeIndices: WinJS.Promise<IListItem<T>[]>;
        removeIndices(indices: number[], options?: any): WinJS.Promise<IListItem<T>[]>;
    }
    export class VirtualList<T> implements IVirtualList<T> {
        //removeIndices: WinJS.Promise<IListItem<T>[]>;
        public removeIndices(indices: number[], options?: any): WinJS.Promise<IListItem<T>[]>;
    }
}

