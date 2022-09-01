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

function* zip(a, b) {
    print(a.length === b.length)
    for (let i = 0; i < a.length; ++i) {
      yield [i, a[i], b[i]];
    }
  }
  
  function compare(actual, expected, message) {
    for (const [i, actualEntry, expectedEntry] of zip(actual, expected)) {
      print(actualEntry.type === expectedEntry.type);
      print(actualEntry.value === expectedEntry.value);
      print(actualEntry.source === expectedEntry.source);
    }
  }
  
  // Switch to a time format instead of using DateTimeFormat's default date-only format.
  const dtf = new Intl.DateTimeFormat(undefined, {
      hour: "numeric", minute: "numeric", second: "numeric"
  });
  const date = Date.now();
  const expected = dtf.formatRangeToParts(0, date);
  
  compare(dtf.formatRangeToParts(-0.9, date), expected, "formatRangeToParts(-0.9)");