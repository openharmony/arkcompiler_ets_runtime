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

/*
 * @tc.name:ecmastringtable
 * @tc.desc:test string
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
// Using to test the concat interface of EcmaStringTable about intern string and non-intern string
{
    let REGISTRY = Symbol();
    function System() {
      this[REGISTRY] = {};
    }
    function getOrCreateLoad(loader, id) {
      let load = loader[REGISTRY][id];
      print("id: " + id + " - load: "+ load);
      load = loader[REGISTRY][id] = {};
      return load;
    }

    // non-intern + non-intern
    let head1 = "no-".concat("schema:");
    let tail1 = "/src/".concat("xxx-js/instantiated-1af0bf5b.js");
    let key1 = head1 + tail1;
    let key2 = head1 + tail1;

    // intern + intern
    let key3 = "no-schema:/src/xxx-js/instantiation.js";
    let key4 = "no-schema:" + "/src/xxx-js/instantiation.js";

    // non-intern + intern
    let head2 = "no-".concat("schema:");
    let tail2 = "/src/xxx-js/cc.js";
    let key5 = "no-schema:" + "/src/xxx-js/cc.js";
    let key6 = head2 + tail2;

    // intern + non-intern
    let head3 = "no-schema:";
    let tail3 = "/src".concat("/instantiated-1af0bf5b.js");
    let key7 = "no-schema:" + "/src/instantiated-1af0bf5b.js";
    let key8 = head3 + tail3;

    // non-intern + non-intern
    let numhead1 = "12".concat("022");
    let numtail1 = "4".concat("21");
    let numkey1 = numhead1 + numtail1;
    let numkey2 = numhead1 + numtail1;

    // intern + intern
    let numkey3 = "123420002";
    let numkey4 = "12342" + "0002";

    // non-intern + intern
    let numhead2 = "90".concat("88");
    let numtail2 = "90";
    let numkey5 = "90" + "8890";
    let numkey6 = numhead2 + numtail2;

    // intern + non-intern
    let numhead3 = "90880";
    let numtail3 = "0".concat("0");
    let numkey7 = "90880" + "00";
    let numkey8 = numhead3 + numtail3;

    // intern + intern
    let numkey9 = "999999999";
    let numkey10 = "9999".concat("99999");

    // intern + intern
    let numkey11 = "1000000000";
    let numkey12 = "10".concat("00000000");

    // intern + intern
    let numkey13 = "0100000";
    let numkey14 = "010".concat("0000");

    let keyArray = [key1, key3, key5, key7, numkey1, numkey3, numkey5, numkey7, numkey9, numkey11, numkey13];
    let system = new System();
    for (let i = 0; i < keyArray.length; i++) {
        getOrCreateLoad(system, keyArray[i]);
    }

    print("key1 === key2: ", key1 === key2);
    print("key3 === key4: ", key3 === key4);
    print("key5 === key6: ", key5 === key6);
    print("key7 === key8: ", key7 === key8);
    getOrCreateLoad(system, key2);
    getOrCreateLoad(system, key4);
    getOrCreateLoad(system, key6);
    getOrCreateLoad(system, key8);

    print("numkey1 === numkey2: ", numkey1 === numkey2);
    print("numkey3 === numkey4: ", numkey3 === numkey4);
    print("numkey5 === numkey6: ", numkey5 === numkey6);
    print("numkey7 === numkey8: ", numkey7 === numkey8);
    print("numkey9 === numkey10: ", numkey9 === numkey10);
    print("numkey11 === numkey12: ", numkey11 === numkey12);
    getOrCreateLoad(system, numkey2);
    getOrCreateLoad(system, numkey4);
    getOrCreateLoad(system, numkey6);
    getOrCreateLoad(system, numkey8);
    getOrCreateLoad(system, numkey10);
    getOrCreateLoad(system, numkey12);
}

{
    let obj = {
        "123一二三": "value 123一二三",
        "1一2二3三": "value 1一2二3三",
        "123456": "value 123456",
    }
    {
        let key1 = "123一二三";
        let tmp1 = "123";
        let tmp2 = "一二三";
        let key2 = tmp1 + tmp2;
        let res1 = obj[key1];
        let res2 = obj[key2]
        print("res:", res1 === res2)
    }
    {
        let key1 = "1一2二3三";
        let tmp1 = "1一2";
        let tmp2 = "二3三";
        let key2 = tmp1 + tmp2;
        let res1 = obj[key1];
        let res2 = obj[key2]
        print("res:", res1 === res2)
    }
    {
        let key1 = "123一二三";
        let tmp1 = "123";
        let tmp2 = "123";
        let key2 = tmp1 + tmp2;
        let res1 = obj[key1];
        let res2 = obj[key2]
        print("res:", res1 !== res2)
    }
    {
        let key1 = "123456";
        let tmp1 = "123";
        let tmp2 = "456";
        let key2 = tmp1 + tmp2;
        let res1 = obj[key1];
        let res2 = obj[key2]
        print("res:", res1 === res2)
    }
    {
        let key1 = "123456";
        let tmp1 = "一二三";
        let tmp2 = "456";
        let key2 = tmp1 + tmp2;
        let res1 = obj[key1];
        let res2 = obj[key2]
        print("res:", res1 !== res2)
    }
}

{
    var x = [];
    x[42949672964] = 1; // insert string to EcmaStringTable in ObjectOperator::HandleKey.
    print(x["42949672964"]);

    var x = [];
    x[12345.6789] = 2; // insert string to EcmaStringTable in ObjectOperator::HandleKey.
    print(x["12345.6789"]);

    let obj = {
        "123456234": "value 123456",
    }
    let key = "123456234";
    print(obj[key]);

}
