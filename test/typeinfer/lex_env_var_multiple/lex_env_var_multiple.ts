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
    function f():any {
        let a:number = 1;
        let b:string = "abc";
        let c = a + b;
        function g():any {
            AssertType(c, "string");
            AssertType(a, "int");
            AssertType(b, "string");
            function h():any {
                AssertType(c, "string");
                AssertType(a, "int");
                AssertType(b, "string");
                return a;
            }
            return h();
        }
        AssertType(c, "string");
        AssertType(a, "int");
        AssertType(b, "string");
        return g();
    }

    let t:number = 1;
    function x():any {
        function y():any {
            function z():any {
                let s:boolean = true;
                function w():any {
                    AssertType(s, "boolean");
                    AssertType(t, "int");
                    AssertType(t + "abc", "string");
                    return t;
                }
                AssertType(t, "int");
                return w();
            }
            AssertType(t, "int");
            return z();
        }
        AssertType(t, "int");
        return y();
    }
    AssertType(t, "int");
}
