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

// === tests/cases/compiler/spreadOfObjectLiteralAssignableToIndexSignature.ts ===
declare function AssertType(value:any, type:string):void;
const foo: Record<never, never> = {} // OK
AssertType(foo, "Record<never, never>");
AssertType({}, "{}");

interface RecordOfRecords extends Record<keyof any, RecordOfRecords> {}
const recordOfRecords: RecordOfRecords = {
AssertType(recordOfRecords, "RecordOfRecords");

AssertType({}, "{}");
}

recordOfRecords.propA = {...(foo !== undefined ? {foo} : {})} // OK
AssertType(recordOfRecords.propA = {...(foo !== undefined ? {foo} : {})}, "{ foo?: union; }");
AssertType(recordOfRecords.propA, "RecordOfRecords");
AssertType({...(foo !== undefined ? {foo} : {})}, "{ foo?: union; }");
AssertType((foo !== undefined ? {foo} : {}), "union");
AssertType(foo !== undefined ? {foo} : {}, "union");
AssertType(foo !== undefined, "boolean");
AssertType(foo, "Record<never, never>");
AssertType(undefined, "undefined");
AssertType({foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");
AssertType({}, "{}");

recordOfRecords.propB = {...(foo && {foo})} // OK
AssertType(recordOfRecords.propB = {...(foo && {foo})}, "{ foo: Record<never, never>; }");
AssertType(recordOfRecords.propB, "RecordOfRecords");
AssertType({...(foo && {foo})}, "{ foo: Record<never, never>; }");
AssertType((foo && {foo}), "{ foo: Record<never, never>; }");
AssertType(foo && {foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");
AssertType({foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");

recordOfRecords.propC = {...(foo !== undefined && {foo})} // error'd in 3.7 beta, should be OK
AssertType(recordOfRecords.propC = {...(foo !== undefined && {foo})}, "{ foo?: union; }");
AssertType(recordOfRecords.propC, "RecordOfRecords");
AssertType({...(foo !== undefined && {foo})}, "{ foo?: union; }");
AssertType((foo !== undefined && {foo}), "union");
AssertType(foo !== undefined && {foo}, "union");
AssertType(foo !== undefined, "boolean");
AssertType(foo, "Record<never, never>");
AssertType(undefined, "undefined");
AssertType({foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");

interface RecordOfRecordsOrEmpty extends Record<keyof any, RecordOfRecordsOrEmpty | {}> {}
const recordsOfRecordsOrEmpty: RecordOfRecordsOrEmpty = {
AssertType(recordsOfRecordsOrEmpty, "RecordOfRecordsOrEmpty");

AssertType({}, "{}");
}

recordsOfRecordsOrEmpty.propA = {...(foo !== undefined ? {foo} : {})} // OK
AssertType(recordsOfRecordsOrEmpty.propA = {...(foo !== undefined ? {foo} : {})}, "{ foo?: union; }");
AssertType(recordsOfRecordsOrEmpty.propA, "union");
AssertType({...(foo !== undefined ? {foo} : {})}, "{ foo?: union; }");
AssertType((foo !== undefined ? {foo} : {}), "union");
AssertType(foo !== undefined ? {foo} : {}, "union");
AssertType(foo !== undefined, "boolean");
AssertType(foo, "Record<never, never>");
AssertType(undefined, "undefined");
AssertType({foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");
AssertType({}, "{}");

recordsOfRecordsOrEmpty.propB = {...(foo && {foo})} // OK
AssertType(recordsOfRecordsOrEmpty.propB = {...(foo && {foo})}, "{ foo: Record<never, never>; }");
AssertType(recordsOfRecordsOrEmpty.propB, "union");
AssertType({...(foo && {foo})}, "{ foo: Record<never, never>; }");
AssertType((foo && {foo}), "{ foo: Record<never, never>; }");
AssertType(foo && {foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");
AssertType({foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");

recordsOfRecordsOrEmpty.propC = {...(foo !== undefined && {foo})} // OK
AssertType(recordsOfRecordsOrEmpty.propC = {...(foo !== undefined && {foo})}, "{ foo?: union; }");
AssertType(recordsOfRecordsOrEmpty.propC, "union");
AssertType({...(foo !== undefined && {foo})}, "{ foo?: union; }");
AssertType((foo !== undefined && {foo}), "union");
AssertType(foo !== undefined && {foo}, "union");
AssertType(foo !== undefined, "boolean");
AssertType(foo, "Record<never, never>");
AssertType(undefined, "undefined");
AssertType({foo}, "{ foo: Record<never, never>; }");
AssertType(foo, "Record<never, never>");


