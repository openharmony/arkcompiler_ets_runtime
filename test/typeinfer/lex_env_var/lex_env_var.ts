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
    let s:number = 1;
    let t:number = 2;
    function f():any {
        let a:string = "1";
        let b:number = 3;
        let c = a + b;
        let d = b + 2;
        let r = s + t;
        AssertType(r, "int");
        function g():any {
            AssertType(c, "string");
            AssertType(d, "int");
            AssertType(c + d, "string");
            AssertType(r, "int");
            return c + d;
        }
        return g();
    }
    AssertType(s + t, "int");
}
