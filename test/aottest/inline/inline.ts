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

declare function print(str:any):void;

class A {
    add(a : number, b : number) : number {
        return a + b;
    }

    select(a : number, b : number) : number {
        return a > b ? a : b;
    }

    foo(a: number) : number {
        return this.add(a*2, a);
    }

    bar(a: number) : number {
        return this.select(a*2, a);
    }
}

let a : A = new A();
print(a.foo(2));
print(a.bar(2));

function f523() {
    throw "error";
    function f537() {}
}
try {
    f523();
    f523();
} catch (e) {}
