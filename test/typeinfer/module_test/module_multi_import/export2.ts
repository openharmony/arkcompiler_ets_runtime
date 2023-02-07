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
import { Test3 , t2} from "./export3";
export function Test2(a1 : Test3, a2 : number) : number{
    return a1.foo() + a2;
}

let t0 = new Test3(456)
let t1 = Test2(t0, 123);
let t3 = t1 + t2;

AssertType(t0, "Test3");
AssertType(Test2, "(Test3, number) => number");
AssertType(t1, "number");
AssertType(t3, "double");