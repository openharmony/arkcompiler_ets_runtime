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

let arr = Array.from("abcd");
print(arr);
arr = Array.from("abcd");
print(arr);
arr[1] = 'e';
print(arr);
arr = Array.from("abcd");
print(arr);

arr = Array.from("01234567890123456789012");
print(arr)
arr = Array.from("方舟")
print(arr);
arr = Array.from("方舟")
print(arr);
arr = Array.from("")
print(arr.length)
arr[0] = 'a'
arr = Array.from("")
print(arr.length)

var src = new Uint8Array(10000);
for(var i = 0; i < 10000; i++)
{
    src[i] = 1;
}
arr = Array.from(src);
print(arr[666]);
print(arr[999]);
print(arr.length);