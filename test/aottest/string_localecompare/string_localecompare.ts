/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

declare function print(arg:any, arg1?:any):string;

let index = 0;
function randomId()
{
    let str = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    let res = '';
    for (let i = 0; i < 13; i++) {
        let n = index + i;
        res += str[n % str.length];
    }
    index++;
    return res;
}

let testCount = 0;
function localeCompareTest()
{
    const count = 100;
    let sortNumber = []
    for (let i = 0; i < count; i++) {
        sortNumber.push(randomId());
    }
    if (testCount++ == 1) {
        sortNumber.sort((a, b) => {
            return a.localeCompare(b)
        });
    } else if (testCount++ == 2) {
        sortNumber.sort((a, b) => {
            return a.localeCompare(b, undefined)
        });
    } else {
        sortNumber.sort((a, b) => {
            return a.localeCompare(b, undefined, undefined)
        });
    }

    for (let i = 1; i < count; i += 20) {
        print(sortNumber[i])
    }
}

localeCompareTest();
localeCompareTest();
localeCompareTest();
localeCompareTest();
