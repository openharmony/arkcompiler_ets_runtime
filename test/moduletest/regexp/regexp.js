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