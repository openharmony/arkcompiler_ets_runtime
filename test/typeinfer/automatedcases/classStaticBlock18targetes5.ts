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

// === tests/cases/conformance/classes/classStaticBlock/classStaticBlock18.ts ===
declare function AssertType(value:any, type:string):void;
function foo () {
AssertType(class {    static foo = 1;    static {      const c = class {        static bar = 2;        static {          // do        }      }    }  }, "typeof (Anonymous class)");
  return class {

    static foo = 1;
AssertType(foo, "number");
AssertType(1, "int");

    static {
      const c = class {
AssertType(c, "typeof c");
AssertType(class {        static bar = 2;        static {          // do        }      }, "typeof c");

        static bar = 2;
AssertType(bar, "number");
AssertType(2, "int");

        static {
          // do
        }
      }
    }
  }
}


