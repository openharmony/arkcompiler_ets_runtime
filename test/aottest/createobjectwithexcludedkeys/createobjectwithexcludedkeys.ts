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

// CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8
declare function assert_equal(a: Object, b: Object):void;
const v0 = {value : 'value', age: 23, name: 'name'};
const {value, age, ...v1} = v0;
assert_equal(value, 'value');
assert_equal(age, 23);
assert_equal(v1.value, undefined);
assert_equal(v1.age, undefined);
assert_equal(v1.name, 'name');
