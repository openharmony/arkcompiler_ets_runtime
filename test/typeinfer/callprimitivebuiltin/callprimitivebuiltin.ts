/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 */

declare function AssertType(value: any, type: string): void;
{
    let str = "abcd"
    AssertType(str, "string");
    let strEle1 = str.indexOf("a");
    AssertType(strEle1, "number");
    let strEle2 = str.toUpperCase();
    AssertType(strEle2, "string");

    let num = 1234;
    AssertType(num, "int");
    let numEle1 = num.toString();
    AssertType(numEle1, "string");
    let numEle2 = num.valueOf();
    AssertType(numEle2, "number");

    let boolValue : boolean = true;
    AssertType(boolValue, "boolean");
    let boolEle = boolValue.valueOf();
    AssertType(boolEle, "boolean");
}