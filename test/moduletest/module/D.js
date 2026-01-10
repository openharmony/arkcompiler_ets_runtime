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

/*
 * @tc.name:module
 * @tc.desc:test module
 * @tc.type: FUNC
 * @tc.require: issueI5RC2C
 */

import { a } from "A.js"
export class D {
    constructor() {
        this.d = a
    }
}

export function add(x, y) {
    return x + y;
}

// Extended exports for testing ldexternalmodulevar optimization
export const PI = 3.14159;
export const VERSION = "1.0.0";

export let counter = 0;
export let message = "Hello from D";

export function increment() {
    counter++;
    return counter;
}

export function multiply(x, y, z) {
    return x * y * z;
}

export function getMessage() {
    return message + " - v" + VERSION;
}

export const config = {
    name: "ModuleD",
    enabled: true,
    value: 42
};

export const utils = {
    square: function(n) { return n * n; },
    cube: function(n) { return n * n * n; },
    double: function(n) { return n * 2; }
};