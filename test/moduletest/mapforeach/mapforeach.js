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

class Person {
    name = 'John';
    age = 28;
    height = 178.3;
}

let map = new Map();
map.set(0, 41);
map.set('key', 'value');
map.set('object', new Person());


function PrintElems(v, k) {
    print('k:', k, 'v:', JSON.stringify(v))
}
map.forEach(PrintElems);

try {
    let nonCallable = new Person();
    map.forEach(nonCallable);
} catch (error) {
    print(error);
}
