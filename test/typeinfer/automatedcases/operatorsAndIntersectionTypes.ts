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

// === tests/cases/conformance/types/intersection/operatorsAndIntersectionTypes.ts ===
declare function AssertType(value:any, type:string):void;
type Guid = string & { $Guid };          // Tagged string type
type SerialNo = number & { $SerialNo };  // Tagged number type

function createGuid() {
AssertType("21EC2020-3AEA-4069-A2DD-08002B30309D" as Guid, "Guid");
AssertType("21EC2020-3AEA-4069-A2DD-08002B30309D", "string");
    return "21EC2020-3AEA-4069-A2DD-08002B30309D" as Guid;
}

function createSerialNo() {
AssertType(12345 as SerialNo, "SerialNo");
AssertType(12345, "int");
    return 12345 as SerialNo;
}

let map1: { [x: string]: number } = {};
AssertType(map1, "{ [string]: number; }");
AssertType(x, "string");
AssertType({}, "{}");

let guid = createGuid();
AssertType(guid, "Guid");
AssertType(createGuid(), "Guid");
AssertType(createGuid, "() => Guid");

map1[guid] = 123;  // Can with tagged string
AssertType(map1[guid] = 123, "int");
AssertType(map1[guid], "number");
AssertType(map1, "{ [string]: number; }");
AssertType(guid, "Guid");
AssertType(123, "int");

let map2: { [x: number]: string } = {};
AssertType(map2, "{ [number]: string; }");
AssertType(x, "number");
AssertType({}, "{}");

let serialNo = createSerialNo();
AssertType(serialNo, "SerialNo");
AssertType(createSerialNo(), "SerialNo");
AssertType(createSerialNo, "() => SerialNo");

map2[serialNo] = "hello";  // Can index with tagged number
AssertType(map2[serialNo] = "hello", "string");
AssertType(map2[serialNo], "string");
AssertType(map2, "{ [number]: string; }");
AssertType(serialNo, "SerialNo");
AssertType("hello", "string");

const s1 = "{" + guid + "}";
AssertType(s1, "string");
AssertType("{" + guid + "}", "string");
AssertType("{" + guid, "string");
AssertType("{", "string");
AssertType(guid, "Guid");
AssertType("}", "string");

const s2 = guid.toLowerCase();
AssertType(s2, "string");
AssertType(guid.toLowerCase(), "string");
AssertType(guid.toLowerCase, "() => string");

const s3 = guid + guid;
AssertType(s3, "string");
AssertType(guid + guid, "string");
AssertType(guid, "Guid");
AssertType(guid, "Guid");

const s4 = guid + serialNo;
AssertType(s4, "string");
AssertType(guid + serialNo, "string");
AssertType(guid, "Guid");
AssertType(serialNo, "SerialNo");

const s5 = serialNo.toPrecision(0);
AssertType(s5, "string");
AssertType(serialNo.toPrecision(0), "string");
AssertType(serialNo.toPrecision, "(?number) => string");
AssertType(0, "int");

const n1 = serialNo * 3;
AssertType(n1, "number");
AssertType(serialNo * 3, "number");
AssertType(serialNo, "SerialNo");
AssertType(3, "int");

const n2 = serialNo + serialNo;
AssertType(n2, "number");
AssertType(serialNo + serialNo, "number");
AssertType(serialNo, "SerialNo");
AssertType(serialNo, "SerialNo");

const b1 = guid === "";
AssertType(b1, "boolean");
AssertType(guid === "", "boolean");
AssertType(guid, "Guid");
AssertType("", "string");

const b2 = guid === guid;
AssertType(b2, "boolean");
AssertType(guid === guid, "boolean");
AssertType(guid, "Guid");
AssertType(guid, "Guid");

const b3 = serialNo === 0;
AssertType(b3, "boolean");
AssertType(serialNo === 0, "boolean");
AssertType(serialNo, "SerialNo");
AssertType(0, "int");

const b4 = serialNo === serialNo;
AssertType(b4, "boolean");
AssertType(serialNo === serialNo, "boolean");
AssertType(serialNo, "SerialNo");
AssertType(serialNo, "SerialNo");


