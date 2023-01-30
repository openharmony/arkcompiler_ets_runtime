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

// === tests/cases/compiler/genericIndexedAccessMethodIntersectionCanBeAccessed.ts ===
declare function AssertType(value:any, type:string):void;
type ExtendedService<T> = {
    [K in keyof T]: T[K] & {
        __$daemonMode?: string;
        __$action?: string;
    };
};

type Service<T> = {
    [K in keyof T]: T[K] & {id?: string};
};

export const createService = <T>(
AssertType(createService, "<T>(ExtendedService<T> & Service<T>) => void");
AssertType(<T>(    ServiceCtr: ExtendedService<T> & Service<T>) => {    Object.keys(ServiceCtr).forEach(key => {        const method = (ServiceCtr)[key as keyof T];        const {__$daemonMode, __$action, id} = method;    })}, "<T>(ExtendedService<T> & Service<T>) => void");

    ServiceCtr: ExtendedService<T> & Service<T>
AssertType(ServiceCtr, "ExtendedService<T> & Service<T>");

) => {
    Object.keys(ServiceCtr).forEach(key => {
AssertType(Object.keys(ServiceCtr).forEach(key => {        const method = (ServiceCtr)[key as keyof T];        const {__$daemonMode, __$action, id} = method;    }), "void");
AssertType(Object.keys(ServiceCtr).forEach, "((string, number, string[]) => void, ?any) => void");
AssertType(Object.keys(ServiceCtr), "string[]");
AssertType(Object.keys, "(object) => string[]");
AssertType(ServiceCtr, "ExtendedService<T> & Service<T>");
AssertType(key => {        const method = (ServiceCtr)[key as keyof T];        const {__$daemonMode, __$action, id} = method;    }, "(string) => void");
AssertType(key, "string");

        const method = (ServiceCtr)[key as keyof T];
AssertType(method, "(ExtendedService<T> & Service<T>)[keyof T]");
AssertType((ServiceCtr)[key as keyof T], "(ExtendedService<T> & Service<T>)[keyof T]");
AssertType((ServiceCtr), "ExtendedService<T> & Service<T>");
AssertType(ServiceCtr, "ExtendedService<T> & Service<T>");
AssertType(key as keyof T, "keyof T");
AssertType(key, "string");

        const {__$daemonMode, __$action, id} = method;
AssertType(__$daemonMode, "union");
AssertType(__$action, "union");
AssertType(id, "union");
AssertType(method, "(ExtendedService<T> & Service<T>)[keyof T]");

    })
}


