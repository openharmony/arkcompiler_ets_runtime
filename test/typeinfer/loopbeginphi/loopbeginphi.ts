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
    function p(num : number) :any {
        if (num == 1) {
            return 1;
        }
        return 2 + p(num - 1);
    }


    let a1 :number = 1;

    for (let i = 0 ; i < 10; i++){
        // loop begin phi (int, any) -> any
        AssertType(a1, "any");
        a1 = a1 + p(10);
    }
    function func() : number {
        return 1;
    }
    let res : number = 3;
    for (let i : number = 1; i < 10; i++) {
        // loop begin phi (int, number) -> number
        AssertType(res, "number");
        res = res + func();
    }
}
