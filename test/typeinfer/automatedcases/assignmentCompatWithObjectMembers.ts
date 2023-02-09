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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/assignmentCompatWithObjectMembers.ts ===
declare function AssertType(value:any, type:string):void;

// members N and M of types S and T have the same name, same accessibility, same optionality, and N is assignable M
// no errors expected

module SimpleTypes {
    class S { foo: string; }
    class T { foo: string; }
    let s: S;
    let t: T;

    interface S2 { foo: string; }
    interface T2 { foo: string; }
    let s2: S2;
    let t2: T2;

    let a: { foo: string; }
    let b: { foo: string; }

    let a2 = { foo: '' };
    let b2 = { foo: '' };

    s = t;
    t = s;
    s = s2;
    s = a2;

    s2 = t2;
    t2 = s2;
    s2 = t;
    s2 = b;
    s2 = a2;

    a = b;
    b = a;
    a = s;
    a = s2;
    a = a2;

    a2 = b2;
    b2 = a2;
    a2 = b;
    a2 = t2;
    a2 = t;
}

module ObjectTypes {
    class S { foo: S; }
    class T { foo: T; }
    let s: S;
    let t: T;

    interface S2 { foo: S2; }
    interface T2 { foo: T2; }
    let s2: S2;
    let t2: T2;

    let a: { foo: typeof a; }
    let b: { foo: typeof b; }

    let a2 = { foo: a2 };
    let b2 = { foo: b2 };

    s = t;
    t = s;
    s = s2;
    s = a2;

    s2 = t2;
    t2 = s2;
    s2 = t;
    s2 = b;
    s2 = a2;

    a = b;
    b = a;
    a = s;
    a = s2;
    a = a2;

    a2 = b2;
    b2 = a2;
    a2 = b;
    a2 = t2;
    a2 = t;

}

