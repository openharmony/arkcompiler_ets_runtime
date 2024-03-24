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

/*
 * @tc.name:sharedarrayerror
 * @tc.desc:test sharedarrayerror
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
declare function print(str: any): string;

class NormalClass {
    public num: number = 0;
    constructor(num: number) {
        this.num = num;
    }
}

function createErrorTest(): void {
    print("Start createErrorTest")
    try {
        const arr = new SharedArray<string>(-1);
        print("Init with length: -1 success.");
    } catch (err) {
        print("Init with length: -1, err: " + err + ", errCode: " + err.code);
    }

    try {
        const arr = new SharedArray<string>(0xffff);
        print("Init with max length: 0xffff success.");
    } catch (err) {
        print("Init with max length: 0xffff, err: " + err + ", errCode: " + err.code);
    }

    try {
        const arr = new SharedArray<string>(0xffffffffff);
        print("Init exceed max length success.");
    } catch (err) {
        print("Init exceed max length: err: " + err + ", errCode: " + err.code);
    }

    try {
        const arr = new SharedArray<NormalClass>(new NormalClass(1), new NormalClass(2));
        print("Create with non-sendable element success.");
    } catch (err) {
        print("Create with non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function fromErrorTest(): void {
    print("Start fromErrorTest")
    try {
        SharedArray.from<NormalClass>(Array.from([new NormalClass(1), new NormalClass(2)]))
        print("Create from non-sendable iterator success.");
    } catch (err) {
        print("Create from non-sendable iterator fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        SharedArray.from<NormalClass>([new NormalClass(1), new NormalClass(2)])
        print("Create from non-sendable element success.");
    } catch (err) {
        print("Create from non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
    try {
        SharedArray.from<number, NormalClass>([1, 2, 3], (x: number) => new NormalClass(x));
        print("Create from mapper: non-sendable element success.");
    } catch (err) {
        print("Create from mapper: non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function atErrorTest() {
    // TODO: remove 401
    print("Start atErrorTest")
    const array1 = new SharedArray<number>(5, 12, 8, 130, 44);
    try {
        print("at invalid index success: " + array1.at("hi"));
    } catch (err) {
        print("at invalid index: err: " + err + ", errCode: " + err.code);
    }
}

function concatErrorTest(): void {
    print("Start concatErrorTest")
    let array: SharedArray<number> = new SharedArray<number>(1, 3, 5);
    let normalArray = new Array<NormalClass>(new NormalClass(2), new NormalClass(4), new NormalClass(6));

    try {
        array.concat(normalArray);
        print("concat with non-sendable array success.");
    } catch (err) {
        print("concat with non-sendable array fail.err: " + err + ", errCode: " + err.code);
    }

    try {
        array.concat(new NormalClass(2));
        print("concat with non-sendable element success.");
    } catch (err) {
        print("concat with non-sendable element fail. err: " + err + ", errCode: " + err.code);
    }
}

function bindErrorTest() {
    print("Start atErrorTest")
    const array1 = new SharedArray<number>(5, 12, 8, 130, 44);
    const normal = new NormalClass(10);
    const unbouondAt = array1.at;
    const boundAt = unbouondAt.bind(normal);
    try {
        boundAt(2)
        print("call boundAt success.");
    } catch (err) {
        print("call boundAt fail. err: " + err + ", errCode: " + err.code);
    }
}

createErrorTest()
fromErrorTest()
atErrorTest()
concatErrorTest()

bindErrorTest()