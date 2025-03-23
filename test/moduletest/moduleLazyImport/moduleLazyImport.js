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
 * @tc.name:importJson
 * @tc.desc:test import Json
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
import lazy {h} from './H'
import lazy {A} from "./A"
import { aaa } from "./E"
import lazy {func1} from './F'
import lazy {dynamicLazySequence} from './dynamicLazySequence'
import lazy {dynamicLazySequence2} from './dynamicLazySequence2'
import lazy {nestTest} from './lazyFileWithDynImport'

print("this is entry");
func1();
let myClass = new A();
myClass.myMethod();
aaa();
print(h);
import("./dynamicLazySequence").then((ns) => {
    print(ns.dynamicLazySequence);
})
import("./dynamicLazySequence2");
print(nestTest);