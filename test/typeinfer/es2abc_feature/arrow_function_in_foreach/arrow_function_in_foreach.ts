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
{
    class A {
       a1 : number;
       a2 : string; 
    }

    class B {
        b1 : A[]
    }

    function C() {
        let d = new B();
        d.b1.forEach((value, index, arr)=>{
            AssertType(value, "A");
            AssertType(index, "int");
            AssertType(arr, "A[]");
            let e = arr[index].a1 + value.a1;
            AssertType(e, "number");
            return e
        });
    }

let f : number[] = [1,2,3]
f.forEach(value => { 
    AssertType(value, "number");
    return value + 1;
})
f.forEach((value, index)=>{
    AssertType(value, "number");
    AssertType(index, "int");
    AssertType(f[index], "number");
    return value + f[index];
})
f.forEach((value, index, arr)=>{
    AssertType(value, "number");
    AssertType(index, "int");
    AssertType(arr, "number[]");
    return value + arr[index];})
}
