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
    let num1 : number = 1;
    let num2 : number = 2;
    let ans1 = num1 + num2;
    AssertType(ans1, "number");

    let arr1 : number[] = [1, 2];
    let arr2 : number[] = [1, 2];
    let ans2 = arr1[0] + arr2[0];
    AssertType(ans2, "number");

    let arr3 : string[] = ['1'];
    let arr4 : string[] = ['1'];
    let ans3 = arr3[0] + arr4[0];
    AssertType(ans3, "string");

    let str1 : string = '1';
    let str2 : string = '2';
    let ans4 = str1 + str2;
    AssertType(ans4, "string");

    let ans5 = arr1[0] + arr3[0];
    AssertType(ans5, "string");
}
