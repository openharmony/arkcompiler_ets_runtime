/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

class A {
    a : number;
    b : string;
    constructor(x : number, y : string) {
        this.a = x;
        this.b = y;
    }
}

function foo(a1: A, a2: number, a3 : number) : number 
{
    let b1 = a1.a;
    AssertType(b1, "number");
    let b2 = a1.b;
    AssertType(b2, "string");
    let b3 = a2 - a3;
    AssertType(b3, "number");
    return b3;
}