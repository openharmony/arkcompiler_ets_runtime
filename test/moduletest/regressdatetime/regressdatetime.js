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
() => (new Intl.DateTimeFormat("en", {fractionalSecondDigits: 0}));

() => (new Intl.DateTimeFormat("en", {fractionalSecondDigits: 4}));

print((new Intl.DateTimeFormat("en", {fractionalSecondDigits: 1}))
.resolvedOptions().fractionalSecondDigits);

print((new Intl.DateTimeFormat("en", {fractionalSecondDigits: 2}))
.resolvedOptions().fractionalSecondDigits);

print((new Intl.DateTimeFormat("en", {fractionalSecondDigits: 3}))
.resolvedOptions().fractionalSecondDigits);

print((new Intl.DateTimeFormat()).resolvedOptions().fractionalSecondDigits);

print((new Intl.DateTimeFormat("en", {fractionalSecondDigits: undefined}))
.resolvedOptions().fractionalSecondDigits);

() => (new Intl.DateTimeFormat(
  "en", {timeStyle: "short", fractionalSecondDigits: 3}));

() => (new Intl.DateTimeFormat(
  "en", {dateStyle: "short", fractionalSecondDigits: 3}));