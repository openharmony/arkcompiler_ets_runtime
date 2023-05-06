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
// @ts-nocheck
declare function print(a:any):string;

class C {
    x:number;
    constructor() {
        this.x = 1;
    }
}

class D extends C {
    y:number;
    constructor() {
        super();
        this.y = 2;
    }
}

function foo(o:D) {
    print(o.x);
}

let d = new D();
foo(d);

d = {};
foo(d);

d = 1;
foo(d);

try {
    d = undefined;
    foo(d);
} catch(e) {
    print(e instanceof TypeError);
}

