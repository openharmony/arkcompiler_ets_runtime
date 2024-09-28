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
(function returnFalsishStrict() {
  "use strict";

  function trySet(o) {
    o["bla"] = 0;
  }

  var proxy = new Proxy({}, {});
  var proxy2 = new Proxy({}, { set() { return ""; } });

  trySet(proxy);
  trySet(proxy);
  try {
    trySet(proxy2)
  } catch (error) {
    print(error instanceof TypeError);
  }
})();

const options = { cause: 'a cause' };
const proxy = new Proxy(options, { get: () => 'proxied cause'});
const err = Error('message', proxy);
print(err.cause);
