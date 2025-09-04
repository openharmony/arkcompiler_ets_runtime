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

/*
 * @tc.name:datecase
 * @tc.desc:test date
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var d1 = new Date("2023-02-19T11:13:48.132Z");
var d2 = new Date("Thu Nov 19 2020 20:18:18 GMT");
var d3 = new Date("Thu, 21 Nov 2023 13:23:26 GMT");
var d4 = new Date("2021-12-25 03:25:45.476Z");
var d5 = new Date("January 13,2016 22:19:35 GMT");

print(d1.getTime() == 1676805228132);
print(d1.getFullYear() == 2023);
print(d1.getMonth() == 1);
print(d1.getDate() == 19);

print(d2.getTime() == 1605817098000);
print(d2.getFullYear() == 2020);
print(d3.getMonth() == 10);

print(d3.getTime() == 1700573006000);
print(d3.getFullYear() == 2023);
print(d3.getMonth() == 10);
print(d3.getDate() == 21);

print(d4.getTime() == 1640402745476);
print(d4.getFullYear() == 2021);
print(d4.getMonth() == 11);
print(d4.getDate() == 25);

print(d5.getTime() == 1452723575000);
print(d5.getFullYear() == 2016);
print(d5.getMonth() == 0);

let d6 = new Date("Wed, 26-Jun-19 GMT")
print(d6.getTime())
let d7 = new Date("Wed, 26-Jun-2019 GMT")
print(d7.getTime())
let date8=new Date('Jun,19 12:03:45 GMT')
print(date8.getTime())

let d9 = new Date('06/15/2023 02:30:45 AM')
print(d9)
let d10 = new Date('06/15/2023 15:30:45 AM')
print(d10)
let d11 = new Date('2023-05-20 03:45:30 PM')
print(d11)
let d12 = new Date('2023-05-20 15:45:30 PM')
print(d12)
let d13 = new Date('2023-05-20 PM 03:45:30')
print(d13)
let d14 = new Date('2023-05-20 03:45:PM30')
print(d14)
let d15 = new Date('PM 2023-05-20 03:45:30')
print(d15)
let d16 = new Date('PM2023-05-20 03:45:30')
print(d16)
