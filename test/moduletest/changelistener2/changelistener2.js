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

{
    var int_arr = new Int16Array(2);
    for (let i = 0; i < 10000; i++) {}
    var a = int_arr.findIndex;
    var arr1 = new Array(1);
    int_arr.__proto__ = arr1;
    var arr = new Array(1);
    Int16Array.prototype.__proto__ = arr;
    arr.__proto__ = int_arr;
    arr["a"] = 7;
}
