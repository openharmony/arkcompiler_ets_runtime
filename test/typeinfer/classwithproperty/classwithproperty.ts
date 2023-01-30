/*
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
 */

declare function AssertType(value:any, type:string):void;

class C {
    public x;
    public b: string = '';
    public c(): string { 
        AssertType('', "string");
        return '' 
    }

    public static e;
    public static f() : string { 
        AssertType('', "string");
        return '' 
    }
}

let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let r1: string = c.x;
AssertType(r1, "string");
AssertType(c.x, "any");

let tmp = "x";
let value1: string = c[tmp]
AssertType(c[tmp], "any");
AssertType(value1, "string");

let r3: string = c.b;
AssertType(r3, "string");
AssertType(c.b, "string");

tmp = "b";
let value2: string = c[tmp]
AssertType(c[tmp], "string");
AssertType(value2, "string");

let r4: string = c.c();
AssertType(r4, "string");
AssertType(c.c(), "string");
AssertType(c.c, "() => string");

let r6: string = C.e;
AssertType(r6, "string");
AssertType(C.e, "any");

tmp = "e";
let value3: string = c[tmp]
AssertType(c[tmp], "any");
AssertType(value3, "string");

let r7: string = C.f();
AssertType(r7, "string");
AssertType(C.f(), "string");
AssertType(C.f, "() => string");