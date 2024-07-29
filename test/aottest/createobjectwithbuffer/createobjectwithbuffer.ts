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

declare function print(str:any):string;
let obj:{a:number, b:string}={
    a:1,
    b:"str",
}
print(obj.a);
print(obj.b);

class A {
    data: {
        runs: number,
        elapsed: number
    };
    constructor() {
    }

    Measure(): void {
        var elapsed: number = 0;
        this.data = {runs: 0, elapsed: 0}
        elapsed = 11.1;

        this.data.elapsed += elapsed;
        print(this.data.elapsed)
    }
}

let a = new A();
a.Measure();

const o46 = {
    3503: 4096,
    "maxByteLength": 4096,
};

const o48 = {
    "maxByteLength": 3012855560,
};

print(o46[3503]);
