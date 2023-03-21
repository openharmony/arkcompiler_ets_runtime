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

import {Test1} from "./export1"
class Test2 extends Test1 {
    value2:number;
    constructor(value1:string, value2:number) {
        super(value1);
        this.value2 = value2;
    }
}

let t2 = new Test2("123", 2);
AssertType(t2, "Test2");
AssertType(t2.value1, "string");
AssertType(t2.value2, "number");

export {Test2}
