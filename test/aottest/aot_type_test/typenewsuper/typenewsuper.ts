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

declare function print(arg:any):string;
class C {
    a:number;
    constructor() {}
}

class D extends C {
    b:string;
    constructor(x:number, y:string) {
        super();
        this.a = x;
        this.b = y;
    }
}

let d = new D(123, "abc");
print(d.a);
print(d.b);


class E {
    a:number;
    constructor() {
        this.a = 123;
        throw TypeError("type error");
    }
}

try {
    new E();
} catch(e) {
    print(e);
}
