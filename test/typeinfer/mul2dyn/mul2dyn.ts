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
    let ans1 = num1 * num2;
    AssertType(ans1, "int");

    let double1 : number = 1.5;
    let ans2 = double1 * double1;
    let ans3 = num1 * double1;
    let ans4 = double1 * num2;
    let ans5 = num2 * double1;
    AssertType(ans2, "double");
    AssertType(ans3, "double");
    AssertType(ans4, "double");
    AssertType(ans5, "double");

    let zero1 : number = 0;
    let ans6 = double1 * zero1;
    AssertType(ans6, "double");

    let inf1 : number = Infinity;
    let nan1 : number = NaN;
    let ans7 = inf1 * inf1;
    let ans8 = inf1 * nan1;
    let ans9 = nan1 * inf1;
    let ans10 = num1 * inf1;
    let ans11 = inf1 * num1;
    let ans12 = num1 * nan1;
    let ans13 = nan1 * num1;
    let ans14 = double1 * inf1;
    let ans15 = inf1 * double1;
    let ans16 = double1 * nan1;
    let ans17 = nan1 * double1;
    AssertType(ans7, "number");
    AssertType(ans8, "number");
    AssertType(ans9, "number");
    AssertType(ans10, "number");
    AssertType(ans11, "number");
    AssertType(ans12, "number");
    AssertType(ans13, "number");
    AssertType(ans14, "double");
    AssertType(ans15, "double");
    AssertType(ans16, "double");
    AssertType(ans17, "double");
}
