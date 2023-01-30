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

// === tests/cases/compiler/getterSetterNonAccessor.ts ===
declare function AssertType(value:any, type:string):void;
function getFunc():any{
AssertType(0, "int");
return 0;
}

function setFunc(v){}          

Object.defineProperty({}, "0", <PropertyDescriptor>({
AssertType(Object.defineProperty({}, "0", <PropertyDescriptor>({          get: getFunc,          set: setFunc,          configurable: true      })), "{}");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType({}, "{}");
AssertType("0", "string");
AssertType(<PropertyDescriptor>({          get: getFunc,          set: setFunc,          configurable: true      }), "PropertyDescriptor");
AssertType(({          get: getFunc,          set: setFunc,          configurable: true      }), "{ get: () => any; set: (any) => void; configurable: true; }");
AssertType({          get: getFunc,          set: setFunc,          configurable: true      }, "{ get: () => any; set: (any) => void; configurable: true; }");

          get: getFunc,
AssertType(get, "() => any");
AssertType(getFunc, "() => any");

          set: setFunc,
AssertType(set, "(any) => void");
AssertType(setFunc, "(any) => void");

          configurable: true
AssertType(configurable, "boolean");
AssertType(true, "boolean");

      }));


