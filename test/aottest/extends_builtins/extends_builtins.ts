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

declare function assert_true(condition: boolean):void;

class MyBoolean extends Boolean {
    constructor(arg:boolean) {
        super(arg);             // stub path
    }
}
let b = new MyBoolean(true);
assert_true(b instanceof MyBoolean);
assert_true(b instanceof Boolean);


class MyNumber extends Number {
    constructor(arg:number) {
        super(arg);             // stub path
    }
}

let n = new MyNumber(1)
assert_true(n instanceof MyNumber);
assert_true(n instanceof Number);


class MyDate extends Date {
    constructor(arg:any) {
        super(arg);
    }
}

let d = new MyDate(16455456000)
assert_true(d instanceof MyDate);
assert_true(d instanceof Date);


class MyArray extends Array {
    constructor(arg:number) {
        super(arg);
    }
}

let a = new MyArray(1);
assert_true(a instanceof MyArray);
assert_true(a instanceof Array);

