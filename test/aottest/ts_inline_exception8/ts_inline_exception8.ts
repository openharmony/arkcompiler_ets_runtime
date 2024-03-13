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

declare function print(arg:any):string;
declare function assert_equal(a: Object, b: Object):void;
declare function assert_unreachable():void;
declare class ArkTools {
    static hiddenStackSourceFile(): boolean;
}
ArkTools.hiddenStackSourceFile()
class A {
    constructor() {}
    get name() {
        assert_equal(arguments.length, 0)
        a.b
    }
    set name(a) {}
}
let ins = new A()
function foo1() {
    ins.name
}
function foo2() {
    foo1()
}
try {
    foo2()
    assert_unreachable()
} catch (e) {
    assert_equal(e.message, "a is not defined")
    print(e.stack)
}