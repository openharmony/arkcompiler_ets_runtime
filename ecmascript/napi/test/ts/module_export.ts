/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

// export namspace/function
export namespace TestNamespace1 {
    export function testFunction(num : number) {
        return num
    }
}

//export namespace/class/function
export namespace TestNamespace2 {
    export class TestClass1 {
        value: number

        constructor() {
            this.value = 0
        }

        testFunction(num1 : number, num2 : number) {
            return num1 + num2
        }
    }
}


// export class/function
export class TestClass2 {
    value: number

    constructor() {
        this.value = 0
    }

    testFunction(num1 : number, num2 : number) {
        return num1 + num2
    }
}

// export function
export function testFunction(num1 : number, num2 : number) {
    return num1 + num2
}
