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
 * @tc.name:regexp
 * @tc.desc:test Regexp
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

{
  let reg= /ab|cd||/
  assert_equal(JSON.stringify(reg.exec("cd")),'["cd"]')
}
{
  let reg= /ab||/
  assert_equal(JSON.stringify(reg.exec("cd")),'[""]')
}
{
  let reg= /ab|cd|ef/
  assert_equal(JSON.stringify(reg.exec("cd")),'["cd"]')
}

{
  let str = "😀";
  let regexp = /[😀]/;
  assert_equal(JSON.stringify(str.replace(regexp,"b")),JSON.stringify("b\ude00"));
}
{
  let str = "😀";
  let regexp = /[😀]/g;
  assert_equal(JSON.stringify(str.replace(regexp,"b")),JSON.stringify("bb"));
}
{
  let str = "😀";
  let regexp = /[😀]/u;
  assert_equal(JSON.stringify(str.replace(regexp,"b")),JSON.stringify("b"));
}
{
  let str = "😀";
  let regexp = /[\😀]/;
  assert_equal(JSON.stringify(str.replace(regexp,"b")),JSON.stringify("b\ude00"));
}


var reg = /[\x5d-\x7e]/i;
var result = reg.test("a");
assert_equal(result,true);

var reg1 = new RegExp("^[-+]?([0-9]+)?(\\٫[0-9]{1,})?$");
var result1 = reg1.test('0٫0000000000001');
assert_equal(result1,true);

var reg2 = /^[Α-ώ]+$/i
assert_equal(reg2.test('άέήίΰϊϋόύώ'),true);

assert_equal(reg2.test('ΆΈΉΊΪΫΎΏ'),true);

assert_equal(reg2.test('αβγδεζηθικλμνξοπρςστυφχψω'),true);

assert_equal(reg2.test('ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ'),true);

let reg3 =/^[A-Z0-9_\-]*$/i
assert_equal(reg3.test(''),true)

let reg4 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>https?[:：]//[^/]+/svn(?:/[a-z0-9.,;?'*:+&%$#=~_ \\u4E00-\\u9FA5-]*)*).*$", "i")
assert_equal(reg4.test('a'),false);

let reg5 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>(?:(?:ht|f)tps?[:：]//)?(?:[a-z0-9-]+\\.)+" + "(?:com|edu|gov|mil|net|org|biz|info|name|museum|us|ca|uk|cn|cc|tw|de|au|sg|hk|ei|fr|me|im)(?![a-z])" + "(?:\\:[0-9][0-9]*)?(?:\\.?/[a-z0-9.,;?'\\|*:\\\\+&%$#=~_-]*)*).*$", "i")
assert_equal(reg5.test('a'),false);

let reg6 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>(?:ht|f)tps?[:：]//(?:[a-z0-9-]+\\.)*[a-z0-9-]+(?:/[a-z0-9]+)*[/a-z0-9.,;?'\\|*:\\\\+&%$#=~_-]*).*$", "i")
assert_equal(reg6.test('a'),false);

let reg7 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>(?:https?[:：]//)?(?:[a-z0-9-\\\\]+\\.)+" + "(?:com|edu|gov|mil|net|org|biz|info|name|museum|us|ca|uk|cn|cc|tw|de|au|sg|hk|ei|fr|me|im)(?![a-z])" + "(?:\\:\\d{4})?(?:/[a-z0-9.,;?'\\|*:\\\\+&%$#=~_-]*)*\\?(?:[a-z0-9]*=[a-z0-9.,;?'*:+%$#=~_\\u4E00-\\u9FA5-]*&?)*).*$", "i")
assert_equal(reg7.test('a'),false);

let arr = []
let temp = true
var quotedEmailUserUtf8 = /^([\s\x01-\x08\x0b\x0c\x0e-\x1f\x7f\x21\x23-\x5b\x5d-\x7e\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF]|(\\[\x01-\x09\x0b\x0c\x0d-\x7f\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF]))*$/i;
arr.push(quotedEmailUserUtf8.test(" foo m端ller "))

let reg8 = /^[A-ZÃÁÀÂÄÇÉÊËÍÏÕÓÔÖÚÜ]+$/i
arr.push(reg8.test('palíndromo'))
arr.push(reg8.test('órgão'))
arr.push(reg8.test('qwértyúão'))
arr.push(reg8.test('àäãcëüïÄÏÜ'))

let reg9 = /^[A-ZÀÉÈÌÎÓÒÙ]+$/i
arr.push(reg9.test('àéèìîóòù'))
arr.push(reg9.test('metró'))
arr.push(reg9.test('pèsca'))
arr.push(reg9.test('genî'))

let reg10 = /^[A-ZÀÁẠẢÃÂẦẤẬẨẪĂẰẮẶẲẴĐÈÉẸẺẼÊỀẾỆỂỄÌÍỊỈĨÒÓỌỎÕÔỒỐỘỔỖƠỜỚỢỞỠÙÚỤỦŨƯỪỨỰỬỮỲÝỴỶỸ]+$/i
arr.push(reg10.test('thiến'))
arr.push(reg10.test('nghiêng'))
arr.push(reg10.test('chào'))
arr.push(reg10.test('thế'))
arr.push(reg10.test('giới'))

let reg11 = /^[A-ZÅÄÖ]+$/i
arr.push(reg11.test('äiti'))

let reg12 = /^[A-ZÆØÅ]+$/i
arr.push(reg12.test('aøå'))

let reg13 = /^[A-ZĄĆĘŚŁŃÓŻŹ]+$/i
arr.push(reg13.test('kreską'))
arr.push(reg13.test('zamknięte'))
arr.push(reg13.test('zwykłe'))
arr.push(reg13.test('kropką'))
arr.push(reg13.test('przyjęły'))
arr.push(reg13.test('święty'))
arr.push(reg13.test('Pozwól'))

let reg14 = /^[А-ЯЂЈЉЊЋЏ]+$/i
arr.push(reg14.test('ШћжЂљЕ'))

let reg15 = /^[A-ZČĆŽŠĐ]+$/i
arr.push(reg15.test('ŠAabčšđćž'))
arr.push(reg15.test('ŠATROĆčđš'))

let reg16 = /^[A-ZÁÉÍÑÓÚÜ]+$/i
arr.push(reg16.test('ábcó'))
arr.push(reg16.test('dormís'))
arr.push(reg16.test('volvés'))
arr.push(reg16.test('español'))

let reg17 = /^[A-ZÅÄÖ]+$/i
arr.push(reg17.test('religiös'))
arr.push(reg17.test('stjäla'))
arr.push(reg17.test('västgöte'))

let reg18 = /^[A-ZÇĞİıÖŞÜ]+$/i
arr.push(reg18.test('AİıÖöÇçŞşĞğÜüZ'))

let reg19 = /^[Α-ώ]+$/i
arr.push(reg19.test('άέήίΰϊϋόύώ'))
arr.push(reg19.test('ΆΈΉΊΪΫΎΏ'))

let reg20 = /^[0-9A-VXYZÇƏĞİıÖŞÜ]+$/i
arr.push(reg20.test('Azərbaycan'))
arr.push(reg20.test('abcç2'))
arr.push(reg20.test('3kərə4kərə'))

let reg21 = /^[0-9А-Я]+$/i
arr.push(reg21.test('абв1'))
arr.push(reg21.test('жаба'))
arr.push(reg21.test('яГоДа2'))
arr.push(reg21.test('йЮя'))

let reg22 = /^[0-9A-ZÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ]+$/i
arr.push(reg22.test('řiť123'))

let reg23 = /^[0-9A-ZÁČĎÉÍŇÓŠŤÚÝŽĹŔĽÄÔ]+$/i
arr.push(reg23.test('1môj'))
arr.push(reg23.test('2ľúbím'))
arr.push(reg23.test('3mäkčeň'))
arr.push(reg23.test('5vŕba'))
arr.push(reg23.test('6ňorimberk'))
arr.push(reg23.test('7ťava'))
arr.push(reg23.test('8žanéta'))
arr.push(reg23.test('9Ďábelské'))
arr.push(reg23.test('10ódy'))

let reg24 = /^[0-9A-ZÁÉËÏÓÖÜÚ]+$/i
arr.push(reg24.test('Kán123'))
arr.push(reg24.test('één354'))

let reg25 = /^[0-9A-ZÅÄÖ]+$/i
arr.push(reg25.test('äiti124'))
arr.push(reg25.test('451åå23'))

let reg26 = /^[0-9A-ZÄÖÜß]+$/i
arr.push(reg26.test('äbc123'))

let reg27 = /^[0-9A-ZÁÉÍÓÖŐÚÜŰ]+$/i
arr.push(reg27.test('0árvíztűrőtükörfúrógép123'))

let reg28 = /^[0-9A-ZÃÁÀÂÄÇÉÊËÍÏÕÓÔÖÚÜ]+$/i
arr.push(reg28.test('palíndromo'))
arr.push(reg28.test('2órgão'))
arr.push(reg28.test('qwértyúão9'))
arr.push(reg28.test('àäãcë4üïÄÏÜ'))

let reg29 = /^[0-9A-ZÀÉÈÌÎÓÒÙ]+$/i
arr.push(reg29.test('123àéèìîóòù'))
arr.push(reg29.test('met23ró'))
arr.push(reg29.test('pès56ca'))
arr.push(reg29.test('gen45î'))

let reg30 = /^[0-9A-ZÁÉÍÑÓÚÜ]+$/i
arr.push(reg30.test('ábcó123'))

let reg31 = /^[0-9A-ZÀÁẠẢÃÂẦẤẬẨẪĂẰẮẶẲẴĐÈÉẸẺẼÊỀẾỆỂỄÌÍỊỈĨÒÓỌỎÕÔỒỐỘỔỖƠỜỚỢỞỠÙÚỤỦŨƯỪỨỰỬỮỲÝỴỶỸ]+$/i
arr.push(reg31.test('Thầy3'))
arr.push(reg31.test('3Gà'))

let reg32 = /^[0-9A-ZĄĆĘŚŁŃÓŻŹ]+$/i
arr.push(reg32.test('kre123ską'))
arr.push(reg32.test('zam21knięte'))
arr.push(reg32.test('zw23ykłe'))
arr.push(reg32.test('prz23yjęły'))
arr.push(reg32.test('świ23ęty'))
arr.push(reg32.test('Poz1322wól'))

let reg33 = /^[0-9А-ЯЂЈЉЊЋЏ]+$/i
arr.push(reg33.test('ШћжЂљЕ123'))

let reg34 = /^[0-9A-ZČĆŽŠĐ]+$/i
arr.push(reg34.test('ŠAabčšđćž123'))
arr.push(reg34.test('ŠATRO11Ćčđš'))

let reg35 = /^[0-9A-ZÅÄÖ]+$/i
arr.push(reg35.test('religiös13'))
arr.push(reg35.test('st23jäla'))
arr.push(reg35.test('västgöte123'))

let reg36 = /^[0-9A-ZÇĞİıÖŞÜ]+$/i
arr.push(reg36.test('AİıÖöÇçŞşĞğÜüZ123'))

let reg37 = new RegExp("^[-+]?([0-9]+)?(\\٫[0-9]{1,})?$")
arr.push(reg37.test('0٫0000000000001'))

let reg38 = new RegExp("^(?:[-+])?(?:[0-9]+)?(?:\\٫[0-9]*)?(?:[eE][\\+\\-]?(?:[0-9]+))?$")
arr.push(reg38.test('123٫'))
arr.push(reg38.test('123٫123'))
arr.push(reg38.test('-123٫123'))

let reg39 =/^[A-Z0-9_\-]*$/i
arr.push(reg39.test(''))

let reg40 = RegExp("^(?!-? )(?=.*\\d)(\\¥)?-?(0|[1-9]\\d|[1-9]\\d{0,2}(\\,\\d{3})*)?(\\.(\\d{2}))?$")
arr.push(reg40.test('¥6,954,231'))
arr.push(reg40.test('¥-6,954,231'))

var reg41 = /^[A-VXYZÇƏĞİıÖŞÜ]+$/i;
arr.push(reg41.test('Azərbaycan'))
arr.push(reg41.test('üöğıəçş'))
arr.push(reg41.test('sizAzərbaycanlaşdırılmışlardansınızmı'))
arr.push(reg41.test('dahaBirDüzgünString'))
arr.push(reg41.test('abcçdeəfgğhxıijkqlmnoöprsştuüvyz'))

let reg42 = /^[А-Я]+$/i
arr.push(reg42.test('абв'))
arr.push(reg42.test('жаба'))
arr.push(reg42.test('яГоДа'))

let reg43 = /^[A-ZÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ]+$/i
arr.push(reg43.test('žluťoučký'))
arr.push(reg43.test('Pěl'))
arr.push(reg43.test('Ďábelské'))
arr.push(reg43.test('ódy'))

let reg44 = /^[A-ZÁČĎÉÍŇÓŠŤÚÝŽĹŔĽÄÔ]+$/i
arr.push(reg44.test('môj'))
arr.push(reg44.test('ľúbím'))
arr.push(reg44.test('mäkčeň'))
arr.push(reg44.test('vŕba'))
arr.push(reg44.test('ňorimberk'))

let reg45 = /^[A-ZÆØÅ]+$/i
arr.push(reg45.test('aøå'))

let reg46 = /^[A-ZÁÉËÏÓÖÜÚ]+$/i
arr.push(reg46.test('Kán'))
arr.push(reg46.test('één'))
arr.push(reg46.test('vóór'))
arr.push(reg46.test('nú'))
arr.push(reg46.test('héél'))

let reg47 = /^[A-ZÄÖÜß]+$/i
arr.push(reg47.test('äbc'))
arr.push(reg47.test('FöÖbär'))

let reg48 = /^[A-ZÁÉÍÓÖŐÚÜŰ]+$/i
arr.push(reg48.test('árvíztűrőtükörfúrógép'))

arr.forEach((item)=>{
  if(!item){
    temp = false
  }
})
assert_equal(temp,true)

let arr1 = []
let temp1 = false
let reg49 = /[^A-Z0-9+\/=]/i;
arr1.push(reg49.test("Zg=="));
arr1.push(reg49.test("Zm8="));
arr1.push(reg49.test("Zm9v"));
arr1.push(reg49.test("Zm9vYg=="));
arr1.push(reg49.test("Zm9vYmE="));
arr1.push(reg49.test("Zm9vYmFy"));
arr1.push(reg49.test(
  "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4="
));
arr1.push(reg49.test("Vml2YW11cyBmZXJtZW50dW0gc2VtcGVyIHBvcnRhLg=="));
arr1.push(reg49.test("U3VzcGVuZGlzc2UgbGVjdHVzIGxlbw=="));
arr1.forEach((item)=>{
  if(item){
    temp1 = true
  }
})
assert_equal(temp1,false)
let str1 = 'SC52BAHL01031234567890123456USD'
assert_equal(str1.replace(/[^A-Z0-9]+/gi, ''),"SC52BAHL01031234567890123456USD")

let reg50 = /^[ABCEGHJKLMNPRSTVXY]\d[ABCEGHJ-NPRSTV-Z][\s-]?\d[ABCEGHJ-NPRSTV-Z]\d$/i

let regabc = /abc/g;
let strabcd = "abcdabcdabcd";
let exceptArr = [true,true,true,false,true,true,true,false,true,true]
let resultArr = []
for (let i = 0; i < 10; i++) {
  // cache is used in this case
  resultArr.push(regabc.test(strabcd))
}
assert_equal(exceptArr,resultArr);

let str2 = "aaaabbBbcccC";
for (let i = 0; i < 2; i++) {
  assert_equal(str2,'aaaabbBbcccC');
  let t1 = str2.replace(/([A-Z])/g, function(e) {
    return "_" + e;
  });
  assert_equal(t1,'aaaabb_Bbccc_C');
  let t2 = str2.replace(/([A-Z])/g, "_$1");
  assert_equal(t2,'aaaabb_Bbccc_C');
  assert_equal(t1.replace(/([a-z]+)/g, "_xy"),'_xy_B_xy_C');
  assert_equal(t2.replace(/_/g, ""),'aaaabbBbcccC');
}

// regexp cache test
let mediaReg = "\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)";
let string = '(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s';
const regex1 = new RegExp(mediaReg);
let matchArray = string.match(regex1);
assert_equal(matchArray.toString(),'(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s,(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(,s,(,s,(,s,(,s,(,s,(,s,(,s,(,s');

// Test regexp.test fastpath
var protoExec = RegExp.prototype.exec
RegExp.prototype.exec = function () {
  return null
}
var reg = /a/
assert_equal(reg.test("aaaaa"),false);

delete RegExp.prototype.exec
assert_equal(reg.test("aaaaa"),true);

Object.prototype.exec = function () {
  return null
}
assert_equal(reg.test("aaaaa"),false);

delete Object.prototype.exec
RegExp.prototype.exec = protoExec
assert_equal(reg.test("aaaaa"),true);

var protoTest = RegExp.prototype.test
RegExp.prototype.test = function () {
    return false
}
var reg2 = /foo*/
assert_equal(reg2.test("fooooooo"),false);

RegExp.prototype.test = protoTest
assert_equal(reg2.test("fooooooo"),true);

// Same hash in cached result, but different flags.
var regexp1 = /a*/gs;
var regexp2 = /a*/g;
regexp2.lastIndex = 8;
assert_equal(regexp1.exec('aaa').toString(),'aaa');
assert_equal(regexp2.exec('aaa'),null);

// Same hash in cached result, and same flags, but different lastIndex.
var regexp3 = /a*/g;
var regexp4 = /a*/g;
regexp4.lastIndex = 1;
assert_equal(regexp3.exec('aaabab').toString(),'aaa');
assert_equal(regexp4.exec('aaabaa').toString(),'aa');

const v43 = /V[\d-\d]/ys;
const o54 = {
    __proto__: v43,
};
try {
    o54.test(Map);
    assert_unreachable();
} catch (error) {
  assert_equal("Caught an error: "+ error.message, "Caught an error: this does not have [[RegExpMatcher]]");
}

// Testing regexp.prototype.replace after optimization
{
  const re1 = /[Cz]/;
  const re2 = /[Cz]/g;
  const re3 = /([Cz])/;
  const re4 = /([Cz])/g;

  let replace_str = "abCdefgzabCdefgzabCdefgz";
  let replace_result = replace_str.replace(re1, "");
  assert_equal(re1.lastIndex,0);
  let cached_reuslt = replace_str.replace(re1, "");
  assert_equal(replace_result === cached_reuslt,true);
  assert_equal(re1.lastIndex,0);

  replace_result = replace_str.replace(re2, "xyz");
  assert_equal(re2.lastIndex,0);
  cached_reuslt = replace_str.replace(re2, "xyz");
  assert_equal(replace_result === cached_reuslt,true);
  assert_equal(re2.lastIndex,0);

  replace_result = replace_str.replace(re3, "x$1yz");
  assert_equal(re3.lastIndex,0);
  cached_reuslt = replace_str.replace(re3, "x$1yz");
  assert_equal(replace_result === cached_reuslt,true);
  assert_equal(re3.lastIndex,0);

  replace_result = replace_str.replace(re4, String);
  assert_equal(re4.lastIndex,0);
  cached_reuslt = replace_str.replace(re4, String);
  assert_equal(replace_result === cached_reuslt,true);
  assert_equal(re4.lastIndex,0);
}

// test RegExp.prototype.xx
assert_equal(RegExp.prototype.dotAll,undefined)
assert_equal(RegExp.prototype.global,undefined)
assert_equal(RegExp.prototype.hasIndices,undefined)
assert_equal(RegExp.prototype.ignoreCase,undefined)
assert_equal(RegExp.prototype.multiline,undefined)
assert_equal(RegExp.prototype.sticky,undefined)
assert_equal(RegExp.prototype.unicode,undefined)
assert_equal(RegExp.prototype.lastIndex,undefined)
assert_equal(RegExp.prototype.flags,"")
assert_equal(RegExp.prototype.source,"(?:)")
try {
    RegExp.prototype.test("abc")
    assert_unreachable();
} catch (error) {
  assert_equal(error instanceof TypeError,true)
}
try {
    RegExp.prototype.exec("abc")
    assert_unreachable();
} catch (error) {
  assert_equal(error instanceof TypeError,true)
}
assert_equal(RegExp.prototype.toString(),'/(?:)/')

let inputString = "/vedio/av{avid}{cid}";
let extractedContent = inputString.match(/\{([^{}]+)\}/g);
let replacedString = inputString.replace(/\{([^{}]+)\}/g, '(uuu)').replace(/\//g, "\\/");
assert_equal(replacedString,'\\/vedio\\/av(uuu)(uuu)');

let str = "beep boop   afff测试样本one1";
assert_equal(str.split(/([{}:;,]|\s+)/).toString(),'beep, ,boop,   ,afff测试样本one1');

function verifyRegExp(text) {
  text = text.replace(new RegExp('[\\s]', 'g'), ' ');
  let emRegExp = new RegExp('<em>.*?</em>', 'ig');
  let results = text.split(emRegExp);
  text.match(emRegExp);
  return results;
}

const srcTextA = '<em>a</em>bcdefghijklmnopqrstuvwxyz<em>a</em>bcdefghijklmnopqrstuvwxyz<em>a</em>bcdefghijklmnopqrstuvwxyz<em>a</em>bcdefghijklmnopqrstuvwxyz'
const srcTextAbcd = '<em>abcd</em>efghijklmnopqrstuvwxyz<em>abcd</em>efghijklmnopqrstuvwxyz<em>abcd</em>efghijklmnopqrstuvwxyz<em>abcd</em>efghijklmnopqrstuvwxyz'

var s =`verifyRegExp_1: ${verifyRegExp(srcTextA)}`
assert_equal(s,'verifyRegExp_1: ,bcdefghijklmnopqrstuvwxyz,bcdefghijklmnopqrstuvwxyz,bcdefghijklmnopqrstuvwxyz,bcdefghijklmnopqrstuvwxyz');
var s =`verifyRegExp_2: ${verifyRegExp(srcTextAbcd)}`
assert_equal(s,'verifyRegExp_2: ,efghijklmnopqrstuvwxyz,efghijklmnopqrstuvwxyz,efghijklmnopqrstuvwxyz,efghijklmnopqrstuvwxyz');
var s = `verifyRegExp_3: ${verifyRegExp(srcTextAbcd)}`
assert_equal(s,'verifyRegExp_3: ,efghijklmnopqrstuvwxyz,efghijklmnopqrstuvwxyz,efghijklmnopqrstuvwxyz,efghijklmnopqrstuvwxyz');

//regexp unicode property
const ans1 = /\p{Alphabetic}/u.test("æ");
const ans2 = /\p{Alpha}/u.test("2");
const ans3 = /^[\p{Lu}\p{Ll}]+$/u.test("ASDFasdf");
const ans4 = /^[\P{Lu}\p{Ll}]+$/u.test("ASDFasdf");
const ans5 = /\P{Ll}/iu.test("b");
const ans6 = /\p{Ll}/iu.test("b");
const ans7 = /[^\P{Any}]+/u.test(456);
const ans8 = /\p{Assigned}+/u.test("🄰🄱🄲");
const ans9 = /[\p{P}\p{S}]/u.test("!");
const ans10 = /\p{General_Category=Math_Symbol}/u.test("+");
assert_equal(ans1,true);
assert_equal(ans2,false);
assert_equal(ans3,true);
assert_equal(ans4,false);
assert_equal(ans5,true);
assert_equal(ans6,true);
assert_equal(ans7,true);
assert_equal(ans8,true);
assert_equal(ans9,true);
assert_equal(ans10,true);
try {
  const ans11 = RegExp("/[\\p{}]/u");
  assert_unreachable();
} catch(error) {
  assert_equal(error instanceof SyntaxError, true);
}
const str3 = "a-b-c";
const re = /-/y;
assert_equal(str3.split(re).toString(),'a,b,c');

re.lastIndex = 1;
assert_equal(str3.split(re).toString(),'a,b,c');

re.lastIndex = -1;
assert_equal(str3.split(re).toString(),'a,b,c');

re.lastIndex = 3;
assert_equal(str3.split(re).toString(),'a,b,c');

assert_equal(re.test(str3),true);

assert_equal(str3.split(/-/g).toString(),'a,b,c');

// search
const str4 = "abc";
let re1 = /b/;
re1.lastIndex = 2;
assert_equal(str4.search(re1),1);
assert_equal(str4.search(/b/y),-1);
assert_equal(str4.search(re1),1);
assert_equal(re1.lastIndex,2);

// check cache
const str5 = "a-bc";
let re2 = /-/;
re2.lastIndex = 2;
assert_equal(str5.split(re2).toString(),'a,bc');
assert_equal(re2.lastIndex,2);
assert_equal(str5.split(re2).toString(),'a,bc');
assert_equal(re2.lastIndex,2);

const str6 = "abcabc";
let re3 = /abc/;
re3.lastIndex = 2;
assert_equal(str6.match(re3).toString(),'abc');
assert_equal(re3.lastIndex,2);
assert_equal(str6.match(re3).toString(),'abc');
assert_equal(re3.lastIndex,2);

let re4 = /abc/g;
re4.lastIndex = 2;
assert_equal(str6.match(re4).toString(),'abc,abc');
assert_equal(re4.lastIndex,0);
assert_equal(str6.match(re4).toString(),'abc,abc');
assert_equal(re4.lastIndex,0);
Object.defineProperty(RegExp.prototype, "global", {
  value: true
})
var flags = RegExp.prototype.flags;
Object.defineProperty(RegExp.prototype, "global", {
  value: false
})
assert_equal(flags,"g");
assert_equal(str6.match(re4).toString(),'abc');
assert_equal(re4.lastIndex,3);
assert_equal(str6.match(re4).toString(),'abc');
assert_equal(re4.lastIndex,6);

let myExp = new RegExp("a+b+c");
Object.defineProperty(myExp, "sticky", {
    value: true
  })
assert_equal(myExp.toString(),'/a+b+c/y');

// Testing regexp.prototype.replace after optimization
{
  const re1 = /[Cz]/;
  const re2 = /[Cz]/g;
  const re3 = /([Cz])/;
  const re4 = /([Cz])/g;

  let replace_str = "abCdefgzabCdefgzabCdefgz";
  let replace_result = replace_str.replace(re1, "");
  assert_equal(re1.lastIndex,0);
  let cached_reuslt = replace_str.replace(re1, "");
  assert_equal(replace_result === cached_reuslt,true);
  assert_equal(re1.lastIndex,0);

  replace_result = replace_str.replace(re2, "xyz");
  assert_equal(re2.lastIndex,3);
  cached_reuslt = replace_str.replace(re2, "xyz");
  assert_equal(replace_result === cached_reuslt,false);
  assert_equal(re2.lastIndex,8);

  replace_result = replace_str.replace(re3, "x$1yz");
  assert_equal(re3.lastIndex,0);
  cached_reuslt = replace_str.replace(re3, "x$1yz");
  assert_equal(replace_result === cached_reuslt,true);
  assert_equal(re3.lastIndex,0);

  replace_result = replace_str.replace(re4, String);
  assert_equal(re4.lastIndex,3);
  cached_reuslt = replace_str.replace(re4, String);
  assert_equal(replace_result === cached_reuslt,true);
  assert_equal(re4.lastIndex,8);
}

const regex = /(?:)+/;
const str10 = "abcabcabc";
const matches = regex.test(str10);
assert_equal(matches,true);
const matches1 = regex.exec(str10);
assert_equal(matches1.toString(),"");

try {
  let matchReg = new RegExp("@【哈哈】*^o^*|@小米（kk）",'g');
  assert_unreachable();
} catch (error) {
  assert_equal("Caught an error: "+ error.message, "Caught an error: nothing to repeat");
}

let e = /./;
e.exec = function() {
    return [];
}
"".replace(e, "");
delete e.exec;

{
  const v0 = /qeu(?<a>.)\k<a>(x)(x)(x)\1*xyz{93}/ugysd;
  const v4 = typeof Date === "string";
  v0[Symbol.match] = v4;
  assert_equal(v0["exec"](),null);
}

{
  ("65535").split(/(?!\1(a\1)\1)\1/g);
  const o4 = {
    ...RegExp,
  };
  assert_equal(o4.toString(),"[object Object]");
}

{
  const v2 = /e\8Z(x)(x)(x)(x)(x)(x)(x)(x)(x)(x)\10*/misd;
  v2[1073741824] = -194290175n;
  for (let v3 = 0; v3 < 2; v3++) {
    assert_equal(v2.test(-194290175n),false);
  }
}

function f(...args) {
  return {};
}
let reg51 = new RegExp("a");
f.exec = f;
let relpfun = reg51[Symbol.replace];
relpfun.apply(f, [1, 2, 3, 4]);
var s = "success"
assert_equal(s,"success");

{
  let str = /^\s*([^;\s]*)/;
  str.test("text/html");
  assert_equal(RegExp.$1,'text/html');
  str.test("text/plain");
  assert_equal(RegExp.$1,'text/plain');
  str.test("text/html");
  assert_equal(RegExp.$1,'text/html');
}

{
  RegExp.prototype.test = function () {
    return false;
  };
  let str = /^\s*([^;\s]*)/;
  str.test("text/html");
  assert_equal(RegExp.$1,'text/html');
  str.test("text/plain");
  assert_equal(RegExp.$1,'text/html');
  str.test("text/html");
  assert_equal(RegExp.$1,'text/html');
  RegExp.prototype.test = protoTest
}

{
  let reg52 = /abc/;
  let count = 0;
  assert_equal(reg52.ignoreCase,false);
  assert_equal(reg52.global,false);
  assert_equal(reg52.flags,"");
  Object.defineProperty(reg52, "global", {
    get: function() { count++; return true; }
  });
  Object.defineProperty(reg52, "ignoreCase", {
    get: function() { count++; return true; }
  });

  assert_equal(reg52.ignoreCase,true);
  assert_equal(count,1);
  assert_equal(reg52.global,true);
  assert_equal(count,2);
  assert_equal(reg52.flags,"gi");
  assert_equal(count,4);
}

// test getFlags
Object.defineProperty(RegExp.prototype, "global", {
  value: true
})
const res = /abc/giymsud;
res.lastIndex = -1;
assert_equal(res.flags,'dgimsuy');
const res1 = /abc/g;
res1.lastIndex = -1;
assert_equal(res1.flags,'g');
const res2 = /abc/i;
res2.lastIndex = -1;
assert_equal(res2.flags,'gi');
const res3 = /abc/y;
res3.lastIndex = -1;
assert_equal(res3.flags,'gy');
const res4 = /abc/m;
res4.lastIndex = -1;
assert_equal(res4.flags,'gm');
const res5 = /abc/s;
res5.lastIndex = -1;
assert_equal(res5.flags,'gs');
const res6 = /abc/u;
res6.lastIndex = -1;
assert_equal(res6.flags,'gu');
const res7 = /abc/d;
res7.lastIndex = -1;
assert_equal(res7.flags,'dg');
const res8 = /abc/;
res8.lastIndex = -1;
assert_equal(res8.flags,'g');

{
  let str = "<style>test</style>"
  let reg = new RegExp("/");
  assert_equal(reg.source + ' ' + JSON.stringify(reg.exec(str)), '\\/ ["/"]');
  
  reg = new RegExp("\/[a-z]*>");
  assert_equal(reg.source + ' ' + JSON.stringify(reg.exec(str)),'\\/[a-z]*> ["/style>"]');
  
  reg = new RegExp("\\/[a-z]*>");
  assert_equal(reg.source + ' ' + JSON.stringify(reg.exec(str)),'\\/[a-z]*> ["/style>"]');

  reg = new RegExp("\\\/[a-z]*>");
  assert_equal(reg.source + ' ' + JSON.stringify(reg.exec(str)),'\\/[a-z]*> ["/style>"]');

  str="<style>test<\\\\/style>"
  reg = new RegExp("\\\\/[a-z]*>");
  assert_equal(reg.source + ' ' + JSON.stringify(reg.exec(str)),'\\\\\\/[a-z]*> ["\\\\/style>"]');
  
  reg = new RegExp("\\\\\/[a-z]*>");
  assert_equal(reg.source + ' ' +  JSON.stringify(reg.exec(str)),'\\\\\\/[a-z]*> ["\\\\/style>"]');
  
  reg = new RegExp("\\\\\\/[a-z]*>");
  assert_equal(reg.source + ' ' +  JSON.stringify(reg.exec(str)),'\\\\\\/[a-z]*> ["\\\\/style>"]');

  reg = new RegExp("\\\\\\\/[a-z]*>");
  assert_equal(reg.source + ' ' +  JSON.stringify(reg.exec(str)),'\\\\\\/[a-z]*> ["\\\\/style>"]');
}
{
  let str="\\\\\\\\[aaa"
  try {
    let pattern="[";
    let reg = new RegExp(pattern);
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError,true);
  }
  try {
    let pattern="\[";
    let reg = new RegExp(pattern);
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError,true);
  }
  try {
    let pattern="\\[";
    let reg = new RegExp(pattern);
    assert_equal(pattern+" "+JSON.stringify(reg.exec(str)),'\\[ ["["]')
  } catch (error) {
    assert_unreachable();
  }
  try {
    let pattern="\\\[";
    let reg = new RegExp(pattern);
    assert_equal(pattern+" "+JSON.stringify(reg.exec(str)),'\\[ ["["]')
  } catch (error) {
    assert_unreachable();
  }
  try {
    let pattern="\\\\[";
    let reg = new RegExp(pattern);
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError,true);
  }
  try {
    let pattern="\\\\\[";
    let reg = new RegExp(pattern);
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof SyntaxError,true);
  }
  try {
    let pattern="\\\\\\[";
    let reg = new RegExp(pattern);
    assert_equal(pattern+" "+JSON.stringify(reg.exec(str)),'\\\\\\[ ["\\\\["]')
  } catch (error) {
    assert_unreachable();
  }
  try {
    let pattern="\\\\\\\[";
    let reg = new RegExp(pattern);
    assert_equal(pattern+" "+JSON.stringify(reg.exec(str)),'\\\\\\[ ["\\\\["]')
  } catch (error) {
    assert_unreachable();
  }
}

{
  Object.defineProperty(RegExp.prototype, "global", {
    value: false
  })
  let str1;
  let result;
  const re1 = /[Cz]/;
  const re2 = /[Cz]/g;
  const re3 = /([Cz])/;
  const re4 = /([Cz])/g;

  function createHaystack() {
    let s = "abCdefgz";
    for (let i = 0; i < 3; i++) s += s;
    return s;
  }
  str1 = createHaystack();
  function String1Replace(re) {
    result = re[Symbol.replace](str1, ".");
  }
  function String2Replace(re) {
    result = re[Symbol.replace](str1, "xyz");
  }
  function String3Replace(re) {
    result = re[Symbol.replace](str1, "x$1yz");
  }
  function Function1Replace(re) {
    result = re[Symbol.replace](str1, String);
  }
  String1Replace(re1);
  assert_equal(result,"ab.defgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");
  String1Replace(re2);
  assert_equal(result,"ab.defgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");
  String2Replace(re2);
  assert_equal(result,"abCdefgxyzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");
  String2Replace(re1);
  assert_equal(result,"abxyzdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");
  String3Replace(re3);
  assert_equal(result,"abxCyzdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");
  String3Replace(re4);
  assert_equal(result,"abxCyzdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");
  Function1Replace(re3);
  assert_equal(result,"abCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");
  Function1Replace(re4);
  assert_equal(result,"abCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgzabCdefgz");

  // subStringIsUtf8 branch canbecompressed
  str1 = 'utf83c这要替换"!';
  let regexp = /([0-9])([a-z])/g
  let newStr1 = str1.replace(regexp, "$1" );
  assert_equal(newStr1,'utf83这要替换"!');

  // subStringIsUtf8 branch length=0;
  str1 = '3c这要替换"!';
  regexp = /([0-9])([a-z])/g
  newStr1 = str1.replace(regexp, "$1" );
  assert_equal(newStr1,'3这要替换"!');

  // subStringIsUtf8 branch src isUtf8;
  str1 = 'abcdefg3chigk"!';
  regexp = /([0-9])([a-z])/g
  newStr1 = str1.replace(regexp, "$1" );
  assert_equal(newStr1,'abcdefg3higk"!');
}

{
  let s1 = 'abc';
  let s2 = '\ud834\udf06';
  let reg = new RegExp(s1.repeat(10000));
  let str = s1.repeat(10000);
  let res = str.replace(reg, s2);
  assert_equal(res == s2, true);

  reg = new RegExp(s2.repeat(10000));
  str = s2.repeat(10000);
  res = str.replace(reg, s1);
  assert_equal(res == s1, true);
}

{
  let reg1 = /(a)/
  reg1.test("a")
  assert_equal(RegExp.$1, "a");

  reg1.test("a")
  assert_equal(RegExp.$1, "a");

  let reg2 = /b(b)/;
  reg2.test("bb");
  assert_equal(RegExp.$1, "b");

  reg1.test("a")
  assert_equal(RegExp.$1, "a");
}

// This case aims to check overflow of error message while parsing regular expression
{
  let patternPart1 = "^(\\p{g}+)aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  let patternPart2 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  let pattern = patternPart1 + patternPart2;
  try {
    let r = new RegExp(pattern);
    assert_unreachable();
  } catch (e) {
    assert_equal(e instanceof SyntaxError, true);
  }
}

{
  class C3 extends Array {
  }
  const o7 = {
    get(a5, a6) {
      return 1942;
    },
  };
  const v9 = new Proxy(C3, o7);
  try {
    RegExp(v9).toString();
    assert_unreachable();
  } catch (e) {
    assert_equal(e instanceof SyntaxError, true);
  }
}
try {
  class C12 {
  }
  const v16 = ("symbol").constructor.fromCharCode(C12);
  const v17 = v16.repeat(65534);
  v17.replace(/(?<a>)/dumgs, v17);
} catch (e) {
  assert_equal(e.name, "RangeError");
}

{
  let name = "x".repeat(1000)
  let reg = new RegExp("(?<" + name + ">asas)", "d")
  let res = reg.exec("asas");
  assert_equal(res.groups[name], "asas");
}
test_end();