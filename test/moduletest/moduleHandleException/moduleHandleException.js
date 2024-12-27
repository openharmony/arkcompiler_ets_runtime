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
 * @tc.name:moduleHandleException
 * @tc.desc:moduleHandleException
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

import('./B.js').then((m) => {
    print("import success 1");
    m.b;
}).catch((e) => {
    print("first, " + e.toString());
})

// test same file
import('./B.js').then((m) => {
    print("import success 2");
    m.b;
}).catch((e) => {
    print("same file, " + e.toString());
})

// test child file
import('./A.js').then((m) => {
    print("import success 3");
    m.a;
}).catch((e) => {
    print("child file, " + e.toString());
})

// test parent file
import('./C.js').then((m) => {
    print("import success 4");
    m.a;
}).catch((e) => {
    print("parent file, " + e.toString());
})

// test other file
import('./E.js').then((m) => {
    print("import success 5");
    m.d;
}).catch((e) => {
    print("other file, " + e.toString());
})

// shared module first
import('./importShared1.js').then((m) => {
    print("import success 6");
    m.importShared1;
}).catch((e) => {
    print("shared module first import, " + e.toString());
})

// shared module get stored error
import('./importShared2.js').then((m) => {
    print("import success 7");
    m.fooA();
}).catch((e) => {
    print("shared module, " + e.toString());
})

// shared module get child error
import('./importSharedChild.js').then((m) => {
    print("import success 8");
    m.fooA();
}).catch((e) => {
    print("shared module child, " + e.toString());
})

// multi level
import('./multiLevel1.js').then((m) => {
    print("import success 9");
    m.mulA();
}).catch((e) => {
    print("multi level, " + e.toString());
})