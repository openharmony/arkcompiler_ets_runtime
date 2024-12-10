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
  print(JSON.stringify(reg.exec("cd")))
}
{
  let reg= /ab||/
  print(JSON.stringify(reg.exec("cd")))
}
{
  let reg= /ab|cd|ef/
  print(JSON.stringify(reg.exec("cd")))
}

{
  let str = "😀";
  let regexp = /[😀]/;
  print(JSON.stringify(str.replace(regexp,"b")));
}
{
  let str = "😀";
  let regexp = /[😀]/g;
  print(JSON.stringify(str.replace(regexp,"b")));
}
{
  let str = "😀";
  let regexp = /[😀]/u;
  print(JSON.stringify(str.replace(regexp,"b")));
}
{
  let str = "😀";
  let regexp = /[\😀]/;
  print(JSON.stringify(str.replace(regexp,"b")));
}


var reg = /[\x5d-\x7e]/i;
var result = reg.test("a");
print(result);

var reg1 = new RegExp("^[-+]?([0-9]+)?(\\٫[0-9]{1,})?$");
var result1 = reg1.test('0٫0000000000001');
print(result1);

var reg2 = /^[Α-ώ]+$/i
print(reg2.test('άέήίΰϊϋόύώ'));

print(reg2.test('ΆΈΉΊΪΫΎΏ'));

print(reg2.test('αβγδεζηθικλμνξοπρςστυφχψω'));

print(reg2.test('ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ'));

let reg3 =/^[A-Z0-9_\-]*$/i
print(reg3.test(''))

let reg4 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>https?[:：]//[^/]+/svn(?:/[a-z0-9.,;?'*:+&%$#=~_ \\u4E00-\\u9FA5-]*)*).*$", "i")
print(reg4.test('a'));

let reg5 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>(?:(?:ht|f)tps?[:：]//)?(?:[a-z0-9-]+\\.)+" + "(?:com|edu|gov|mil|net|org|biz|info|name|museum|us|ca|uk|cn|cc|tw|de|au|sg|hk|ei|fr|me|im)(?![a-z])" + "(?:\\:[0-9][0-9]*)?(?:\\.?/[a-z0-9.,;?'\\|*:\\\\+&%$#=~_-]*)*).*$", "i")
print(reg5.test('a'));

let reg6 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>(?:ht|f)tps?[:：]//(?:[a-z0-9-]+\\.)*[a-z0-9-]+(?:/[a-z0-9]+)*[/a-z0-9.,;?'\\|*:\\\\+&%$#=~_-]*).*$", "i")
print(reg6.test('a'));

let reg7 =   new RegExp("^(?<urlStrIndex>.*?)(?<urlStr>(?:https?[:：]//)?(?:[a-z0-9-\\\\]+\\.)+" + "(?:com|edu|gov|mil|net|org|biz|info|name|museum|us|ca|uk|cn|cc|tw|de|au|sg|hk|ei|fr|me|im)(?![a-z])" + "(?:\\:\\d{4})?(?:/[a-z0-9.,;?'\\|*:\\\\+&%$#=~_-]*)*\\?(?:[a-z0-9]*=[a-z0-9.,;?'*:+%$#=~_\\u4E00-\\u9FA5-]*&?)*).*$", "i")
print(reg7.test('a'));

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
print(temp)

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
print(temp1)
let str1 = 'SC52BAHL01031234567890123456USD'
print(str1.replace(/[^A-Z0-9]+/gi, ''))

let reg50 = /^[ABCEGHJKLMNPRSTVXY]\d[ABCEGHJ-NPRSTV-Z][\s-]?\d[ABCEGHJ-NPRSTV-Z]\d$/i

let regabc = /abc/g;
let strabcd = "abcdabcdabcd";
for (let i = 0; i < 10; i++) {
  // cache is used in this case
  print(regabc.test(strabcd));
}

let str2 = "aaaabbBbcccC";
for (let i = 0; i < 2; i++) {
  print(str2);
  let t1 = str2.replace(/([A-Z])/g, function(e) {
    return "_" + e;
  });
  print(t1);
  let t2 = str2.replace(/([A-Z])/g, "_$1");
  print(t2);
  print(t1.replace(/([a-z]+)/g, "_xy"));
  print(t2.replace(/_/g, ""));
}

// regexp cache test
let mediaReg = "\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)\([^\)]+\)";
let string = '(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s(s';
const regex1 = new RegExp(mediaReg);
let matchArray = string.match(regex1);
print(matchArray);

// Test regexp.test fastpath
var protoExec = RegExp.prototype.exec
RegExp.prototype.exec = function () {
  return null
}
var reg = /a/
print(reg.test("aaaaa"));

delete RegExp.prototype.exec
print(reg.test("aaaaa"));

Object.prototype.exec = function () {
  return null
}
print(reg.test("aaaaa"));

delete Object.prototype.exec
RegExp.prototype.exec = protoExec
print(reg.test("aaaaa"));

var protoTest = RegExp.prototype.test
RegExp.prototype.test = function () {
    return false
}
var reg2 = /foo*/
print(reg2.test("fooooooo"));

RegExp.prototype.test = protoTest
print(reg2.test("fooooooo"));

// Same hash in cached result, but different flags.
var regexp1 = /a*/gs;
var regexp2 = /a*/g;
regexp2.lastIndex = 8;
print(regexp1.exec('aaa'));
print(regexp2.exec('aaa'));

// Same hash in cached result, and same flags, but different lastIndex.
var regexp3 = /a*/g;
var regexp4 = /a*/g;
regexp4.lastIndex = 1;
print(regexp3.exec('aaabab'));
print(regexp4.exec('aaabaa'));

const v43 = /V[\d-\d]/ys;
const o54 = {
    __proto__: v43,
};
try {
    o54.test(Map);
} catch (e) {
    print(e)
}

// Testing regexp.prototype.replace after optimization
{
  const re1 = /[Cz]/;
  const re2 = /[Cz]/g;
  const re3 = /([Cz])/;
  const re4 = /([Cz])/g;

  let replace_str = "abCdefgzabCdefgzabCdefgz";
  let replace_result = replace_str.replace(re1, "");
  print(re1.lastIndex);
  let cached_reuslt = replace_str.replace(re1, "");
  print(replace_result === cached_reuslt);
  print(re1.lastIndex);

  replace_result = replace_str.replace(re2, "xyz");
  print(re2.lastIndex);
  cached_reuslt = replace_str.replace(re2, "xyz");
  print(replace_result === cached_reuslt);
  print(re2.lastIndex);

  replace_result = replace_str.replace(re3, "x$1yz");
  print(re3.lastIndex);
  cached_reuslt = replace_str.replace(re3, "x$1yz");
  print(replace_result === cached_reuslt);
  print(re3.lastIndex);

  replace_result = replace_str.replace(re4, String);
  print(re4.lastIndex);
  cached_reuslt = replace_str.replace(re4, String);
  print(replace_result === cached_reuslt);
  print(re4.lastIndex);
}

// test RegExp.prototype.xx
print(RegExp.prototype.dotAll)
print(RegExp.prototype.global)
print(RegExp.prototype.hasIndices)
print(RegExp.prototype.ignoreCase)
print(RegExp.prototype.multiline)
print(RegExp.prototype.sticky)
print(RegExp.prototype.unicode)
print(RegExp.prototype.lastIndex)
print(RegExp.prototype.flags)
print(RegExp.prototype.source)
try {
    RegExp.prototype.test("abc")
} catch (e) {
    print(e.name)
}
try {
    RegExp.prototype.exec("abc")
} catch (e) {
    print(e.name)
}
print(RegExp.prototype.toString())

let inputString = "/vedio/av{avid}{cid}";
let extractedContent = inputString.match(/\{([^{}]+)\}/g);
let replacedString = inputString.replace(/\{([^{}]+)\}/g, '(uuu)').replace(/\//g, "\\/");
print(replacedString);

let str = "beep boop   afff测试样本one1";
print(str.split(/([{}:;,]|\s+)/));

function verifyRegExp(text) {
  text = text.replace(new RegExp('[\\s]', 'g'), ' ');
  let emRegExp = new RegExp('<em>.*?</em>', 'ig');
  let results = text.split(emRegExp);
  text.match(emRegExp);
  return results;
}

const srcTextA = '<em>a</em>bcdefghijklmnopqrstuvwxyz<em>a</em>bcdefghijklmnopqrstuvwxyz<em>a</em>bcdefghijklmnopqrstuvwxyz<em>a</em>bcdefghijklmnopqrstuvwxyz'
const srcTextAbcd = '<em>abcd</em>efghijklmnopqrstuvwxyz<em>abcd</em>efghijklmnopqrstuvwxyz<em>abcd</em>efghijklmnopqrstuvwxyz<em>abcd</em>efghijklmnopqrstuvwxyz'

print(`verifyRegExp_1: ${verifyRegExp(srcTextA)}`);
print(`verifyRegExp_2: ${verifyRegExp(srcTextAbcd)}`);
print(`verifyRegExp_3: ${verifyRegExp(srcTextAbcd)}`);

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
print(ans1);
print(ans2);
print(ans3);
print(ans4);
print(ans5);
print(ans6);
print(ans7);
print(ans8);
print(ans9);
print(ans10);
try {
  const ans11 = RegExp("/[\\p{}]/u");
  print(ans11);
} catch(e) {
  print(e);
}
const str3 = "a-b-c";
const re = /-/y;
print(str3.split(re));

re.lastIndex = 1;
print(str3.split(re));

re.lastIndex = -1;
print(str3.split(re));

re.lastIndex = 3;
print(str3.split(re));

print(re.test(str3));

print(str3.split(/-/g));

// search
const str4 = "abc";
let re1 = /b/;
re1.lastIndex = 2;
print(str4.search(re1));
print(str4.search(/b/y));
print(str4.search(re1));
print(re1.lastIndex);

// check cache
const str5 = "a-bc";
let re2 = /-/;
re2.lastIndex = 2;
print(str5.split(re2));
print(re2.lastIndex);
print(str5.split(re2));
print(re2.lastIndex);

const str6 = "abcabc";
let re3 = /abc/;
re3.lastIndex = 2;
print(str6.match(re3));
print(re3.lastIndex);
print(str6.match(re3));
print(re3.lastIndex);

let re4 = /abc/g;
re4.lastIndex = 2;
print(str6.match(re4));
print(re4.lastIndex);
print(str6.match(re4));
print(re4.lastIndex);
Object.defineProperty(RegExp.prototype, "global", {
  value: true
})
var flags = RegExp.prototype.flags;
Object.defineProperty(RegExp.prototype, "global", {
  value: false
})
print(flags);
print(str6.match(re4));
print(re4.lastIndex);
print(str6.match(re4));
print(re4.lastIndex);

let myExp = new RegExp("a+b+c");
Object.defineProperty(myExp, "sticky", {
    value: true
  })
print(myExp.toString());

// Testing regexp.prototype.replace after optimization
{
  const re1 = /[Cz]/;
  const re2 = /[Cz]/g;
  const re3 = /([Cz])/;
  const re4 = /([Cz])/g;

  let replace_str = "abCdefgzabCdefgzabCdefgz";
  let replace_result = replace_str.replace(re1, "");
  print(re1.lastIndex);
  let cached_reuslt = replace_str.replace(re1, "");
  print(replace_result === cached_reuslt);
  print(re1.lastIndex);

  replace_result = replace_str.replace(re2, "xyz");
  print(re2.lastIndex);
  cached_reuslt = replace_str.replace(re2, "xyz");
  print(replace_result === cached_reuslt);
  print(re2.lastIndex);

  replace_result = replace_str.replace(re3, "x$1yz");
  print(re3.lastIndex);
  cached_reuslt = replace_str.replace(re3, "x$1yz");
  print(replace_result === cached_reuslt);
  print(re3.lastIndex);

  replace_result = replace_str.replace(re4, String);
  print(re4.lastIndex);
  cached_reuslt = replace_str.replace(re4, String);
  print(replace_result === cached_reuslt);
  print(re4.lastIndex);
}

const regex = /(?:)+/;
const str10 = "abcabcabc";
const matches = regex.test(str10);
print(matches);
const matches1 = regex.exec(str10);
print(matches1);

try {
  let matchReg = new RegExp("@【哈哈】*^o^*|@小米（kk）",'g');
} catch (error) {
  print(error)
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
  print(v0["exec"]());
}

{
  ("65535").split(/(?!\1(a\1)\1)\1/g);
  const o4 = {
    ...RegExp,
  };
  print(o4);
}

{
  const v2 = /e\8Z(x)(x)(x)(x)(x)(x)(x)(x)(x)(x)\10*/misd;
  v2[1073741824] = -194290175n;
  for (let v3 = 0; v3 < 2; v3++) {
    print(v2.test(-194290175n));
  }
}

function f(...args) {
  return {};
}
let reg51 = new RegExp("a");
f.exec = f;
let relpfun = reg51[Symbol.replace];
relpfun.apply(f, [1, 2, 3, 4]);
print("success");

{
  let str = /^\s*([^;\s]*)/;
  str.test("text/html");
  print(RegExp.$1);
  str.test("text/plain");
  print(RegExp.$1);
  str.test("text/html");
  print(RegExp.$1);
}

{
  let reg52 = /abc/;
  let count = 0;
  print(reg52.ignoreCase);
  print(reg52.global);
  print(reg52.flags);
  Object.defineProperty(reg52, "global", {
    get: function() { count++; return true; }
  });
  Object.defineProperty(reg52, "ignoreCase", {
    get: function() { count++; return true; }
  });

  print(reg52.ignoreCase);
  print(count);
  print(reg52.global);
  print(count);
  print(reg52.flags);
  print(count);
}

// The above test case in false environment did not reset the environment
Object.defineProperty(RegExp.prototype, "global", {
  value: true
})
string = 'aaa\n789\r\nccc\r\n345';
var pattern = /\d$/gm;
result = string.match(pattern);
print(2 == result.length);
print('9' == result[0]);
print('5' == result[1]);

string = 'aaa\n789\r\nccc\r\nddd';
pattern = /\d$/gm;
result = string.match(pattern);
print(1 == result.length);
print('9' == result[0]);

// test getFlags
Object.defineProperty(RegExp.prototype, "global", {
  value: true
})
const res = /abc/giymsud;
res.lastIndex = -1;
print(res.flags);
const res1 = /abc/g;
res1.lastIndex = -1;
print(res1.flags);
const res2 = /abc/i;
res2.lastIndex = -1;
print(res2.flags);
const res3 = /abc/y;
res3.lastIndex = -1;
print(res3.flags);
const res4 = /abc/m;
res4.lastIndex = -1;
print(res4.flags);
const res5 = /abc/s;
res5.lastIndex = -1;
print(res5.flags);
const res6 = /abc/u;
res6.lastIndex = -1;
print(res6.flags);
const res7 = /abc/d;
res7.lastIndex = -1;
print(res7.flags);
const res8 = /abc/;
res8.lastIndex = -1;
print(res8.flags);

{
  let str = "<style>test</style>"
  let reg = new RegExp("/");
  print(reg.source,JSON.stringify(reg.exec(str)));
  
  reg = new RegExp("\/[a-z]*>");
  print(reg.source,JSON.stringify(reg.exec(str)));
  
  reg = new RegExp("\\/[a-z]*>");
  print(reg.source,JSON.stringify(reg.exec(str)));

  reg = new RegExp("\\\/[a-z]*>");
  print(reg.source,JSON.stringify(reg.exec(str)));

  str="<style>test<\\\\/style>"
  reg = new RegExp("\\\\/[a-z]*>");
  print(reg.source,JSON.stringify(reg.exec(str)));
  
  reg = new RegExp("\\\\\/[a-z]*>");
  print(reg.source,JSON.stringify(reg.exec(str)));
  
  reg = new RegExp("\\\\\\/[a-z]*>");
  print(reg.source,JSON.stringify(reg.exec(str)));

  reg = new RegExp("\\\\\\\/[a-z]*>");
  print(reg.source,JSON.stringify(reg.exec(str)));
}
{
  let str="\\\\\\\\[aaa"
  try {
    let pattern="[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
  }
  try {
    let pattern="\[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
  }
  try {
    let pattern="\\[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
  }
  try {
    let pattern="\\\[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
  }
  try {
    let pattern="\\\\[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
  }
  try {
    let pattern="\\\\\[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
  }
  try {
    let pattern="\\\\\\[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
  }
  try {
    let pattern="\\\\\\\[";
    let reg = new RegExp(pattern);
    print(pattern,JSON.stringify(reg.exec(str)))
  } catch (e) {
    print(e.name);
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
  print(result);
  String1Replace(re2);
  print(result);
  String2Replace(re2);
  print(result);
  String2Replace(re1);
  print(result);
  String3Replace(re3);
  print(result);
  String3Replace(re4);
  print(result);
  Function1Replace(re3);
  print(result);
  Function1Replace(re4);
  print(result);

  // subStringIsUtf8 branch canbecompressed
  str1 = 'utf83c这要替换"!';
  let regexp = /([0-9])([a-z])/g
  let newStr1 = str1.replace(regexp, "$1" );
  print(newStr1);

  // subStringIsUtf8 branch length=0;
  str1 = '3c这要替换"!';
  regexp = /([0-9])([a-z])/g
  newStr1 = str1.replace(regexp, "$1" );
  print(newStr1);

  // subStringIsUtf8 branch src isUtf8;
  str1 = 'abcdefg3chigk"!';
  regexp = /([0-9])([a-z])/g
  newStr1 = str1.replace(regexp, "$1" );
  print(newStr1);
}

{
  let s1 = 'abc';
  let s2 = '\ud834\udf06';
  let reg = new RegExp(s1.repeat(10000));
  let str = s1.repeat(10000);
  let res = str.replace(reg, s2);
  print(res == s2);

  reg = new RegExp(s2.repeat(10000));
  str = s2.repeat(10000);
  res = str.replace(reg, s1);
  print(res == s1);
}
