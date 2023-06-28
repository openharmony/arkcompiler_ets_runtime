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
 * @tc.name:errorcause
 * @tc.desc:test Error Constructors with cause
 * @tc.type: FUNC
 * @tc.require: issueI7F4Y5
 */
[
    Error,
    EvalError,
    RangeError,
    ReferenceError,
    SyntaxError,
    TypeError,
    URIError,
].forEach(function (ctor, i) {
    if (testErrorCause(ctor)) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function testErrorCause(ctor) {
    try {
        let err = new ctor("message", { cause: "error cause" });
        throw err;
    } catch (e) {
        if (e.cause == "error cause") {
            return true;
        } else {
            return false;
        }
    }
}

let err2 = new AggregateError([], "message", { cause: "error cause" });
try {
    throw err2;
} catch (e) {
    if (e.cause == "error cause") {
        print(e.name + " test success !!!")
    } else {
        print(e.name + " test fail !!!")
    }
}