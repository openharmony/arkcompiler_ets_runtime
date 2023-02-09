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

// === tests/cases/compiler/narrowCommaOperatorNestedWithinLHS.ts ===
declare function AssertType(value:any, type:string):void;
const otherValue = () => true;
AssertType(otherValue, "() => boolean");
AssertType(() => true, "() => boolean");
AssertType(true, "boolean");

const value: { inner: number | string } = null as any;
AssertType(value, "{ inner: union; }");
AssertType(inner, "union");
AssertType(null as any, "any");
AssertType(null, "null");

function isNumber(obj: any): obj is number {
AssertType(true, "boolean");
    return true; // method implementation irrelevant
}

if (typeof (otherValue(), value).inner === 'number') {
    const a = value.inner; // number
AssertType(a, "number");
AssertType(value.inner, "number");

    const b: number = (otherValue(), value).inner; // string | number , but should be number
AssertType(b, "number");
AssertType((otherValue(), value).inner, "number");
AssertType((otherValue(), value), "{ inner: union; }");
AssertType(otherValue(), value, "{ inner: union; }");
AssertType(otherValue(), "boolean");
AssertType(otherValue, "() => boolean");
AssertType(value, "{ inner: union; }");
}

if (isNumber((otherValue(), value).inner)) {
    const a = value.inner; // number
AssertType(a, "number");
AssertType(value.inner, "number");

    const b: number = (otherValue(), value).inner; // string | number , but should be number
AssertType(b, "number");
AssertType((otherValue(), value).inner, "number");
AssertType((otherValue(), value), "{ inner: union; }");
AssertType(otherValue(), value, "{ inner: union; }");
AssertType(otherValue(), "boolean");
AssertType(otherValue, "() => boolean");
AssertType(value, "{ inner: union; }");
}

