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
    let obj1 : { [v:string]:number } = {value1: 123, value2: 456};
    let obj2 : { [v:string]:boolean } = {value1: true, value2: false};
    let obj3 : { [v:number]:number } = [1, 2, 3];
    let t:number = 0;
    let s:number = 1;

    AssertType(obj1.value1, "number");
    AssertType(obj1.value2, "number");
    AssertType(obj2.value1, "boolean");
    AssertType(obj2.value2, "boolean");
    AssertType(obj3[0], "number");
    AssertType(obj3[1], "number");
    AssertType(obj3[2], "number");
    AssertType(obj3[t], "number");
    AssertType(obj3[s], "number");
}
