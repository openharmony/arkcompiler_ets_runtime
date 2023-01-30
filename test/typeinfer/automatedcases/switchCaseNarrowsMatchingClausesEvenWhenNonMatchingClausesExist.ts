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

// === tests/cases/compiler/switchCaseNarrowsMatchingClausesEvenWhenNonMatchingClausesExist.ts ===
declare function AssertType(value:any, type:string):void;
export const narrowToLiterals = (str: string) => {
AssertType(narrowToLiterals, "(string) => union");
AssertType(str, "string");
AssertType((str: string) => {    switch (str) {      case 'abc': {        // inferred type as `abc`        return str;      }      default:        return 'defaultValue';    }  }, "(string) => union");

    switch (str) {
AssertType(str, "string");

      case 'abc': {
AssertType('abc', "string");

        // inferred type as `abc`
AssertType(str, "string");
        return str;
      }
      default:
AssertType('defaultValue', "string");
        return 'defaultValue';
    }
  };
  
  export const narrowToString = (str: string, someOtherStr: string) => {
AssertType(narrowToString, "(string, string) => string");
AssertType(str, "string");
AssertType(someOtherStr, "string");
AssertType((str: string, someOtherStr: string) => {    switch (str) {      case 'abc': {        // inferred type should be `abc`        return str;      }      case someOtherStr: {        // `string`        return str;      }      default:        return 'defaultValue';    }  }, "(string, string) => string");

    switch (str) {
AssertType(str, "string");

      case 'abc': {
AssertType('abc', "string");

        // inferred type should be `abc`
AssertType(str, "string");
        return str;
      }
      case someOtherStr: {
AssertType(someOtherStr, "string");

        // `string`
AssertType(str, "string");
        return str;
      }
      default:
AssertType('defaultValue', "string");
        return 'defaultValue';
    }
  };
  
  export const narrowToStringOrNumber = (str: string | number, someNumber: number) => {
AssertType(narrowToStringOrNumber, "(union, number) => union");
AssertType(str, "union");
AssertType(someNumber, "number");
AssertType((str: string | number, someNumber: number) => {    switch (str) {      case 'abc': {        // inferred type should be `abc`        return str;      }      case someNumber: {        // inferred type should be `number`        return str;      }      default:        return 'defaultValue';    }  }, "(union, number) => union");

    switch (str) {
AssertType(str, "union");

      case 'abc': {
AssertType('abc', "string");

        // inferred type should be `abc`
AssertType(str, "string");
        return str;
      }
      case someNumber: {
AssertType(someNumber, "number");

        // inferred type should be `number`
AssertType(str, "number");
        return str;
      }
      default:
AssertType('defaultValue', "string");
        return 'defaultValue';
    }
  };

