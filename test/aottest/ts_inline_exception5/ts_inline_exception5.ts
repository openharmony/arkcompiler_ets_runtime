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
declare class ArkTools {
    static hiddenStackSourceFile(): boolean;
}
ArkTools.hiddenStackSourceFile()

try {
    function foo1() {
        new Array(111111111111111111111)
    }
    function foo2() {
        foo1()
    }
    function foo3(a) {
        foo2()
    }
    function foo4() {
        foo3()
    }
    foo4()
} catch (e) {
    print(e)
    print(e.stack)
}
