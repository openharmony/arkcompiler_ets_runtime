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

// === tests/cases/compiler/restParameterAssignmentCompatibility.ts ===
declare function AssertType(value:any, type:string):void;
class T {
    m(...p3) {

    }
}

class S {
    m(p1, p2) {

    }
}

let t: T;
AssertType(t, "T");

let s: S;
AssertType(s, "S");

// M is a non - specialized call or construct signature and S' contains a call or construct signature N where,
//  the number of non-optional parameters in N is less than or equal to the total number of parameters in M,
t = s; // Should be valid (rest params correspond to an infinite expansion of parameters)
AssertType(t = s, "S");
AssertType(t, "T");
AssertType(s, "S");

class T1 {
    m(p1?, p2?) {

    }
}
let t1: T1;
AssertType(t1, "T1");

// When comparing call or construct signatures, parameter names are ignored and rest parameters correspond to an unbounded expansion of optional parameters of the rest parameter element type.
t1 = s; // Similar to above, but optionality does not matter here.
AssertType(t1 = s, "S");
AssertType(t1, "T1");
AssertType(s, "S");


