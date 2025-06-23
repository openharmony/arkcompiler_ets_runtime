/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

const objA = {
    toString() {
        return './A.js';
    }
};

const objB = {
    toString() {
        return './B.js';
    }
};

function dynamic_import_context1() {
    import(objA).then(moduleValue => {
        print(moduleValue.testArray.length);
        moduleValue.testArray.push(1);
        print(moduleValue.testArray.length);
    })
}

function dynamic_import_context2() {
    import(objB).then(moduleValue => {
        print(moduleValue.testArray.length);
    })
}

dynamic_import_context1();
dynamic_import_context2();
