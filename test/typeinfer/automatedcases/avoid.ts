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

// === tests/cases/compiler/avoid.ts ===
declare function AssertType(value:any, type:string):void;
function f() {
    let x=1;
AssertType(x, "number");
AssertType(1, "int");
}

let y=f();  // error void fn
AssertType(y, "void");
AssertType(f(), "void");
AssertType(f, "() => void");

let why:any=f(); // error void fn
AssertType(why, "any");
AssertType(f(), "void");
AssertType(f, "() => void");

let w:any;
AssertType(w, "any");

w=f(); // error void fn
AssertType(w=f(), "void");
AssertType(w, "any");
AssertType(f(), "void");
AssertType(f, "() => void");

class C {
    g() {
        
    }
}

let z=new C().g(); // error void fn
AssertType(z, "void");
AssertType(new C().g(), "void");
AssertType(new C().g, "() => void");

let N=new f();  // ok with void fn
AssertType(N, "any");
AssertType(new f(), "any");
AssertType(f, "() => void");



