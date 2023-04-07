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

// The following test case once exposed a bug: For the same class type, the function TSManager::RecursiveGenTSHClass may
// be entered duplicatly, which causes the actual class prototype in runtime is the last time created, but the owner of
// vtable does not be updated.

declare function print(arg:any):string;

class C {
    constructor() {}

    foo():string {
        return "foo"
    }
}

class D extends C {
    constructor() {
        super();
    }
}

// In general test cases we use let to declare class instances, while using var, the order of parsing class type is
// different(due to variable hoisting), child class firstly, and parent class nextly, which exposed the bug.
var d = new D();
print(d.foo());
