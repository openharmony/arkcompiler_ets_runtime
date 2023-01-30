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

// === tests/cases/compiler/declFileAccessors_0.ts ===
declare function AssertType(value:any, type:string):void;
/** This is comment for c1*/
export class c1 {
    /** getter property*/
    public get p3() {
AssertType(10, "int");
        return 10;
    }
    /** setter property*/
    public set p3(/** this is value*/value: number) {
    }
    /** private getter property*/
    private get pp3() {
AssertType(10, "int");
        return 10;
    }
    /** private setter property*/
    private set pp3(/** this is value*/value: number) {
    }
    /** static getter property*/
    static get s3() {
AssertType(10, "int");
        return 10;
    }
    /** setter property*/
    static set s3( /** this is value*/value: number) {
    }
    public get nc_p3() {
AssertType(10, "int");
        return 10;
    }
    public set nc_p3(value: number) {
    }
    private get nc_pp3() {
AssertType(10, "int");
        return 10;
    }
    private set nc_pp3(value: number) {
    }
    static get nc_s3() {
AssertType("", "string");
        return "";
    }
    static set nc_s3(value: string) {
    }

    // Only getter property
    public get onlyGetter() {
AssertType(10, "int");
        return 10;
    }

    // Only setter property
    public set onlySetter(value: number) {
    }
}


