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
    let a : boolean[] = [];
    let b = a.push(false);
    AssertType(b, "number");
    let c : number[] = [1, 2, 3];
    let d = c.indexOf(2);
    AssertType(d, "number");

    let t = 2;
    let arr1:number[] = new Array(100).fill(0);
    AssertType(arr1, "number[]");
    AssertType(arr1[0], "number");
    AssertType(arr1[t], "number");

    let arr2:number[] = new Array(100).fill(0, 0);
    AssertType(arr2, "number[]");
    AssertType(arr2[0], "number");
    AssertType(arr2[t], "number");

    let arr3:number[] = new Array(100).fill(0, 0, 99);
    AssertType(arr3, "number[]");
    AssertType(arr3[0], "number");
    AssertType(arr3[t], "number");

    let arr4:string[] = new Array(100).fill("abc");
    AssertType(arr4, "string[]");
    AssertType(arr4[0], "string");
    AssertType(arr4[0], "string");

    let arr5:number[] = new Array(100);
    AssertType(arr5.fill(0), "number[]");
    AssertType(arr5.sort(), "number[]");
    AssertType(arr5.copyWithin(0, 3, 4), "number[]");
    AssertType(arr5.fill(0)[0], "number");
    AssertType(arr5.fill(0)[t], "number");
    AssertType(arr5.fill(0).sort(), "number[]");
    AssertType(arr5.fill(0).sort().copyWithin(0, 3, 4), "number[]");

    function foo():number[] {
        return [1, 2, 3, 4];
    }

    let arr6 = foo();
    AssertType(arr6.fill(0), "number[]");
    AssertType(arr6.sort(), "number[]");
    AssertType(arr6.copyWithin(0, 3, 4), "number[]");
    AssertType(arr6.fill(0)[0], "number");
    AssertType(arr6.fill(0)[t], "number");

    let arr7 = new Array<number>(10);
    AssertType(arr7, "number[]");
    AssertType(arr7[0], "number");
    AssertType(arr7[t], "number");
    AssertType(arr7.fill(0), "number[]");
    AssertType(arr7.sort(), "number[]");
    AssertType(arr7.copyWithin(0, 3, 4), "number[]");
    AssertType(arr7.fill(0)[0], "number");
    AssertType(arr7.fill(0)[t], "number");
}
