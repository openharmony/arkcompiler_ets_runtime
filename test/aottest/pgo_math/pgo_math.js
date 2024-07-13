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

function foo1() {
    const v = Math.acos();
    print(Math.min(v));
}

function foo2() {
    const v = Math.abs();
    print(Math.min(v));
}

function foo3() {
    const v = Math.pow();
    print(Math.min(v));
}

function foo4() {
    const v = Math.atan2();
    print(Math.min(v));
}

foo1();
foo2();
foo3();
foo4();