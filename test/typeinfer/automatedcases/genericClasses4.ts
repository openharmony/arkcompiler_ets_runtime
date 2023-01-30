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

// === tests/cases/compiler/genericClasses4.ts ===
declare function AssertType(value:any, type:string):void;
// once caused stack overflow
class Vec2_T<A>
{
    constructor(public x: A, public y: A) { }
    fmap<B>(f: (a: A) => B): Vec2_T<B> {
        let x:B = f(this.x);
AssertType(x, "B");
AssertType(f(this.x), "B");
AssertType(f, "(A) => B");
AssertType(this.x, "A");
AssertType(this, "this");

        let y:B = f(this.y);
AssertType(y, "B");
AssertType(f(this.y), "B");
AssertType(f, "(A) => B");
AssertType(this.y, "A");
AssertType(this, "this");

        let retval: Vec2_T<B> = new Vec2_T(x, y);
AssertType(retval, "Vec2_T<B>");
AssertType(new Vec2_T(x, y), "Vec2_T<B>");
AssertType(Vec2_T, "typeof Vec2_T");
AssertType(x, "B");
AssertType(y, "B");

AssertType(retval, "Vec2_T<B>");
        return retval;
    }
    apply<B>(f: Vec2_T<(a: A) => B>): Vec2_T<B> {
        let x:B = f.x(this.x);
AssertType(x, "B");
AssertType(f.x(this.x), "B");
AssertType(f.x, "(A) => B");
AssertType(this.x, "A");
AssertType(this, "this");

        let y:B = f.y(this.y);
AssertType(y, "B");
AssertType(f.y(this.y), "B");
AssertType(f.y, "(A) => B");
AssertType(this.y, "A");
AssertType(this, "this");

        let retval: Vec2_T<B> = new Vec2_T(x, y);
AssertType(retval, "Vec2_T<B>");
AssertType(new Vec2_T(x, y), "Vec2_T<B>");
AssertType(Vec2_T, "typeof Vec2_T");
AssertType(x, "B");
AssertType(y, "B");

AssertType(retval, "Vec2_T<B>");
        return retval;
    }
}

