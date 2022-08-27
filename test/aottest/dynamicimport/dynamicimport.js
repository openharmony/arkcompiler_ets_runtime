/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

import(objA).then(animalA => {
    print('full name : ', animalA.name, ' ', animalA.cognomen);
    print(animalA.familyCrets);
}).then().catch();

async function f() {
    var animalB = await import(objB);
    print('full name : ', animalB.name, ' ', animalB.cognomen);
    print(animalB.familyCrets);
}

f().then().catch();