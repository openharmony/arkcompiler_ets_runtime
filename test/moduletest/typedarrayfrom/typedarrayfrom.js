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

/*
 * @tc.name:typedarrayfrom
 * @tc.desc:test TypedArray.from
 * @tc.type: FUNC
 */

var arr=new Int8Array();
let it = arr[Symbol.iterator]();
let original_next=it.next;
it.__proto__["next"]=function (){
    print("get value");
    return {value:undefined,done:true};
}
var fromArr=Int8Array.from(arr);
print(fromArr.length)