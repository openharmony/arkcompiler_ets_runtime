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
{
    let numObj = new Number(31_000_000);
    let start = Date.now();
    let res;
    for (let i = 0; i < 1_000_000; i++) {
        res = numObj.toLocaleString();
    }
    let end = Date.now();
    let time = (end - start);
    print(res);
    print("Number toLocaleString:\t"+String(time)+"\tms");
}
{
    let numObj = new Number(5);
    let start = Date.now();
    let res;
    for (let i = 0; i < 1_000_000; i++) {
        res = numObj.toString();
    }
    let end = Date.now();
    let time = (end - start);
    print(res);
    print("Number toString:\t"+String(time)+"\tms");
}
{
    var numObj = new Number(10);
    let start = Date.now();
    let res;
    for (let i = 0; i < 1_000_000; i++) {
        res = numObj.valueOf();
    }
    let end = Date.now();
    let time = (end - start);
    print(res);
    print("Number valueOf:\t"+String(time)+"\tms");

}
{
    var numObj = "1234";
    const time1 = Date.now()
    for(var i = 0; i < 100000; ++i) {
        res = Number.parseInt(numObj);
    }
    const time2 = Date.now()
    const time3 = time2 - time1;
    print("Number parseInt 1234 : " + time3);
}
{
    var numObj = "100.0";
    let start = Date.now();
    let res;
    for (let i = 0; i < 1_000_000; i++) {
        res = Number.parseFloat(numObj);
    }
    let end = Date.now();
    let time = (end - start);
    print(res);
    print("Number parseFloat:\t"+String(time)+"\tms");
}
{
    let start = Date.now();
    let res;
    for (let i = 0; i < 1_000_000; i++) {
        res = new Number(i);
    }
    let end = Date.now();
    let time = (end - start)
    print(res);
    print("Number Constructor:\t"+String(time)+"\tms");
}
