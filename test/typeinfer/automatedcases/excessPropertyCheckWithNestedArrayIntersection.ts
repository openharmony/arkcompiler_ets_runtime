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

// === tests/cases/compiler/excessPropertyCheckWithNestedArrayIntersection.ts ===
declare function AssertType(value:any, type:string):void;
interface ValueOnlyFields {
    fields: Array<{
        value: number | null;
    }>;
}
interface ValueAndKeyFields {
    fields: Array<{
        key: string | null;
        value: number | null;
    }>;
}
interface BugRepro {
  dataType: ValueAndKeyFields & ValueOnlyFields;
}
const repro: BugRepro = {
AssertType(repro, "BugRepro");
AssertType({  dataType: {    fields: [{      key: 'bla', // should be OK: Not excess      value: null,    }],  }}, "{ dataType: { fields: { key: string; value: null; }[]; }; }");

  dataType: {
AssertType(dataType, "{ fields: { key: string; value: null; }[]; }");
AssertType({    fields: [{      key: 'bla', // should be OK: Not excess      value: null,    }],  }, "{ fields: { key: string; value: null; }[]; }");

    fields: [{
AssertType(fields, "{ key: string; value: null; }[]");
AssertType([{      key: 'bla', // should be OK: Not excess      value: null,    }], "{ key: string; value: null; }[]");
AssertType({      key: 'bla', // should be OK: Not excess      value: null,    }, "{ key: string; value: null; }");

      key: 'bla', // should be OK: Not excess
AssertType(key, "string");
AssertType('bla', "string");

      value: null,
AssertType(value, "null");
AssertType(null, "null");

    }],
  }
}


