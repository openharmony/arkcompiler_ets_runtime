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
const user = {value:undefined, done:true};

// Array iterator.
const arrayResult = (new Array())[Symbol.iterator]().next();
print(ArkTools.haveSameMap(user, arrayResult));

// Map iterator.
const mapResult = (new Map())[Symbol.iterator]().next();
print(ArkTools.haveSameMap(user, mapResult));

// Set iterator.
const setResult = (new Set())[Symbol.iterator]().next();
print(ArkTools.haveSameMap(user, setResult));

// Generator.
function* generator() {}
const generatorResult = generator().next();
print(ArkTools.haveSameMap(user, setResult));
