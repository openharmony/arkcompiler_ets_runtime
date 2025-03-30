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

async function f(number) {
  let module = await import('./test.js');
  assert_equal(module.name + number, 'Tyrion' + number);
  markModuleCollectable(module);
}

function deregister (){
  return new Promise((resolve, reject)=> {
    resolve("done");
  })
}

deregister().then(r=>{
  f(1).then(()=>{ // test for esm functioned well when second loading.
    assert_equal("second load","second load");
    ArkTools.forceFullGC();
    f(2); // test whether the module has been successfully uninstalled.
  });
  assert_equal(r,done);});

test_end();