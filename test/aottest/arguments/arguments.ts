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

class A {
    constructor(a, b, ...args){
        print("A: " + args);
    }
}
class B extends A {
    constructor() {
        super(...arguments);
        print(...arguments);
    }
}
new B(1, 2, 3, 4, 5);

function func(a, b, ...args) {
    print(args);
}

func(1);
func(1, 2);
func(1, 2, 3);
func(1, 2, 3, 4, 5);