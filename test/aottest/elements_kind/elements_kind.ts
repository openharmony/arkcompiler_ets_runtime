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

declare function print(arg: any): string;

function foo() {
    let intArray: number[] = [1, 2, 3];
    print(intArray[0]);

    let doubleArray: number[] = [1.1, 2.2, 3.3];
    print(doubleArray[1]);

    let objectArray: object[] = [{}, {}, {}];
    print(objectArray[1]);

    let holeyArray: (number | undefined | object)[] = [1, , {}];
    print(holeyArray[1]);
}

foo();


class C {
    x: number;
    constructor(v: number) {
        this.x = v;
    }
}

function testObject() {
    let objArray: C[] = [new C(1), new C(2)];
    for (let i: number = 0; i < objArray.length; ++i) {
        print(objArray[i].x);
    }
}

testObject();

