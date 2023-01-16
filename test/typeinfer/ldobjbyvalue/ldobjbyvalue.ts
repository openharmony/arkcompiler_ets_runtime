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
        v1:number;
        constructor(v1:number) {
            this.v1 = v1;
        }
        5() : number {
            return this.v1;
        }
    }

    let arr1:string[] = ["a", "b", "c"];
    let arr2:number[] = [1, 2, 3, 4];
    let arr3:boolean[] = [true, false, false];
    let a1 = new A(0);
    let a2 = new A(1);
    let arr4:A[] = [a1, a2];
    let t = 1;
    let name = "v1";
    let value = 5;
    let a3 = a1[name];
    let a4 = a2[value]();
    AssertType(arr1[t], "string");
    AssertType(arr2[t], "number");
    AssertType(arr3[t], "boolean");
    AssertType(arr4[t], "A");
    AssertType(a3, "number");
    AssertType(a4, "number");
}
