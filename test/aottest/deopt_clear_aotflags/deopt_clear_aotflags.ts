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
declare function print(arg:any):void;

function foo(v: number): Object {
    function tryHello(v: number): void {
        let a: number = 1;
        let ret: number = a + v;
        print(ret);
    }
    if (v < 0) {
        tryHello.x = 1;
        tryHello.y = 1;
        return tryHello;
    } else {
        tryHello.y = 1;
        return tryHello;
    }
}

const func1 = foo(-1);
const func2 = foo(2);
for (let i = 0; i < 11; i++) {
    func1(undefined);
}
func2(3);