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
    let ans1 = num1 / num2;
    AssertType(ans1, "double");

    let double1 : number = 1.5;
    let double2 : number = 0.5;
    let ans2 = num1 / double1;
    let ans3 = double1 / num1;
    let ans4 = double1 / double2;
    let ans5 = double2 / double1;
    AssertType(ans2, "double");
    AssertType(ans3, "double");
    AssertType(ans4, "double");
    AssertType(ans5, "double");

    let zero1 : number = 0;
    let ans6 = num1 / zero1;
    let ans7 = double1 / zero1;
    AssertType(ans6, "double");
    AssertType(ans7, "double");

    let inf1 : number = Infinity;
    let nan1 : number = NaN;
    let ans8 = inf1 / inf1;
    let ans9 = inf1 / nan1;
    let ans10 = nan1 / inf1;
    let ans11 = num1 / inf1;
    let ans12 = inf1 / num1;
    let ans13 = num1 / nan1;
    let ans14 = nan1 / num1;
    let ans15 = double1 / inf1;
    let ans16 = inf1 / double1;
    let ans17 = double1 / nan1;
    let ans18 = nan1 / double1;
    AssertType(ans8, "double");
    AssertType(ans9, "double");
    AssertType(ans10, "double");
    AssertType(ans11, "double");
    AssertType(ans12, "double");
    AssertType(ans13, "double");
    AssertType(ans14, "double");
    AssertType(ans15, "double");
    AssertType(ans16, "double");
    AssertType(ans17, "double");
    AssertType(ans18, "double");
}
