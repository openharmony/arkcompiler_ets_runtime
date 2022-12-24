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
    AssertType(ans1, "int");

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

    let double1 : number = 0.1;
    let double2 : number = 0.2;
    let double3 : number = 0.8;
    let ans6 = double1 + double2;
    let ans7 = double2 + double3;
    AssertType(ans6, "double");
    AssertType(ans7, "double");

    let ans8 = double1 + num1;
    AssertType(ans8, "double");

    let ans9 = num1 + double1;
    AssertType(ans9, "double");

    let inf1 : number = Infinity;
    let nan1 : number = NaN;
    let ans10 = inf1 + inf1;
    let ans11 = inf1 + nan1;
    let ans12 = nan1 + inf1;
    let ans13 = num1 + inf1;
    let ans14 = inf1 + num1;
    let ans15 = num1 + nan1;
    let ans16 = nan1 + num1;
    let ans17 = double1 + inf1;
    let ans18 = inf1 + double1;
    let ans19 = double1 + nan1;
    let ans20 = nan1 + double1;
    AssertType(ans10, "number");
    AssertType(ans11, "number");
    AssertType(ans12, "number");
    AssertType(ans13, "number");
    AssertType(ans14, "number");
    AssertType(ans15, "number");
    AssertType(ans16, "number");
    AssertType(ans17, "double");
    AssertType(ans18, "double");
    AssertType(ans19, "double");
    AssertType(ans20, "double");
}
