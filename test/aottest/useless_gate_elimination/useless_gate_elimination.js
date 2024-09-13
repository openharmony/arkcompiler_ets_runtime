/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

"use strict";
let i, j;
let cnt = 0;
for (i = 0; i < 10000; i++) {
    for (j = 0; j < 10000; j++) {
        cnt = cnt + i + j;
    }
}

class C3 {}

let v4 = new C3()

class C27 {
    static #a;
    constructor() {
        try{
            this.#a
        } catch(e) {
            v4.a = 1
        }
    }
}

new C27()
new C27()