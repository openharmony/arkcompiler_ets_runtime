/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.name:wrapperclassfunc
 * @tc.desc:test wrapper
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
(() => {
	let value1 = "";
	let value2 = "";
    function WrapperFunc(propertyDesc) {
        const method = propertyDesc.value
        propertyDesc.value = () => {
            value1 = "wrapper call";
            method()
        }
    }
    function decorate(obj, key) {
        var method = Object.getOwnPropertyDescriptor(obj, key)
        WrapperFunc(method)
        Object.defineProperty(obj, key, method)
    }
    class RawClass {
        printString() {
            value2 = "raw function";
        }
    }
    decorate(RawClass.prototype, "printString")
    globalThis.cls = new RawClass()
    globalThis.cls.printString()
    
    assert_equal(value1,"wrapper call");
    assert_equal(value2,"raw function");
    
    test_end();
})()