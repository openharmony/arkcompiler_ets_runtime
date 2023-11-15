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

var a = 2;
print(a != null);
print(null != a);
print(a != undefined);
print(undefined != a);

var b = null;
print(b != null);
print(null != b);
print(b != undefined);
print(undefined != b);

var c = undefined;
print(c != null);
print(null != c);
print(c != undefined);
print(undefined != c);

var d = 2;
print(d !== null);
print(null !== d);
print(d !== undefined);
print(undefined !== d);

var e = null;
print(e !== null);
print(null !== e);
print(e !== undefined);
print(undefined !== e);

var f = undefined;
print(f !== null);
print(null !== f);
print(f !== undefined);
print(undefined !== f);