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
    let sum = (a:number, b:number):number => {
        return a + b;
    }

    let combine = (v1:number, v2:number, v3:number):number[] => {
        let arr:number[] = [v1, v2, v3];
        return arr;
    }

    AssertType(sum, "(number, number) => number");
    AssertType(sum(1, 2), "number");
    AssertType(combine, "(number, number, number) => number[]");
    AssertType(combine(1, 2, 3), "number[]");
}
