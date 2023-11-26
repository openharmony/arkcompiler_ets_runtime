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

var a = "abc"
var b = "abcdefgh" + "tyuioplkj"
var c = "abcdefghtyuioplkj"
var d = "abcde" + "fghtyuioplkj"
var e = b.slice(0, 3);
var f = "诶比吸"
print(b == d)
print(b == c)
print(a == c)
print(a == b)
print(a == e)
print(b == e)
print(a == f)
print(f == f)
