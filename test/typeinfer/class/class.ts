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
    t1:number;
    constructor(t1:number) {
      this.t1 = t1;
    }
    fun1():boolean {
      return true;
    }
    static fun2():void {
      return;
    }
    static t2:string = "aot";
}

let s1 = new A(1).t1;
let s2 = new A(1).fun1();
let s3 = A.t2;
let s4 = A.fun2();

AssertType(s1, "number");
AssertType(s2, "boolean");
AssertType(s3, "string");
AssertType(s4, "void");
