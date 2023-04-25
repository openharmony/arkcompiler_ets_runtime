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
declare function print(a:any, b?:any):string;

class C {
    x:number = 1;
    y:number = 2;
    z:number = 3;

    public update(d:number) {
        const x = this.x;  // occur deopt
        const {
            y
        } = this;
        print(y);
    }
}

let c = new C();
C.prototype.foo = 123;
c.update(123);

