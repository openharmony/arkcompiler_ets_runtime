/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

/*
 * @tc.name:finalizationregistrywithsymbol
 * @tc.desc:test FinalizationRegistry allow the use of most Symbols
 * @tc.type: FUNC
 * @tc.require: issueI7J2VN
 */

var fr = new FinalizationRegistry((value) => {
    assert_equal(value + " destroyed","symbol 2 destroyed");
})

let s = Symbol("symbol");
let ss = Symbol("symbol");
fr.register(s, "symbol 1", Symbol.match);
fr.register(ss, "symbol 2", Symbol.matchAll);

try {
    fr.register(Symbol.for("symbol2"), "symbol 2");
} catch (err) {
    assert_equal(err instanceof TypeError, true);
}

fr.unregister(Symbol.match);
s = {};
ss = {};
ArkTools.forceFullGC();

test_end();