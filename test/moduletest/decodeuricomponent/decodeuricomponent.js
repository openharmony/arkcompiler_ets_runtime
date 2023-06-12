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
 * @tc.name:decodeuricomponent
 * @tc.desc:test decodeuricomponent
 * @tc.type: FUNC
 * @tc.require: issueI7CTF7
 */

let uri="%c2%aa%66%55%58%c2%83%c2%93%00%c2%89%c3%96%08%58%c2%b4%c3%bd%46";
let uri_encode=decodeURIComponent(uri);
print(encodeURIComponent(uri_encode));

let uri1="%00";
let uri_encode1=decodeURIComponent(uri1);
print(encodeURIComponent(uri_encode1));