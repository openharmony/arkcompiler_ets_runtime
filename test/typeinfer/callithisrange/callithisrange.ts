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
{
    class A {
        v:number;
        constructor(v:number) {
            this.v = v;
        }
        fun(value:number):number { 
            return this.v + value;
        }
        fun1():void { 
            this.v += 1;
        }
        fun2():string { 
            return "hello";
        }
        fun3():boolean { 
            return true;
        }
        fun4():B { 
            let b = new B(this.v);
            return b;
        }
        fun5():number[] { 
            return [this.v, this.v];
        }
    }

    class B {
        v:number;
        constructor(v:number) {
            this.v = v + 1;
        }
    }

    let a = new A(1);
    AssertType(a.fun(2), "number");
    AssertType(a.fun1(), "void");
    AssertType(a.fun2(), "string");
    AssertType(a.fun3(), "boolean");
    AssertType(a.fun4(), "B");
    AssertType(a.fun5(), "number[]");
}
