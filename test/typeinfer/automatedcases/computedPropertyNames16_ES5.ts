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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNames16_ES5.ts ===
declare function AssertType(value:any, type:string):void;
let s: string;
AssertType(s, "string");

let n: number;
AssertType(n, "number");

let a: any;
AssertType(a, "any");

class C {
    get [s]() { 
AssertType(0, "int");
return 0;
}

    set [n](v) { }
    static get [s + s]() { 
AssertType(0, "int");
return 0; 
}

    set [s + n](v) { }
    get [+s]() { 
AssertType(0, "int");
return 0; 
}

    static set [""](v) { }
    get [0]() { 
AssertType(0, "int");
return 0; 
}

    set [a](v) { }
    static get [<any>true]() { 
AssertType(0, "int");
return 0; 
}

    set [`hello bye`](v) { }
    get [`hello ${a} bye`]() { 
AssertType(0, "int");
return 0; 
}
}

