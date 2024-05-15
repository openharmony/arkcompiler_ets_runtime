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

function t1() {
    return function t2(obj) {
        print(obj.x);
    };
}

function foo() {
    let f = t1();
    let obj = {x : 123};
    f(obj);
}

foo();

var makeAuthorRepo = function (dep) {
    var author = dep.author;
    return {
        get author() {
            return author;
        },
    };
};
var repo1 = makeAuthorRepo({
    author: {
        name: 'prop1'
    }
});
var repo2 = makeAuthorRepo({
    author: {
        name: 'prop2'
    }
});
print('repo1 ' + repo1.author.name);