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
print(-0 == parseInt("-0"))

print(isNaN(parseInt('0x  ')));

print(isNaN(parseInt('0x  ', 16)));

let result = null;
let str = { toString: function() { result = "InvalidString"; throw null; } };
let radix = { valueOf: function() { result = "InvalidRadix"; throw null; } };
try { parseInt(str, radix); } catch (e) {}
print(result);

print(parseInt('4',16))
print(parseInt('a',16))
print(parseInt('a',26))
print(parseInt('a',36))
print(parseInt('a'))
print(parseInt('0xa'))
print(parseInt('123'))
print(parseInt('0x123'))