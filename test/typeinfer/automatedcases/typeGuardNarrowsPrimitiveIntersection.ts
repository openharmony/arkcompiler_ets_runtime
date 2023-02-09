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

// === tests/cases/conformance/expressions/typeGuards/typeGuardNarrowsPrimitiveIntersection.ts ===
declare function AssertType(value:any, type:string):void;
type Tag = {__tag: any};
declare function isNonBlank(value: string) : value is (string & Tag);
declare function doThis(value: string & Tag): void;
declare function doThat(value: string) : void;
let value: string;
AssertType(value, "string");

if (isNonBlank(value)) {
    doThis(value);
AssertType(doThis(value), "void");
AssertType(doThis, "(string & Tag) => void");
AssertType(value, "string & Tag");

} else {
    doThat(value);
AssertType(doThat(value), "void");
AssertType(doThat, "(string) => void");
AssertType(value, "string");
}


const enum Tag2 {}
declare function isNonBlank2(value: string) : value is (string & Tag2);
declare function doThis2(value: string & Tag2): void;
declare function doThat2(value: string) : void;
if (isNonBlank2(value)) {
    doThis2(value);
AssertType(doThis2(value), "void");
AssertType(doThis2, "(never) => void");
AssertType(value, "never");

} else {
    doThat2(value);
AssertType(doThat2(value), "void");
AssertType(doThat2, "(string) => void");
AssertType(value, "string");
}


