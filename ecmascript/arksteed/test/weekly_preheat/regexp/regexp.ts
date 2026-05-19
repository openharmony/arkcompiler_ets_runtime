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

const REGEXP_2: number = 2;
const REGEXP_3: number = 3;
const REGEXP_4: number = 4;
const REGEXP_5: number = 5;
const REGEXP_6: number = 6;
const REGEXP_7: number = 7;
const REGEXP_8: number = 8;
const REGEXP_9: number = 9;
const REGEXP_10: number = 10;
const REGEXP_11: number = 11;
const REGEXP_12: number = 12;
const REGEXP_13: number = 13;
const REGEXP_14: number = 14;
const REGEXP_15: number = 15;
const REGEXP_16: number = 16;
const REGEXP_17: number = 17;
const REGEXP_18: number = 18;
const REGEXP_20: number = 20;
const REGEXP_21: number = 21;
const REGEXP_22: number = 22;
const REGEXP_23: number = 23;
const REGEXP_27: number = 27;
const REGEXP_28: number = 28;
const REGEXP_30: number = 30;
const REGEXP_31: number = 31;
const REGEXP_34: number = 34;
const REGEXP_35: number = 35;
const REGEXP_37: number = 37;
const REGEXP_39: number = 39;
const REGEXP_40: number = 40;
const REGEXP_44: number = 44;
const REGEXP_49: number = 49;
const REGEXP_68: number = 68;
const REGEXP_70: number = 70;
const REGEXP_72: number = 72;
const REGEXP_73: number = 73;
const REGEXP_77: number = 77;
const REGEXP_78: number = 78;
const REGEXP_81: number = 81;
const REGEXP_85: number = 85;
const REGEXP_92: number = 92;
const REGEXP_93: number = 93;
const REGEXP_95: number = 95;
const REGEXP_117: number = 117;
const REGEXP_128: number = 128;
const REGEXP_137: number = 137;
const REGEXP_139: number = 139;
const REGEXP_144: number = 144;
const REGEXP_156: number = 156;
const REGEXP_170: number = 170;
const REGEXP_177: number = 177;
const REGEXP_282: number = 282;
const REGEXP_312: number = 312;
const REGEXP_352: number = 352;
const REGEXP_454: number = 454;
const REGEXP_525: number = 525;
const REGEXP_598: number = 598;
const REGEXP_739: number = 739;
const REGEXP_1000: number = 1000;
const REGEXP_1844: number = 1844;
const REGEXP_2369: number = 2369;
const REGEXP_3108: number = 3108;
const REGEXP_3706: number = 3706;
const REGEXP_4160: number = 4160;
const REGEXP_4512: number = 4512;
const REGEXP_4824: number = 4824;
const REGEXP_5106: number = 5106;
const REGEXP_5283: number = 5283;
const REGEXP_5453: number = 5453;
const REGEXP_5609: number = 5609;
const REGEXP_5753: number = 5753;
const REGEXP_5892: number = 5892;
const REGEXP_6029: number = 6029;
const REGEXP_6146: number = 6146;
const REGEXP_6241: number = 6241;
const REGEXP_6334: number = 6334;
const REGEXP_6426: number = 6426;
const REGEXP_6511: number = 6511;
function computeInvariants(str: string, n: number): string[] {
  let variants = [str];
  for (let i = 1; i < n; i++) {
    let pos = Math.floor(Math.random() * str.length);
    let chr = String.fromCharCode((str.charCodeAt(pos) + Math.floor(Math.random() * REGEXP_128)) % REGEXP_128);
    let cpStr = str;
    variants[i] = cpStr.substring(0, pos) + chr + cpStr.substring(pos + 1, cpStr.length);
  }
  return variants;
}
declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 * @State
 * @Tags Jetstream2
 */
class RegExpBenchmark {
  /*
   * @Benchmark
   */
  run(): void {
    let startTime = ArkTools.timeInUs() / REGEXP_1000;
    for (let i = 0; i < REGEXP_5; i++) {
      let sum = 0;
      sum += runBlock0();
      sum += runBlock1();
      sum += runBlock2();
      sum += runBlock3();
      sum += runBlock4();
      sum += runBlock5();
      sum += runBlock6();
      sum += runBlock7();
      sum += runBlock8();
      sum += runBlock9();
      sum += runBlock10();
      sum += runBlock11();
    }
    let endTime = ArkTools.timeInUs() / REGEXP_1000;
    print('regexp: ms =', endTime - startTime);
  }

  execString(re: RegExp, string: string): number {
    let sum = 0;
    re.lastIndex = 0;
    let array = re.exec(string);
    if (array !== null && array !== undefined) {
      for (let i = 0; i < array.length; i++) {
        let substring = array[i];
        if (substring) {
          sum += substring.length;
        }
      }
    }
    return sum;
  }
}

let re0: RegExp = new RegExp('^ba');
let re1: RegExp = new RegExp('(((\\w+):\\/\\/)([^\\/:]*)(:(\\d+))?)?([^#?]*)(\\?([^#]*))?(#(.*))?');
let re2: RegExp = new RegExp('^\\s*|\\s*$', 'g');
let re3: RegExp = new RegExp('\\bQBZPbageby_cynprubyqre\\b');
let re4: RegExp = new RegExp(',');
let re5: RegExp = new RegExp('\\bQBZPbageby_cynprubyqre\\b', 'g');
let re6: RegExp = new RegExp('^\\s*|\\s*$', 'g');
let re7: RegExp = new RegExp('(\\d*)(\\D*)', 'g');
let re8: RegExp = new RegExp('=');
let re9: RegExp = new RegExp('(^|\\s)lhv-h(\\s|$)');
let str0 = 'Zbmvyyn/5.0 (Jvaqbjf; H; Jvaqbjf AG 5.1; ra-HF) NccyrJroXvg/528.9 (XUGZY, yvxr Trpxb) Puebzr/2.0.157.0 Fnsnev/528.9';
let re10: RegExp = new RegExp('#', 'g');
let re11: RegExp = new RegExp('.', 'g');
let re12: RegExp = new RegExp("'", 'g');
let re13: RegExp = new RegExp('\\?[wW]*(sevraqvq|punaaryvq|tebhcvq)=([^&?#]*)', 'i');
let str1 = 'Fubpxjnir Synfu 9.0  e115';
let re14: RegExp = new RegExp('\\s+', 'g');
let re15: RegExp = new RegExp('^\\s*(\\S*(\\s+\\S+)*)\\s*$');
let re16: RegExp = new RegExp('-[a-z]', 'i');

let s0 = computeInvariants('pyvpx', REGEXP_6511);
let s1 = computeInvariants('uggc://jjj.snprobbx.pbz/ybtva.cuc', REGEXP_1844);
let s2 = computeInvariants('QBZPbageby_cynprubyqre', REGEXP_739);
let s3 = computeInvariants('uggc://jjj.snprobbx.pbz/', REGEXP_598);
let s4 = computeInvariants('uggc://jjj.snprobbx.pbz/fepu.cuc', REGEXP_454);
let s5 = computeInvariants('qqqq, ZZZ q, llll', REGEXP_352);
let s6 = computeInvariants('vachggrkg QBZPbageby_cynprubyqre', REGEXP_312);
let s7 = computeInvariants('/ZlFcnprUbzrcntr/Vaqrk-FvgrUbzr,10000000', REGEXP_282);
let s8 = computeInvariants('vachggrkg', REGEXP_177);
let s9 = computeInvariants('528.9', REGEXP_170);
let s10 = computeInvariants('528', REGEXP_170);
let s11 = computeInvariants('VCPhygher=ra-HF', REGEXP_156);
let s12 = computeInvariants('CersreerqPhygher=ra-HF', REGEXP_156);
let s13 = computeInvariants('xrlcerff', REGEXP_144);
let s14 = computeInvariants('521', REGEXP_139);
let s15 = computeInvariants(str0, REGEXP_139);
let s16 = computeInvariants('qvi .so_zrah', REGEXP_137);
let s17 = computeInvariants('qvi.so_zrah', REGEXP_137);
let s18 = computeInvariants('uvqqra_ryrz', REGEXP_117);
let s19 = computeInvariants('sevraqfgre_naba=nvq%3Qn6ss9p85n868ro9s059pn854735956o3%26ers%3Q%26df%3Q%26vpgl%3QHF', REGEXP_95);
let s20 = computeInvariants('uggc://ubzr.zlfcnpr.pbz/vaqrk.psz', REGEXP_93);
let s21 = computeInvariants(str1, REGEXP_92);
let s22 = computeInvariants('svefg', REGEXP_85);
let s23 = computeInvariants('uggc://cebsvyr.zlfcnpr.pbz/vaqrk.psz', REGEXP_85);
let s24 = computeInvariants('ynfg', REGEXP_85);
let s25 = computeInvariants('qvfcynl', REGEXP_85);

function runBlock0(): number {
  let sum: number = 0;
  for (let i = 0; i < REGEXP_525; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i]);
  }
  for (let i = 0; i < REGEXP_1844; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_525]);
    sum += new RegExpBenchmark().execString(re1, s1[i]);
  }
  for (let i = 0; i < REGEXP_739; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_2369]);
    sum += s2[i].replace(re2, '').length;
  }
  for (let i = 0; i < REGEXP_598; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_3108]);
    sum += new RegExpBenchmark().execString(re1, s3[i]);
  }
  for (let i = 0; i < REGEXP_454; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_3706]);
    sum += new RegExpBenchmark().execString(re1, s4[i]);
  }
  for (let i = 0; i < REGEXP_352; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_4160]);
    sum += new RegExpBenchmark().execString(new RegExp('/qqqq|qqq|qq|q|ZZZZ|ZZZ|ZZ|Z|llll|ll|l|uu|u|UU|U|zz|z|ff|f|gg|g|sss|ss|s|mmm|mm|m/g'), s5[i]);
  }
  for (let i = 0; i < REGEXP_312; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_4512]);
    sum += new RegExpBenchmark().execString(re3, s6[i]);
  }
  for (let i = 0; i < REGEXP_282; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_4824]);
    sum += new RegExpBenchmark().execString(re4, s7[i]);
  }
  for (let i = 0; i < REGEXP_177; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_5106]);
    sum += s8[i].replace(re5, '').length;
  }
  for (let i = 0; i < REGEXP_170; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_5283]);
    sum += s9[i].replace(re6, '').length;
    sum += new RegExpBenchmark().execString(re7, s10[i]);
  }
  for (let i = 0; i < REGEXP_156; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_5453]);
    sum += new RegExpBenchmark().execString(re8, s11[i]);
    sum += new RegExpBenchmark().execString(re8, s12[i]);
  }
  for (let i = 0; i < REGEXP_144; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_5609]);
    sum += new RegExpBenchmark().execString(re0, s13[i]);
  }
  for (let i = 0; i < REGEXP_139; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_5753]);
    sum += s14[i].replace(re6, '').length;
    sum += new RegExpBenchmark().execString(re7, s14[i]);
    sum += new RegExpBenchmark().execString(re9, '');
    sum += new RegExpBenchmark().execString(new RegExp('JroXvg/(S+)'), s15[i]);
  }
  for (let i = 0; i < REGEXP_137; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_5892]);
    sum += s16[i].replace(re10, '').length;
    sum += s16[i].replace(new RegExp('\\[', 'g'), '').length;
    sum += s17[i].replace(re11, '').length;
  }
  for (let i = 0; i < REGEXP_117; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_6029]);
    sum += s18[i].replace(re2, '').length;
  }
  for (let i = 0; i < REGEXP_95; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_6146]);
    sum += new RegExpBenchmark().execString(new RegExp('(?:^|;)s*sevraqfgre_ynat=([^;]*)'), s19[i]);
  }
  for (let i = 0; i < REGEXP_93; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_6241]);
    sum += s20[i].replace(re12, '').length;
    sum += new RegExpBenchmark().execString(re13, s20[i]);
  }
  for (let i = 0; i < REGEXP_92; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_6334]);
    sum += s21[i].replace(new RegExp('([a-zA-Z]|s)+'), '').length;
  }
  for (let i = 0; i < REGEXP_85; i++) {
    sum += new RegExpBenchmark().execString(re0, s0[i + REGEXP_6426]);
    sum += s22[i].replace(re14, '').length;
    sum += s22[i].replace(re15, '').length;
    sum += s23[i].replace(re12, '').length;
    sum += s24[i].replace(re14, '').length;
    sum += s24[i].replace(re15, '').length;
    sum += new RegExpBenchmark().execString(re16, s25[i]);
    sum += new RegExpBenchmark().execString(re13, s23[i]);
  }
  return sum;
}
let re17: RegExp = new RegExp('(^|[^\\\\])\\"\\\\\\/Qngr\\((-?[0-9]+)\\)\\\\\\/\\"', 'g');
let str2 =
  '{"anzr":"","ahzoreSbezng":{"PheeraplQrpvznyQvtvgf":2,"PheeraplQrpvznyFrcnengbe":".","VfErnqBayl":gehr,"PheeraplTebhcFvmrf":[3],"AhzoreTebhcFvmrf":[3],"CrepragTebhcFvmrf":[3],"PheeraplTebhcFrcnengbe":",","PheeraplFlzoby":"\xa4","AnAFlzoby":"AnA","PheeraplArtngvirCnggrea":0,"AhzoreArtngvirCnggrea":1,"CrepragCbfvgvirCnggrea":0,"CrepragArtngvirCnggrea":0,"ArtngvirVasvavglFlzoby":"-Vasvavgl","ArtngvirFvta":"-","AhzoreQrpvznyQvtvgf":2,"AhzoreQrpvznyFrcnengbe":".","AhzoreTebhcFrcnengbe":",","PheeraplCbfvgvirCnggrea":0,"CbfvgvirVasvavglFlzoby":"Vasvavgl","CbfvgvirFvta":"+","CrepragQrpvznyQvtvgf":2,"CrepragQrpvznyFrcnengbe":".","CrepragTebhcFrcnengbe":",","CrepragFlzoby":"%","CreZvyyrFlzoby":"\u2030","AngvirQvtvgf":["0","1","2","3","4","5","6","7","8","9"],"QvtvgFhofgvghgvba":1},"qngrGvzrSbezng":{"NZQrfvtangbe":"NZ","Pnyraqne":{"ZvaFhccbegrqQngrGvzr":"@-62135568000000@","ZnkFhccbegrqQngrGvzr":"@253402300799999@","NytbevguzGlcr":1,"PnyraqneGlcr":1,"Renf":[1],"GjbQvtvgLrneZnk":2029,"VfErnqBayl":gehr},"QngrFrcnengbe":"/","SvefgQnlBsJrrx":0,"PnyraqneJrrxEhyr":0,"ShyyQngrGvzrCnggrea":"qqqq, qq ZZZZ llll UU:zz:ff","YbatQngrCnggrea":"qqqq, qq ZZZZ llll","YbatGvzrCnggrea":"UU:zz:ff","ZbaguQnlCnggrea":"ZZZZ qq","CZQrfvtangbe":"CZ","ESP1123Cnggrea":"qqq, qq ZZZ llll UU\':\'zz\':\'ff \'TZG\'","FubegQngrCnggrea":"ZZ/qq/llll","FubegGvzrCnggrea":"UU:zz","FbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq\'G\'UU\':\'zz\':\'ff","GvzrFrcnengbe":":","HavirefnyFbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq UU\':\'zz\':\'ff\'M\'","LrneZbaguCnggrea":"llll ZZZZ","NooerivngrqQnlAnzrf":["Fha","Zba","Ghr","Jrq","Guh","Sev","Fng"],"FubegrfgQnlAnzrf":["Fh","Zb","Gh","Jr","Gu","Se","Fn"],"QnlAnzrf":["Fhaqnl","Zbaqnl","Ghrfqnl","Jrqarfqnl","Guhefqnl","Sevqnl","Fngheqnl"],"NooerivngrqZbaguAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""],"VfErnqBayl":gehr,"AngvirPnyraqneAnzr":"Tertbevna Pnyraqne","NooerivngrqZbaguTravgvirAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguTravgvirAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""]}}';
let str3 =
  '{"anzr":"ra-HF","ahzoreSbezng":{"PheeraplQrpvznyQvtvgf":2,"PheeraplQrpvznyFrcnengbe":".","VfErnqBayl":snyfr,"PheeraplTebhcFvmrf":[3],"AhzoreTebhcFvmrf":[3],"CrepragTebhcFvmrf":[3],"PheeraplTebhcFrcnengbe":",","PheeraplFlzoby":"$","AnAFlzoby":"AnA","PheeraplArtngvirCnggrea":0,"AhzoreArtngvirCnggrea":1,"CrepragCbfvgvirCnggrea":0,"CrepragArtngvirCnggrea":0,"ArtngvirVasvavglFlzoby":"-Vasvavgl","ArtngvirFvta":"-","AhzoreQrpvznyQvtvgf":2,"AhzoreQrpvznyFrcnengbe":".","AhzoreTebhcFrcnengbe":",","PheeraplCbfvgvirCnggrea":0,"CbfvgvirVasvavglFlzoby":"Vasvavgl","CbfvgvirFvta":"+","CrepragQrpvznyQvtvgf":2,"CrepragQrpvznyFrcnengbe":".","CrepragTebhcFrcnengbe":",","CrepragFlzoby":"%","CreZvyyrFlzoby":"\u2030","AngvirQvtvgf":["0","1","2","3","4","5","6","7","8","9"],"QvtvgFhofgvghgvba":1},"qngrGvzrSbezng":{"NZQrfvtangbe":"NZ","Pnyraqne":{"ZvaFhccbegrqQngrGvzr":"@-62135568000000@","ZnkFhccbegrqQngrGvzr":"@253402300799999@","NytbevguzGlcr":1,"PnyraqneGlcr":1,"Renf":[1],"GjbQvtvgLrneZnk":2029,"VfErnqBayl":snyfr},"QngrFrcnengbe":"/","SvefgQnlBsJrrx":0,"PnyraqneJrrxEhyr":0,"ShyyQngrGvzrCnggrea":"qqqq, ZZZZ qq, llll u:zz:ff gg","YbatQngrCnggrea":"qqqq, ZZZZ qq, llll","YbatGvzrCnggrea":"u:zz:ff gg","ZbaguQnlCnggrea":"ZZZZ qq","CZQrfvtangbe":"CZ","ESP1123Cnggrea":"qqq, qq ZZZ llll UU\':\'zz\':\'ff \'TZG\'","FubegQngrCnggrea":"Z/q/llll","FubegGvzrCnggrea":"u:zz gg","FbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq\'G\'UU\':\'zz\':\'ff","GvzrFrcnengbe":":","HavirefnyFbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq UU\':\'zz\':\'ff\'M\'","LrneZbaguCnggrea":"ZZZZ, llll","NooerivngrqQnlAnzrf":["Fha","Zba","Ghr","Jrq","Guh","Sev","Fng"],"FubegrfgQnlAnzrf":["Fh","Zb","Gh","Jr","Gu","Se","Fn"],"QnlAnzrf":["Fhaqnl","Zbaqnl","Ghrfqnl","Jrqarfqnl","Guhefqnl","Sevqnl","Fngheqnl"],"NooerivngrqZbaguAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""],"VfErnqBayl":snyfr,"AngvirPnyraqneAnzr":"Tertbevna Pnyraqne","NooerivngrqZbaguTravgvirAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguTravgvirAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""]}}';
let str4 =
  'HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str5 =
  'HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let re18: RegExp = new RegExp('^s+|s+$', 'g');
let str6 = 'uggc://jjj.snprobbx.pbz/vaqrk.cuc';
let re19: RegExp = new RegExp('(?:^|s+)ba(?:s+|$)');
let re20: RegExp = new RegExp('[+, ]');
let re21: RegExp = new RegExp('ybnqrq|pbzcyrgr');
let str7 =
  ';;jvaqbj.IjPurpxZbhfrCbfvgvbaNQ_VQ=shapgvba(r){vs(!r)ine r=jvaqbj.rirag;ine c=-1;vs(d1)c=d1.EbyybssCnary;ine bo=IjTrgBow("IjCnayNQ_VQ_"+c);vs(bo&&bo.fglyr.ivfvovyvgl=="ivfvoyr"){ine fns=IjFns?8:0;ine pheK=r.pyvragK+IjBOFpe("U")+fns,pheL=r.pyvragL+IjBOFpe("I")+fns;ine y=IjBOEC(NQ_VQ,bo,"Y"),g=IjBOEC(NQ_VQ,bo,"G");ine e=y+d1.Cnaryf[c].Jvqgu,o=g+d1.Cnaryf[c].Urvtug;vs((pheK<y)||(pheK>e)||(pheL<g)||(pheL>o)){vs(jvaqbj.IjBaEbyybssNQ_VQ)IjBaEbyybssNQ_VQ(c);ryfr IjPybfrNq(NQ_VQ,c,gehr,"");}ryfr erghea;}IjPnapryZbhfrYvfgrareNQ_VQ();};;jvaqbj.IjFrgEbyybssCnaryNQ_VQ=shapgvba(c){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;c=IjTc(NQ_VQ,c);vs(d1&&d1.EbyybssCnary>-1)IjPnapryZbhfrYvfgrareNQ_VQ();vs(d1)d1.EbyybssCnary=c;gel{vs(q.nqqRiragYvfgrare)q.nqqRiragYvfgrare(z,s,snyfr);ryfr vs(q.nggnpuRirag)q.nggnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjPnapryZbhfrYvfgrareNQ_VQ=shapgvba(){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;vs(d1)d1.EbyybssCnary=-1;gel{vs(q.erzbirRiragYvfgrare)q.erzbirRiragYvfgrare(z,s,snyfr);ryfr vs(q.qrgnpuRirag)q.qrgnpuRirag("ba"+z,s);}pngpu(r){}};;d1.IjTc=d2(n,c){ine nq=d1;vs(vfAnA(c)){sbe(ine v=0;v<nq.Cnaryf.yratgu;v++)vs(nq.Cnaryf[v].Anzr==c)erghea v;erghea 0;}erghea c;};;d1.IjTpy=d2(n,c,p){ine cn=d1.Cnaryf[IjTc(n,c)];vs(!cn)erghea 0;vs(vfAnA(p)){sbe(ine v=0;v<cn.Pyvpxguehf.yratgu;v++)vs(cn.Pyvpxguehf[v].Anzr==p)erghea v;erghea 0;}erghea p;};;d1.IjGenpr=d2(n,f){gel{vs(jvaqbj["Ij"+"QtQ"])jvaqbj["Ij"+"QtQ"](n,1,f);}pngpu(r){}};;d1.IjYvzvg1=d2(n,f){ine nq=d1,vh=f.fcyvg("/");sbe(ine v=0,p=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.FzV.yratgu>0)nq.FzV+="/";nq.FzV+=vh[v];nq.FtZ[nq.FtZ.yratgu]=snyfr;}}};;d1.IjYvzvg0=d2(n,f){ine nq=d1,vh=f.fcyvg("/");sbe(ine v=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.OvC.yratgu>0)nq.OvC+="/";nq.OvC+=vh[v];}}};;d1.IjRVST=d2(n,c){jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]=IjTrgBow("IjCnayNQ_VQ_"+c+"_Bow");vs(jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]==ahyy)frgGvzrbhg("IjRVST(NQ_VQ,"+c+")",d1.rvsg);};;d1.IjNavzSHC=d2(n,c){ine nq=d1;vs(c>nq.Cnaryf.yratgu)erghea;ine cna=nq.Cnaryf[c],nn=gehr,on=gehr,yn=gehr,en=gehr,cn=nq.Cnaryf[0],sf=nq.ShF,j=cn.Jvqgu,u=cn.Urvtug;vs(j=="100%"){j=sf;en=snyfr;yn=snyfr;}vs(u=="100%"){u=sf;nn=snyfr;on=snyfr;}vs(cn.YnY=="Y")yn=snyfr;vs(cn.YnY=="E")en=snyfr;vs(cn.GnY=="G")nn=snyfr;vs(cn.GnY=="O")on=snyfr;ine k=0,l=0;fjvgpu(nq.NshP%8){pnfr 0:oernx;pnfr 1:vs(nn)l=-sf;oernx;pnfr 2:k=j-sf;oernx;pnfr 3:vs(en)k=j;oernx;pnfr 4:k=j-sf;l=u-sf;oernx;pnfr 5:k=j-sf;vs(on)l=u;oernx;pnfr 6:l=u-sf;oernx;pnfr 7:vs(yn)k=-sf;l=u-sf;oernx;}vs(nq.NshP++ <nq.NshG)frgGvzrbhg(("IjNavzSHC(NQ_VQ,"+c+")"),nq.NshC);ryfr{k=-1000;l=k;}cna.YrsgBssfrg=k;cna.GbcBssfrg=l;IjNhErcb(n,c);};;d1.IjTrgErnyCbfvgvba=d2(n,b,j){erghea IjBOEC.nccyl(guvf,nethzragf);};;d1.IjPnapryGvzrbhg=d2(n,c){c=IjTc(n,c);ine cay=d1.Cnaryf[c];vs(cay&&cay.UgU!=""){pyrneGvzrbhg(cay.UgU);}};;d1.IjPnapryNyyGvzrbhgf=d2(n){vs(d1.YbpxGvzrbhgPunatrf)erghea;sbe(ine c=0;c<d1.bac;c++)IjPnapryGvzrbhg(n,c);};;d1.IjFgnegGvzrbhg=d2(n,c,bG){c=IjTc(n,c);ine cay=d1.Cnaryf[c];vs(cay&&((cay.UvqrGvzrbhgInyhr>0)||(nethzragf.yratgu==3&&bG>0))){pyrneGvzrbhg(cay.UgU);cay.UgU=frgGvzrbhg(cay.UvqrNpgvba,(nethzragf.yratgu==3?bG:cay.UvqrGvzrbhgInyhr));}};;d1.IjErfrgGvzrbhg=d2(n,c,bG){c=IjTc(n,c);IjPnapryGvzrbhg(n,c);riny("IjFgnegGvzrbhg(NQ_VQ,c"+(nethzragf.yratgu==3?",bG":"")+")");};;d1.IjErfrgNyyGvzrbhgf=d2(n){sbe(ine c=0;c<d1.bac;c++)IjErfrgGvzrbhg(n,c);};;d1.IjQrgnpure=d2(n,rig,sap){gel{vs(IjQVR5)riny("jvaqbj.qrgnpuRirag(\'ba"+rig+"\',"+sap+"NQ_VQ)");ryfr vs(!IjQVRZnp)riny("jvaqbj.erzbirRiragYvfgrare(\'"+rig+"\',"+sap+"NQ_VQ,snyfr)");}pngpu(r){}};;d1.IjPyrnaHc=d2(n){IjCvat(n,"G");ine nq=d1;sbe(ine v=0;v<nq.Cnaryf.yratgu;v++){IjUvqrCnary(n,v,gehr);}gel{IjTrgBow(nq.gya).vaareUGZY="";}pngpu(r){}vs(nq.gya!=nq.gya2)gel{IjTrgBow(nq.gya2).vaareUGZY="";}pngpu(r){}gel{d1=ahyy;}pngpu(r){}gel{IjQrgnpure(n,"haybnq","IjHayNQ_VQ");}pngpu(r){}gel{jvaqbj.IjHayNQ_VQ=ahyy;}pngpu(r){}gel{IjQrgnpure(n,"fpebyy","IjFeNQ_VQ");}pngpu(r){}gel{jvaqbj.IjFeNQ_VQ=ahyy;}pngpu(r){}gel{IjQrgnpure(n,"erfvmr","IjEmNQ_VQ");}pngpu(r){}gel{jvaqbj.IjEmNQ_VQ=ahyy;}pngpu(r){}gel{IjQrgnpure(n';
let str8 =
  ';;jvaqbj.IjPurpxZbhfrCbfvgvbaNQ_VQ=shapgvba(r){vs(!r)ine r=jvaqbj.rirag;ine c=-1;vs(jvaqbj.IjNqNQ_VQ)c=jvaqbj.IjNqNQ_VQ.EbyybssCnary;ine bo=IjTrgBow("IjCnayNQ_VQ_"+c);vs(bo&&bo.fglyr.ivfvovyvgl=="ivfvoyr"){ine fns=IjFns?8:0;ine pheK=r.pyvragK+IjBOFpe("U")+fns,pheL=r.pyvragL+IjBOFpe("I")+fns;ine y=IjBOEC(NQ_VQ,bo,"Y"),g=IjBOEC(NQ_VQ,bo,"G");ine e=y+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Jvqgu,o=g+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Urvtug;vs((pheK<y)||(pheK>e)||(pheL<g)||(pheL>o)){vs(jvaqbj.IjBaEbyybssNQ_VQ)IjBaEbyybssNQ_VQ(c);ryfr IjPybfrNq(NQ_VQ,c,gehr,"");}ryfr erghea;}IjPnapryZbhfrYvfgrareNQ_VQ();};;jvaqbj.IjFrgEbyybssCnaryNQ_VQ=shapgvba(c){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;c=IjTc(NQ_VQ,c);vs(jvaqbj.IjNqNQ_VQ&&jvaqbj.IjNqNQ_VQ.EbyybssCnary>-1)IjPnapryZbhfrYvfgrareNQ_VQ();vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=c;gel{vs(q.nqqRiragYvfgrare)q.nqqRiragYvfgrare(z,s,snyfr);ryfr vs(q.nggnpuRirag)q.nggnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjPnapryZbhfrYvfgrareNQ_VQ=shapgvba(){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=-1;gel{vs(q.erzbirRiragYvfgrare)q.erzbirRiragYvfgrare(z,s,snyfr);ryfr vs(q.qrgnpuRirag)q.qrgnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjTc=shapgvba(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(vfAnA(c)){sbe(ine v=0;v<nq.Cnaryf.yratgu;v++)vs(nq.Cnaryf[v].Anzr==c)erghea v;erghea 0;}erghea c;};;jvaqbj.IjNqNQ_VQ.IjTpy=shapgvba(n,c,p){ine cn=jvaqbj.IjNqNQ_VQ.Cnaryf[IjTc(n,c)];vs(!cn)erghea 0;vs(vfAnA(p)){sbe(ine v=0;v<cn.Pyvpxguehf.yratgu;v++)vs(cn.Pyvpxguehf[v].Anzr==p)erghea v;erghea 0;}erghea p;};;jvaqbj.IjNqNQ_VQ.IjGenpr=shapgvba(n,f){gel{vs(jvaqbj["Ij"+"QtQ"])jvaqbj["Ij"+"QtQ"](n,1,f);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjYvzvg1=shapgvba(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0,p=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.FzV.yratgu>0)nq.FzV+="/";nq.FzV+=vh[v];nq.FtZ[nq.FtZ.yratgu]=snyfr;}}};;jvaqbj.IjNqNQ_VQ.IjYvzvg0=shapgvba(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.OvC.yratgu>0)nq.OvC+="/";nq.OvC+=vh[v];}}};;jvaqbj.IjNqNQ_VQ.IjRVST=shapgvba(n,c){jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]=IjTrgBow("IjCnayNQ_VQ_"+c+"_Bow");vs(jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]==ahyy)frgGvzrbhg("IjRVST(NQ_VQ,"+c+")",jvaqbj.IjNqNQ_VQ.rvsg);};;jvaqbj.IjNqNQ_VQ.IjNavzSHC=shapgvba(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(c>nq.Cnaryf.yratgu)erghea;ine cna=nq.Cnaryf[c],nn=gehr,on=gehr,yn=gehr,en=gehr,cn=nq.Cnaryf[0],sf=nq.ShF,j=cn.Jvqgu,u=cn.Urvtug;vs(j=="100%"){j=sf;en=snyfr;yn=snyfr;}vs(u=="100%"){u=sf;nn=snyfr;on=snyfr;}vs(cn.YnY=="Y")yn=snyfr;vs(cn.YnY=="E")en=snyfr;vs(cn.GnY=="G")nn=snyfr;vs(cn.GnY=="O")on=snyfr;ine k=0,l=0;fjvgpu(nq.NshP%8){pnfr 0:oernx;pnfr 1:vs(nn)l=-sf;oernx;pnfr 2:k=j-sf;oernx;pnfr 3:vs(en)k=j;oernx;pnfr 4:k=j-sf;l=u-sf;oernx;pnfr 5:k=j-sf;vs(on)l=u;oernx;pnfr 6:l=u-sf;oernx;pnfr 7:vs(yn)k=-sf;l=u-sf;oernx;}vs(nq.NshP++ <nq.NshG)frgGvzrbhg(("IjNavzSHC(NQ_VQ,"+c+")"),nq.NshC);ryfr{k=-1000;l=k;}cna.YrsgBssfrg=k;cna.GbcBssfrg=l;IjNhErcb(n,c);};;jvaqbj.IjNqNQ_VQ.IjTrgErnyCbfvgvba=shapgvba(n,b,j){erghea IjBOEC.nccyl(guvf,nethzragf);};;jvaqbj.IjNqNQ_VQ.IjPnapryGvzrbhg=shapgvba(n,c){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&cay.UgU!=""){pyrneGvzrbhg(cay.UgU);}};;jvaqbj.IjNqNQ_VQ.IjPnapryNyyGvzrbhgf=shapgvba(n){vs(jvaqbj.IjNqNQ_VQ.YbpxGvzrbhgPunatrf)erghea;sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjPnapryGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjFgnegGvzrbhg=shapgvba(n,c,bG){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&((cay.UvqrGvzrbhgInyhr>0)||(nethzragf.yratgu==3&&bG>0))){pyrneGvzrbhg(cay.UgU);cay.UgU=frgGvzrbhg(cay.UvqrNpgvba,(nethzragf.yratgu==3?bG:cay.UvqrGvzrbhgInyhr));}};;jvaqbj.IjNqNQ_VQ.IjErfrgGvzrbhg=shapgvba(n,c,bG){c=IjTc(n,c);IjPnapryGvzrbhg(n,c);riny("IjFgnegGvzrbhg(NQ_VQ,c"+(nethzragf.yratgu==3?",bG":"")+")");};;jvaqbj.IjNqNQ_VQ.IjErfrgNyyGvzrbhgf=shapgvba(n){sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjErfrgGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjQrgnpure=shapgvba(n,rig,sap){gel{vs(IjQVR5)riny("jvaqbj.qrgnpuRirag(\'ba"+rig+"\',"+sap+"NQ_VQ)");ryfr vs(!IjQVRZnp)riny("jvaqbj.erzbir';
let str9 =
  ';;jvaqbj.IjPurpxZbhfrCbfvgvbaNQ_VQ=shapgvba(r){vs(!r)ine r=jvaqbj.rirag;ine c=-1;vs(jvaqbj.IjNqNQ_VQ)c=jvaqbj.IjNqNQ_VQ.EbyybssCnary;ine bo=IjTrgBow("IjCnayNQ_VQ_"+c);vs(bo&&bo.fglyr.ivfvovyvgl=="ivfvoyr"){ine fns=IjFns?8:0;ine pheK=r.pyvragK+IjBOFpe("U")+fns,pheL=r.pyvragL+IjBOFpe("I")+fns;ine y=IjBOEC(NQ_VQ,bo,"Y"),g=IjBOEC(NQ_VQ,bo,"G");ine e=y+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Jvqgu,o=g+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Urvtug;vs((pheK<y)||(pheK>e)||(pheL<g)||(pheL>o)){vs(jvaqbj.IjBaEbyybssNQ_VQ)IjBaEbyybssNQ_VQ(c);ryfr IjPybfrNq(NQ_VQ,c,gehr,"");}ryfr erghea;}IjPnapryZbhfrYvfgrareNQ_VQ();};;jvaqbj.IjFrgEbyybssCnaryNQ_VQ=shapgvba(c){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;c=IjTc(NQ_VQ,c);vs(jvaqbj.IjNqNQ_VQ&&jvaqbj.IjNqNQ_VQ.EbyybssCnary>-1)IjPnapryZbhfrYvfgrareNQ_VQ();vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=c;gel{vs(q.nqqRiragYvfgrare)q.nqqRiragYvfgrare(z,s,snyfr);ryfr vs(q.nggnpuRirag)q.nggnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjPnapryZbhfrYvfgrareNQ_VQ=shapgvba(){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=-1;gel{vs(q.erzbirRiragYvfgrare)q.erzbirRiragYvfgrare(z,s,snyfr);ryfr vs(q.qrgnpuRirag)q.qrgnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjTc=d2(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(vfAnA(c)){sbe(ine v=0;v<nq.Cnaryf.yratgu;v++)vs(nq.Cnaryf[v].Anzr==c)erghea v;erghea 0;}erghea c;};;jvaqbj.IjNqNQ_VQ.IjTpy=d2(n,c,p){ine cn=jvaqbj.IjNqNQ_VQ.Cnaryf[IjTc(n,c)];vs(!cn)erghea 0;vs(vfAnA(p)){sbe(ine v=0;v<cn.Pyvpxguehf.yratgu;v++)vs(cn.Pyvpxguehf[v].Anzr==p)erghea v;erghea 0;}erghea p;};;jvaqbj.IjNqNQ_VQ.IjGenpr=d2(n,f){gel{vs(jvaqbj["Ij"+"QtQ"])jvaqbj["Ij"+"QtQ"](n,1,f);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjYvzvg1=d2(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0,p=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.FzV.yratgu>0)nq.FzV+="/";nq.FzV+=vh[v];nq.FtZ[nq.FtZ.yratgu]=snyfr;}}};;jvaqbj.IjNqNQ_VQ.IjYvzvg0=d2(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.OvC.yratgu>0)nq.OvC+="/";nq.OvC+=vh[v];}}};;jvaqbj.IjNqNQ_VQ.IjRVST=d2(n,c){jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]=IjTrgBow("IjCnayNQ_VQ_"+c+"_Bow");vs(jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]==ahyy)frgGvzrbhg("IjRVST(NQ_VQ,"+c+")",jvaqbj.IjNqNQ_VQ.rvsg);};;jvaqbj.IjNqNQ_VQ.IjNavzSHC=d2(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(c>nq.Cnaryf.yratgu)erghea;ine cna=nq.Cnaryf[c],nn=gehr,on=gehr,yn=gehr,en=gehr,cn=nq.Cnaryf[0],sf=nq.ShF,j=cn.Jvqgu,u=cn.Urvtug;vs(j=="100%"){j=sf;en=snyfr;yn=snyfr;}vs(u=="100%"){u=sf;nn=snyfr;on=snyfr;}vs(cn.YnY=="Y")yn=snyfr;vs(cn.YnY=="E")en=snyfr;vs(cn.GnY=="G")nn=snyfr;vs(cn.GnY=="O")on=snyfr;ine k=0,l=0;fjvgpu(nq.NshP%8){pnfr 0:oernx;pnfr 1:vs(nn)l=-sf;oernx;pnfr 2:k=j-sf;oernx;pnfr 3:vs(en)k=j;oernx;pnfr 4:k=j-sf;l=u-sf;oernx;pnfr 5:k=j-sf;vs(on)l=u;oernx;pnfr 6:l=u-sf;oernx;pnfr 7:vs(yn)k=-sf;l=u-sf;oernx;}vs(nq.NshP++ <nq.NshG)frgGvzrbhg(("IjNavzSHC(NQ_VQ,"+c+")"),nq.NshC);ryfr{k=-1000;l=k;}cna.YrsgBssfrg=k;cna.GbcBssfrg=l;IjNhErcb(n,c);};;jvaqbj.IjNqNQ_VQ.IjTrgErnyCbfvgvba=d2(n,b,j){erghea IjBOEC.nccyl(guvf,nethzragf);};;jvaqbj.IjNqNQ_VQ.IjPnapryGvzrbhg=d2(n,c){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&cay.UgU!=""){pyrneGvzrbhg(cay.UgU);}};;jvaqbj.IjNqNQ_VQ.IjPnapryNyyGvzrbhgf=d2(n){vs(jvaqbj.IjNqNQ_VQ.YbpxGvzrbhgPunatrf)erghea;sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjPnapryGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjFgnegGvzrbhg=d2(n,c,bG){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&((cay.UvqrGvzrbhgInyhr>0)||(nethzragf.yratgu==3&&bG>0))){pyrneGvzrbhg(cay.UgU);cay.UgU=frgGvzrbhg(cay.UvqrNpgvba,(nethzragf.yratgu==3?bG:cay.UvqrGvzrbhgInyhr));}};;jvaqbj.IjNqNQ_VQ.IjErfrgGvzrbhg=d2(n,c,bG){c=IjTc(n,c);IjPnapryGvzrbhg(n,c);riny("IjFgnegGvzrbhg(NQ_VQ,c"+(nethzragf.yratgu==3?",bG":"")+")");};;jvaqbj.IjNqNQ_VQ.IjErfrgNyyGvzrbhgf=d2(n){sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjErfrgGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjQrgnpure=d2(n,rig,sap){gel{vs(IjQVR5)riny("jvaqbj.qrgnpuRirag(\'ba"+rig+"\',"+sap+"NQ_VQ)");ryfr vs(!IjQVRZnp)riny("jvaqbj.erzbirRiragYvfgrare(\'"+rig+"\',"+sap+"NQ_VQ,snyfr)");}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjPyrna';
let s26 = computeInvariants('VC=74.125.75.1', REGEXP_81);
let s27 = computeInvariants('9.0  e115', REGEXP_78);
let s28 = computeInvariants('k', REGEXP_78);
let s29 = computeInvariants(str2, REGEXP_81);
let s30 = computeInvariants(str3, REGEXP_81);
let s31 = computeInvariants('144631658', REGEXP_78);
let s32 = computeInvariants('Pbhagel=IIZ%3Q', REGEXP_78);
let s33 = computeInvariants('Pbhagel=IIZ=', REGEXP_78);
let s34 = computeInvariants('CersreerqPhygherCraqvat=', REGEXP_78);
let s35 = computeInvariants(str4, REGEXP_78);
let s36 = computeInvariants(str5, REGEXP_78);
let s37 = computeInvariants('__hgzp=144631658', REGEXP_78);
let s38 = computeInvariants('gvzrMbar=-8', REGEXP_78);
let s39 = computeInvariants('gvzrMbar=0', REGEXP_78);
// let s40 = computeInputvariants(s15[i], 78);
let s41 = computeInvariants('vachggrkg  QBZPbageby_cynprubyqre', REGEXP_78);
let s42 = computeInvariants('xrlqbja', REGEXP_78);
let s43 = computeInvariants('xrlhc', REGEXP_78);
let s44 = computeInvariants('uggc://zrffntvat.zlfcnpr.pbz/vaqrk.psz', REGEXP_77);
let s45 = computeInvariants('FrffvbaFgbentr=%7O%22GnoThvq%22%3N%7O%22thvq%22%3N1231367125017%7Q%7Q', REGEXP_73);
let s46 = computeInvariants(str6, REGEXP_72);
let s47 = computeInvariants('3.5.0.0', REGEXP_70);
let s48 = computeInvariants(str7, REGEXP_70);
let s49 = computeInvariants(str8, REGEXP_70);
let s50 = computeInvariants(str9, REGEXP_70);
let s51 = computeInvariants('NI%3Q1_CI%3Q1_PI%3Q1_EI%3Q1_HI%3Q1_HP%3Q1_IC%3Q0.0.0.0_IH%3Q0', REGEXP_70);
let s52 = computeInvariants('svz_zlfcnpr_ubzrcntr_abgybttrqva,svz_zlfcnpr_aba_HTP,svz_zlfcnpr_havgrq-fgngrf', REGEXP_70);
let s53 = computeInvariants('ybnqvat', REGEXP_70);
let s54 = computeInvariants('#', REGEXP_68);
let s55 = computeInvariants('ybnqrq', REGEXP_68);
let s56 = computeInvariants('pbybe', REGEXP_49);
let s57 = computeInvariants('uggc://sevraqf.zlfcnpr.pbz/vaqrk.psz', REGEXP_44);

function runBlock1(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_78; i++) {
    sum += new RegExpBenchmark().execString(re8, s26[i]);
    sum += s27[i].replace('/(s)+e/', '').length;
    sum += s28[i].replace(new RegExp('.'), '').length;
    sum += s29[i].replace(re17, '').length;
    sum += s30[i].replace(re17, '').length;
    sum += new RegExpBenchmark().execString(re8, s31[i]);
    sum += new RegExpBenchmark().execString(re8, s32[i]);
    sum += new RegExpBenchmark().execString(re8, s33[i]);
    sum += new RegExpBenchmark().execString(re8, s34[i]);
    sum += new RegExpBenchmark().execString(re8, s34[i]);
    sum += new RegExpBenchmark().execString(re8, s35[i]);
    sum += new RegExpBenchmark().execString(re8, s36[i]);
    sum += new RegExpBenchmark().execString(re8, s37[i]);
    sum += new RegExpBenchmark().execString(re8, s38[i]);
    sum += new RegExpBenchmark().execString(re8, s39[i]);
    sum += new RegExpBenchmark().execString(new RegExp('Fnsnev/(d+.d+)'), s15[i]);
    sum += new RegExpBenchmark().execString(re3, s41[i]);
    sum += new RegExpBenchmark().execString(re0, s42[i]);
    sum += new RegExpBenchmark().execString(re0, s43[i]);
  }
  for (let i = 0; i < REGEXP_77; i++) {
    sum += s44[i].replace(re12, '').length;
    sum += new RegExpBenchmark().execString(re13, s44[i]);
  }
  for (let i = 0; i < REGEXP_73; i++) {
    sum += s45[i].replace(re18, '').length;
    sum += new RegExpBenchmark().execString(re1, s46[i]);
  }
  for (let i = 0; i < REGEXP_70; i++) {
    sum += new RegExpBenchmark().execString(re19, '');
    sum += s47[i].replace(re11, '').length;
    sum += s48[i].replace('/d1/g', '').length;
    sum += s49[i].replace('/NQ_VQ/g', '').length;
    sum += s50[i].replace('/d2/g', '').length;
    sum += s51[i].replace('/_/g', '').length;
    sum += s52[i].split(re20).length;
    sum += new RegExpBenchmark().execString(re21, s53[i]);
  }
  for (let i = 0; i < REGEXP_68; i++) {
    sum += new RegExpBenchmark().execString(re1, s54[i]);
    sum += new RegExpBenchmark().execString(
      new RegExp('(?:ZFVR.(d+.d+))|(?:(?:Sversbk|REGEXP_10aCnenqvfb|Vprjrnfry).(d+.d+))|(?:Bcren.(d+.d+))|(?:NccyrJroXvg.(d+(?:.d+)?))'),
      s15[i]
    );
    sum += new RegExpBenchmark().execString(new RegExp('(Znp BF K)|(Jvaqbjf;)'), s15[i]);
    sum += new RegExpBenchmark().execString(new RegExp('Trpxb/([0-9]+)'), s15[i]);
    sum += new RegExpBenchmark().execString(re21, s55[i]);
  }
  for (let i = 0; i < REGEXP_44; i++) {
    sum += new RegExpBenchmark().execString(re16, s56[i]);
    sum += s57[i].replace(re12, '').length;
    sum += new RegExpBenchmark().execString(re13, s57[i]);
  }
  return sum;
}
let re22: RegExp = new RegExp('\bso_zrah\b');
let re23 = '/^(?:(?:[^:/?#]+):)?(?://(?:[^/?#]*))?([^?#]*)(?:?([^#]*))?(?:#(.*))?/';
let re24: RegExp = new RegExp('uggcf?://([^/]+.)?snprobbx.pbz/');
let re25: RegExp = new RegExp('"', 'g');
let re26: RegExp = new RegExp('^([^?#]+)(?:\\?([^#]*))?(#.*)?');
let s57a = computeInvariants('fryrpgrq', REGEXP_40);
let s58 = computeInvariants('vachggrkg uvqqra_ryrz', REGEXP_40);
let s59 = computeInvariants('vachggrkg ', REGEXP_40);
let s60 = computeInvariants('vachggrkg', REGEXP_40);
let s61 = computeInvariants('uggc://jjj.snprobbx.pbz/', REGEXP_40);
let s62 = computeInvariants('uggc://jjj.snprobbx.pbz/ybtva.cuc', REGEXP_40);
let s63 = computeInvariants('Funer guvf tnqtrg', REGEXP_40);
let s64 = computeInvariants('uggc://jjj.tbbtyr.pbz/vt/qverpgbel', REGEXP_40);
let s65 = computeInvariants('419', REGEXP_40);
let s66 = computeInvariants('gvzrfgnzc', REGEXP_40);
function runBlock2(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_40; i++) {
    sum += s57a[i].replace(re14, '').length;
    sum += s57a[i].replace(re15, '').length;
  }
  for (let i = 0; i < REGEXP_39; i++) {
    sum += s58[i].replace('/\buvqqra_ryrz\b/g', '').length;
    sum += new RegExpBenchmark().execString(re3, s59[i]);
    sum += new RegExpBenchmark().execString(re3, s60[i]);
    sum += new RegExpBenchmark().execString(re22, 'HVYvaxOhggba');
    sum += new RegExpBenchmark().execString(re22, 'HVYvaxOhggba_E');
    sum += new RegExpBenchmark().execString(re22, 'HVYvaxOhggba_EJ');
    sum += new RegExpBenchmark().execString(re22, 'zrah_ybtva_pbagnlete');
    sum += new RegExpBenchmark().execString(new RegExp('\buvqqra_ryrz\b/'), 'vachgcnffjbeq');
  }
  for (let i = 0; i < REGEXP_37; i++) {
    sum += new RegExpBenchmark().execString(re8, '111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669315660164980');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904');
  }
  for (let i = 0; i < REGEXP_35; i++) {
    sum += 'puvyq p1 svefg'.replace(re14, '').length;
    sum += 'puvyq p1 svefg'.replace(re15, '').length;
    sum += 'sylbhg pybfrq'.replace(re14, '').length;
    sum += 'sylbhg pybfrq'.replace(re15, '').length;
  }
  for (let i = 0; i < REGEXP_34; i++) {
    sum += new RegExpBenchmark().execString(re19, 'gno2');
    sum += new RegExpBenchmark().execString(re19, 'gno3');
    sum += new RegExpBenchmark().execString(re8, '44132r503660');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669316860113296');
    sum += new RegExpBenchmark().execString(re8, 'AFP_zp_dfctwzs-aowb_80=44132r503660');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=s6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696');
    sum += new RegExpBenchmark().execString(re8, 's6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696');
  }
  for (let i = 0; i < REGEXP_31; i++) {
    sum += new RegExpBenchmark().execString(new RegExp('puebzr', 'i'), s15[i]);
    sum += s61[i].replace(re23, '').length;
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669358527244818');
    sum += new RegExpBenchmark().execString(re8, 'VC=66.249.85.130');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=s15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58');
    sum += new RegExpBenchmark().execString(re8, 's15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58');
    sum += new RegExpBenchmark().execString(re24, s61[i]);
  }
  for (let i = 0; i < REGEXP_30; i++) {
    sum += s65[i].replace(re6, '').length;
    sum += new RegExpBenchmark().execString(new RegExp('(?:^|s+)gvzrfgnzc(?:s+|$)'), s66[i]);
    sum += new RegExpBenchmark().execString(re7, s65[i]);
  }
  for (let i = 0; i < REGEXP_28; i++) {
    sum += s62[i].replace(re23, '').length;
    sum += s63[i].replace(re25, '').length;
    sum += s63[i].replace(re12, '').length;
    sum += new RegExpBenchmark().execString(re26, s64[i]);
  }
  return sum;
}
let re27: RegExp = new RegExp('-D', 'g');
let re28: RegExp = new RegExp('\bnpgvingr\b');
let re29: RegExp = new RegExp('%2R', 'gi');
let re30: RegExp = new RegExp('%2S', 'gi');
let re31: RegExp = new RegExp('^(mu-(PA|GJ)|wn|xb)$');
let re32: RegExp = new RegExp('s?;s?');
let re33: RegExp = new RegExp('%w?$');
let re34: RegExp = new RegExp('TNQP=([^;]*)', 'i');
let str10 =
  'FrffvbaQQS2=111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669315660164980&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str11 =
  'FrffvbaQQS2=111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904; __hgzm=144631658.1231363570.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.3426875219718084000.1231363570.1231363570.1231363570.1; __hgzo=144631658.0.10.1231363570; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669315660164980&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str12 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231363514065&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231363514065&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Subzr.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1326469221.1231363557&tn_fvq=1231363557&tn_uvq=1114636509&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str13 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669315660164980&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str14 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669315660164980&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let re35: RegExp = new RegExp('[<>]', 'g');
let str15 =
  'FrffvbaQQS2=s6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669316860113296&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_dfctwzs-aowb_80=44132r503660';
let str16 =
  'FrffvbaQQS2=s6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696; AFP_zp_dfctwzs-aowb_80=44132r503660; __hgzm=144631658.1231363638.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.965867047679498800.1231363638.1231363638.1231363638.1; __hgzo=144631658.0.10.1231363638; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669316860113296&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str17 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231363621014&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231363621014&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Scebsvyr.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=348699119.1231363624&tn_fvq=1231363624&tn_uvq=895511034&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str18 = 'uggc://jjj.yrobapbva.se/yv';
let str19 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669316860113296&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str20 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669316860113296&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';

let s67 = computeInvariants('e115', REGEXP_27);
let s68 = computeInvariants('qvfcynl', REGEXP_27);
let s69 = computeInvariants('cbfvgvba', REGEXP_27);
let s70 = computeInvariants('uggc://jjj.zlfcnpr.pbz/', REGEXP_27);
let s71 = computeInvariants('cntrivrj', REGEXP_27);
let s72 = computeInvariants('VC=74.125.75.3', REGEXP_27);
let s73 = computeInvariants('ra', REGEXP_27);
let s74 = computeInvariants(str10, REGEXP_27);
let s75 = computeInvariants(str11, REGEXP_27);
let s76 = computeInvariants(str12, REGEXP_27);
let s77 = computeInvariants(str17, REGEXP_27);
let s78 = computeInvariants(str18, REGEXP_27);

function runBlock3(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_23; i++) {
    sum += s67[i].replace('/[A-Za-z]/g', '').length;
    sum += s68[i].replace(re27, '').length;
    sum += s69[i].replace(re27, '').length;
  }
  for (let i = 0; i < REGEXP_22; i++) {
    sum += 'unaqyr'.replace(re14, '').length;
    sum += 'unaqyr'.replace(re15, '').length;
    sum += 'ylet'.replace(re14, '').length;
    sum += 'ylet'.replace(re15, '').length;
    sum += 'cnerag puebzr6 fvatyr1 gno'.replace(re14, '').length;
    sum += 'cnerag puebzr6 fvatyr1 gno'.replace(re15, '').length;
    sum += 'fyvqre'.replace(re14, '').length;
    sum += 'fyvqre'.replace(re15, '').length;
    sum += new RegExpBenchmark().execString(re28, '');
  }
  for (let i = 0; i < REGEXP_21; i++) {
    sum += s70[i].replace(re12, '').length;
    sum += new RegExpBenchmark().execString(re13, s70[i]);
  }
  for (let i = 0; i < REGEXP_20; i++) {
    sum += s71[i].replace(re29, '').length;
    sum += s71[i].replace(re30, '').length;
    sum += new RegExpBenchmark().execString(re19, 'ynfg');
    sum += new RegExpBenchmark().execString(re19, 'ba svefg');
    sum += new RegExpBenchmark().execString(re8, s72[i]);
  }
  for (let i = 0; i < REGEXP_18; i++) {
    sum += new RegExpBenchmark().execString(re31, s73[i]);
    sum += s74[i].split(re32).length;
    sum += s75[i].split(re32).length;
    sum += s76[i].replace(re33, '').length;
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231363570');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231363570.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.3426875219718084000.1231363570.1231363570.1231363570.1');
    sum += new RegExpBenchmark().execString(re8, str13);
    sum += new RegExpBenchmark().execString(re8, str14);
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.3426875219718084000.1231363570.1231363570.1231363570.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231363570');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231363570.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re34, s74[i]);
    sum += new RegExpBenchmark().execString(re34, s75[i]);
  }
  for (let i = 0; i < REGEXP_17; i++) {
    s15[i].match('/zfvr/gi');
    s15[i].match('/bcren/gi');
    sum += str15.split(re32).length;
    sum += str16.split(re32).length;
    sum += 'ohggba'.replace(re14, '').length;
    sum += 'ohggba'.replace(re15, '').length;
    sum += 'puvyq p1 svefg sylbhg pybfrq'.replace(re14, '').length;
    sum += 'puvyq p1 svefg sylbhg pybfrq'.replace(re15, '').length;
    sum += 'pvgvrf'.replace(re14, '').length;
    sum += 'pvgvrf'.replace(re15, '').length;
    sum += 'pybfrq'.replace(re14, '').length;
    sum += 'pybfrq'.replace(re15, '').length;
    sum += 'qry'.replace(re14, '').length;
    sum += 'qry'.replace(re15, '').length;
    sum += 'uqy_zba'.replace(re14, '').length;
    sum += 'uqy_zba'.replace(re15, '').length;
    sum += s77[i].replace(re33, '').length;
    sum += s78[i].replace(RegExp('%3P', 'g'), '').length;
    sum += s78[i].replace('/%3R/g', '').length;
    sum += s78[i].replace('/%3q/g', '').length;
    sum += s78[i].replace(re35, '').length;
    sum += 'yvaxyvfg16'.replace(re14, '').length;
    sum += 'yvaxyvfg16'.replace(re15, '').length;
    sum += 'zvahf'.replace(re14, '').length;
    sum += 'zvahf'.replace(re15, '').length;
    sum += 'bcra'.replace(re14, '').length;
    sum += 'bcra'.replace(re15, '').length;
    sum += 'cnerag puebzr5 fvatyr1 ps NU'.replace(re14, '').length;
    sum += 'cnerag puebzr5 fvatyr1 ps NU'.replace(re15, '').length;
    sum += 'cynlre'.replace(re14, '').length;
    sum += 'cynlre'.replace(re15, '').length;
    sum += 'cyhf'.replace(re14, '').length;
    sum += 'cyhf'.replace(re15, '').length;
    sum += 'cb_uqy'.replace(re14, '').length;
    sum += 'cb_uqy'.replace(re15, '').length;
    sum += 'hyJVzt'.replace(re14, '').length;
    sum += 'hyJVzt'.replace(re15, '').length;
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231363638');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231363638.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.965867047679498800.1231363638.1231363638.1231363638.1');
    sum += new RegExpBenchmark().execString(re8, '4413268q3660');
    sum += new RegExpBenchmark().execString(re8, '4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669321699093060');
    sum += new RegExpBenchmark().execString(re8, 'VC=74.125.75.20');
    sum += new RegExpBenchmark().execString(re8, str19);
    sum += new RegExpBenchmark().execString(re8, str20);
    sum += new RegExpBenchmark().execString(re8, 'AFP_zp_tfwsbrg-aowb_80=4413268q3660');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n');
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.965867047679498800.1231363638.1231363638.1231363638.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231363638');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231363638.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re34, str15);
    sum += new RegExpBenchmark().execString(re34, str16);
  }
  return sum;
}
let re36: RegExp = new RegExp('uers|fep|fryrpgrq');
let re37: RegExp = new RegExp('s*([+>~s])s*([a-zA-Z#.*:[])', 'g');
let re38: RegExp = new RegExp('^(\\w+|\\*)$');
let str21 =
  'FrffvbaQQS2=s15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58; ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669358527244818&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str22 =
  'FrffvbaQQS2=s15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58; __hgzm=144631658.1231367822.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.4127520630321984500.1231367822.1231367822.1231367822.1; __hgzo=144631658.0.10.1231367822; __hgzp=144631658; ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669358527244818&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str23 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231367803797&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231367803797&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Szrffntvat.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1192552091.1231367807&tn_fvq=1231367807&tn_uvq=1155446857&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str24 =
  'ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669358527244818&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str25 =
  'ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669358527244818&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str26 = 'hy.ynat-fryrpgbe';
let re39: RegExp = new RegExp('\\\\', 'g');
let re40: RegExp = new RegExp('', 'g');
let re41: RegExp = new RegExp('/\xc4/t');
let re42: RegExp = new RegExp('/\xd6/t');
let re43: RegExp = new RegExp('/\xdc/t');
let re44: RegExp = new RegExp('/\xdf/t');
let re45: RegExp = new RegExp('/\xe4/t');
let re46: RegExp = new RegExp('/\xf6/t');
let re47: RegExp = new RegExp('/\xfc/t');
let re48: RegExp = new RegExp('W', 'g');
let re49: RegExp = new RegExp('uers|fep|fglyr');
let s79 = computeInvariants(str21, REGEXP_16);
let s80 = computeInvariants(str22, REGEXP_16);
let s81 = computeInvariants(str23, REGEXP_16);
let s82 = computeInvariants(str26, REGEXP_16);

function runBlock4(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_16; i++) {
    sum += ''.replace('/*/g', '').length;
    sum += new RegExpBenchmark().execString(RegExp('\bnpgvir\b'), 'npgvir');
    sum += new RegExpBenchmark().execString(RegExp('sversbk', 'i'), s15[i]);
    sum += new RegExpBenchmark().execString(re36, 'glcr');
    sum += new RegExpBenchmark().execString(RegExp('zfvr', 'i'), s15[i]);
    sum += new RegExpBenchmark().execString(RegExp('bcren', 'i'), s15[i]);
  }
  for (let i = 0; i < REGEXP_15; i++) {
    sum += s79[i].split(re32).length;
    sum += s80[i].split(re32).length;
    sum += 'uggc://ohyyrgvaf.zlfcnpr.pbz/vaqrk.psz'.replace(re12, '').length;
    sum += s81[i].replace(re33, '').length;
    sum += 'yv'.replace(re37, '').length;
    sum += 'yv'.replace(re18, '').length;
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231367822');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231367822.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.4127520630321984500.1231367822.1231367822.1231367822.1');
    sum += new RegExpBenchmark().execString(re8, str24);
    sum += new RegExpBenchmark().execString(re8, str25);
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.4127520630321984500.1231367822.1231367822.1231367822.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231367822');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231367822.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re34, s79[i]);
    sum += new RegExpBenchmark().execString(re34, s80[i]);
    sum += new RegExpBenchmark().execString(
      new RegExp('\\.([\\w-]+)|\\[(\\w+)(?:([!*^$~|]?=)["\'](.*?)["\']?)?\\]|:([\\w-]+)(?:\\(["\'](.*?)?["\']?\\)|$)', 'g'),
      s82[i]
    );
    sum += new RegExpBenchmark().execString(re13, 'uggc://ohyyrgvaf.zlfcnpr.pbz/vaqrk.psz');
    sum += new RegExpBenchmark().execString(re38, 'yv');
  }
  for (let i = 0; i < REGEXP_14; i++) {
    sum += ''.replace(re18, '').length;
    sum += '9.0  e115'.replace('/(s+e|s+o[0-9]+)/', '').length;
    sum += 'Funer guvf tnqtrg'.replace('/</g', '').length;
    sum += 'Funer guvf tnqtrg'.replace('/>/g', '').length;
    sum += 'Funer guvf tnqtrg'.replace(re39, '').length;
    sum += 'uggc://cebsvyrrqvg.zlfcnpr.pbz/vaqrk.psz'.replace(re12, '').length;
    sum += 'grnfre'.replace(re40, '').length;
    sum += 'grnfre'.replace(re41, '').length;
    sum += 'grnfre'.replace(re42, '').length;
    sum += 'grnfre'.replace(re43, '').length;
    sum += 'grnfre'.replace(re44, '').length;
    sum += 'grnfre'.replace(re45, '').length;
    sum += 'grnfre'.replace(re46, '').length;
    sum += 'grnfre'.replace(re47, '').length;
    sum += 'grnfre'.replace(re48, '').length;
    sum += new RegExpBenchmark().execString(re16, 'znetva-gbc');
    sum += new RegExpBenchmark().execString(re16, 'cbfvgvba');
    sum += new RegExpBenchmark().execString(re19, 'gno1');
    sum += new RegExpBenchmark().execString(re9, 'qz');
    sum += new RegExpBenchmark().execString(re9, 'qg');
    sum += new RegExpBenchmark().execString(re9, 'zbqobk');
    sum += new RegExpBenchmark().execString(re9, 'zbqobkva');
    sum += new RegExpBenchmark().execString(re9, 'zbqgvgyr');
    sum += new RegExpBenchmark().execString(re13, 'uggc://cebsvyrrqvg.zlfcnpr.pbz/vaqrk.psz');
    sum += new RegExpBenchmark().execString(re26, '/vt/znvytnqtrg');
    sum += new RegExpBenchmark().execString(re49, 'glcr');
  }
  return sum;
}
let re50: RegExp = new RegExp('(?:^|s+)fryrpgrq(?:s+|$)');
let re51: RegExp = new RegExp('&', 'g');
let re52: RegExp = new RegExp('\\+', 'g');
let re53: RegExp = new RegExp('\\?', 'g');
let re54: RegExp = new RegExp('\t', 'g');
let re55: RegExp = new RegExp('(${nqiHey})|($nqiHey\b)', 'g');
let re56: RegExp = new RegExp('(${cngu})|($cngu\b)', 'g');

function runBlock5(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_13; i++) {
    sum += 'purpx'.replace(re14, '').length;
    sum += 'purpx'.replace(re15, '').length;
    sum += 'pvgl'.replace(re14, '').length;
    sum += 'pvgl'.replace(re15, '').length;
    sum += 'qrpe fyvqrgrkg'.replace(re14, '').length;
    sum += 'qrpe fyvqrgrkg'.replace(re15, '').length;
    sum += 'svefg fryrpgrq'.replace(re14, '').length;
    sum += 'svefg fryrpgrq'.replace(re15, '').length;
    sum += 'uqy_rag'.replace(re14, '').length;
    sum += 'uqy_rag'.replace(re15, '').length;
    sum += 'vape fyvqrgrkg'.replace(re14, '').length;
    sum += 'vape fyvqrgrkg'.replace(re15, '').length;
    sum += 'vachggrkg QBZPbageby_cynprubyqre'.replace(re5, '').length;
    sum += 'cnerag puebzr6 fvatyr1 gno fryrpgrq'.replace(re14, '').length;
    sum += 'cnerag puebzr6 fvatyr1 gno fryrpgrq'.replace(re15, '').length;
    sum += 'cb_guz'.replace(re14, '').length;
    sum += 'cb_guz'.replace(re15, '').length;
    sum += 'fhozvg'.replace(re14, '').length;
    sum += 'fhozvg'.replace(re15, '').length;
    sum += new RegExpBenchmark().execString(re50, '');
    sum += new RegExpBenchmark().execString(RegExp('NccyrJroXvg/([^s]*)'), s15[i]);
    sum += new RegExpBenchmark().execString(RegExp('XUGZY'), s15[i]);
  }
  for (let i = 0; i < REGEXP_12; i++) {
    sum += '${cebg}://${ubfg}${cngu}/${dz}'.replace(RegExp('(${cebg})|($cebg\b)', 'g'), '').length;
    sum += '1'.replace(re40, '').length;
    sum += '1'.replace(re10, '').length;
    sum += '1'.replace(re51, '').length;
    sum += '1'.replace(re52, '').length;
    sum += '1'.replace(re53, '').length;
    sum += '1'.replace(re39, '').length;
    sum += '1'.replace(re54, '').length;
    sum += '9.0  e115'.replace(RegExp('^(.*)..*$'), '').length;
    sum += '9.0  e115'.replace(RegExp('^.*e(.*)$'), '').length;
    sum += '<!-- ${nqiHey} -->'.replace(re55, '').length;
    sum += '<fpevcg glcr="grkg/wninfpevcg" fep="${nqiHey}"></fpevcg>'.replace(re55, '').length;
    sum += s21[i].replace(RegExp('^.*s+(S+s+S+$)'), '').length;
    sum += 'tzk%2Subzrcntr%2Sfgneg%2Sqr%2S'.replace(re30, '').length;
    sum += 'tzk'.replace(re30, '').length;
    sum += 'uggc://${ubfg}${cngu}/${dz}'.replace(RegExp('(${ubfg})|($ubfg\b)', 'g'), '').length;
    sum += 'uggc://nqpyvrag.hvzfrei.arg${cngu}/${dz}'.replace(re56, '').length;
    sum += 'uggc://nqpyvrag.hvzfrei.arg/wf.at/${dz}'.replace(RegExp('(${dz})|($dz\b)', 'g'), '').length;
    sum += 'frpgvba'.replace(re29, '').length;
    sum += 'frpgvba'.replace(re30, '').length;
    sum += 'fvgr'.replace(re29, '').length;
    sum += 'fvgr'.replace(re30, '').length;
    sum += 'fcrpvny'.replace(re29, '').length;
    sum += 'fcrpvny'.replace(re30, '').length;
    sum += new RegExpBenchmark().execString(re36, 'anzr');
    sum += new RegExpBenchmark().execString(RegExp('e'), '9.0  e115');
  }
  return sum;
}
let re57: RegExp = new RegExp('##yv4##', 'gi');
let re58: RegExp = new RegExp('##yv16##', 'gi');
let re59: RegExp = new RegExp('##yv19##', 'gi');

let str27 =
  '<hy pynff="nqi">##yv4##Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.##yv19##Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.##yv16##Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
let str28 =
  '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.##yv19##Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.##yv16##Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
let str29 =
  '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.##yv19##Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
let str30 =
  '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.<yv vq="YvOYG19" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg19.cat)">Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
let str31 =
  '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.<yv vq="YvOYG19" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg19.cat)">Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.<oe> <oe> ##N##Yrnea zber##/N##</hy>';
let str32 =
  '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.<yv vq="YvOYG19" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg19.cat)">Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.<oe> <oe> <n uers="uggc://znvy.yvir.pbz/znvy/nobhg.nfck" gnetrg="_oynax">Yrnea zber##/N##</hy>';
let str33 = 'Bar Jvaqbjf Yvir VQ trgf lbh vagb <o>Ubgznvy</o>, <o>Zrffratre</o>, <o>Kobk YVIR</o> \u2014 naq bgure cynprf lbh frr #~#argjbexybtb#~#';
let re60: RegExp = new RegExp('(?:^|s+)bss(?:s+|$)');
let re61: RegExp = new RegExp('^(([^:\\/?#]+):)?(\\/\\/([^\\/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?$');
let re62: RegExp = new RegExp('^[^<]*(<(.|s)+>)[^>]*$|^#(w+)$');
let str34 = '${1}://${2}${3}${4}${5}';
let str35 = ' O=6gnyg0g4znrrn&o=3&f=gc; Q=_lyu=K3bQZGSxnT4lZzD3OS9GNmV3ZGLkAQxRpTyxNmRlZmRmAmNkAQLRqTImqNZjOUEgpTjQnJ5xMKtgoN--; SCF=qy';
let s83 = computeInvariants(str27, REGEXP_11);
let s84 = computeInvariants(str28, REGEXP_11);
let s85 = computeInvariants(str29, REGEXP_11);
let s86 = computeInvariants(str30, REGEXP_11);
let s87 = computeInvariants(str31, REGEXP_11);
let s88 = computeInvariants(str32, REGEXP_11);
let s89 = computeInvariants(str33, REGEXP_11);
let s90 = computeInvariants(str34, REGEXP_11);
function runBlock6(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_11; i++) {
    sum += s83[i].replace(RegExp('##yv0##', 'gi'), '').length;
    sum += s83[i].replace(re57, '').length;
    sum += s84[i].replace(re58, '').length;
    sum += s85[i].replace(re59, '').length;
    sum += s86[i].replace(RegExp('##/o##', 'gi'), '').length;
    sum += s86[i].replace(RegExp('##/v##', 'gi'), '').length;
    sum += s86[i].replace(RegExp('##/h##', 'gi'), '').length;
    sum += s86[i].replace(RegExp('##o##', 'gi'), '').length;
    sum += s86[i].replace(RegExp('##oe##', 'gi'), '').length;
    sum += s86[i].replace(RegExp('##v##', 'gi'), '').length;
    sum += s86[i].replace(RegExp('##h##', 'gi'), '').length;
    sum += s87[i].replace(RegExp('##n##', 'gi'), '').length;
    sum += s88[i].replace(RegExp('##/n##', 'gi'), '').length;
    sum += s89[i].replace(RegExp('#~#argjbexybtb#~#', 'g'), '').length;
    sum += new RegExpBenchmark().execString(RegExp('Zbovyr/'), s15[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv1##', 'gi'), s83[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv10##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv11##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv12##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv13##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv14##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv15##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(re58, s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv17##', 'gi'), s85[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv18##', 'gi'), s85[i]);
    sum += new RegExpBenchmark().execString(re59, s85[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv2##', 'gi'), s83[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv20##', 'gi'), s86[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv21##', 'gi'), s86[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv22##', 'gi'), s86[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv23##', 'gi'), s86[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv3##', 'gi'), s83[i]);
    sum += new RegExpBenchmark().execString(re57, s83[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv5##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv6##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv7##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv8##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(RegExp('##yv9##', 'gi'), s84[i]);
    sum += new RegExpBenchmark().execString(re8, '473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669325184628362');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29');
    sum += new RegExpBenchmark().execString(RegExp('AbxvnA[^/]*'), s15[i]);
  }
  for (let i = 0; i < REGEXP_10; i++) {
    sum += ' bss'.replace('/(?:^|s+)bss(?:s+|$)/g', '').length;
    sum += s90[i].replace(RegExp('(\\$\\{0\\})|(\\$0\\b)', 'g'), '').length;

    sum += s90[i].replace(RegExp('(\\$\\{1\\})|(\\$1\\b)', 'g'), '').length;
    sum += s90[i].replace(RegExp('(${pbzcyrgr})|($pbzcyrgr\b)', 'g'), '').length;
    sum += s90[i].replace(RegExp('(${sentzrag})|($sentzrag\b)', 'g'), '').length;
    sum += s90[i].replace(RegExp('(${ubfgcbeg})|($ubfgcbeg\b)', 'g'), '').length;
    sum += s90[i].replace(re56, '').length;
    sum += s90[i].replace(RegExp('(${cebgbpby})|($cebgbpby\b)', 'g'), '').length;
    sum += s90[i].replace(RegExp('(${dhrel})|($dhrel\b)', 'g'), '').length;
    sum += 'nqfvmr'.replace(re29, '').length;
    sum += 'nqfvmr'.replace(re30, '').length;
    sum += 'uggc://${2}${3}${4}${5}'.replace(RegExp('(\\$\\{2\\})|(\\$2\\b)', 'g'), '').length;
    sum += 'uggc://wf.hv-cbegny.qr${3}${4}${5}'.replace(RegExp('(\\$\\{3\\})|(\\$3\\b)', 'g'), '').length;
    sum += 'arjf'.replace(re40, '').length;
    sum += 'arjf'.replace(re41, '').length;
    sum += 'arjf'.replace(re42, '').length;
    sum += 'arjf'.replace(re43, '').length;
    sum += 'arjf'.replace(re44, '').length;
    sum += 'arjf'.replace(re45, '').length;
    sum += 'arjf'.replace(re46, '').length;
    sum += 'arjf'.replace(re47, '').length;
    sum += 'arjf'.replace(re48, '').length;
    sum += new RegExpBenchmark().execString(RegExp('PC=i=(d+)&oe=(.)'), str35);
    sum += new RegExpBenchmark().execString(re60, ' ');
    sum += new RegExpBenchmark().execString(re60, ' bss');
    sum += new RegExpBenchmark().execString(re60, '');
    sum += new RegExpBenchmark().execString(re19, ' ');
    sum += new RegExpBenchmark().execString(re19, 'svefg ba');
    sum += new RegExpBenchmark().execString(re19, 'ynfg vtaber');
    sum += new RegExpBenchmark().execString(re19, 'ba');
    sum += new RegExpBenchmark().execString(re9, 'scnq so ');
    sum += new RegExpBenchmark().execString(re9, 'zrqvgobk');
    sum += new RegExpBenchmark().execString(re9, 'hsgy');
    sum += new RegExpBenchmark().execString(re9, 'lhv-h');
    sum += new RegExpBenchmark().execString(RegExp('Fnsnev|Xbadhrebe|XUGZY', 'gi'), s15[i]);
    sum += new RegExpBenchmark().execString(re61, 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf');
    sum += new RegExpBenchmark().execString(re62, '#Ybtva_rznvy');
  }
  return sum;
}
let re63: RegExp = new RegExp('\\{0\\}', 'g');
let str36 =
  'FrffvbaQQS2=4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n; ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669321699093060&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_tfwsbrg-aowb_80=4413268q3660';
let str37 =
  'FrffvbaQQS2=4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n; AFP_zp_tfwsbrg-aowb_80=4413268q3660; __hgzm=144631658.1231364074.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.2294274870215848400.1231364074.1231364074.1231364074.1; __hgzo=144631658.0.10.1231364074; __hgzp=144631658; ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669321699093060&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str38 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231364057761&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231364057761&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Ssevraqf.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1667363813.1231364061&tn_fvq=1231364061&tn_uvq=1917563877&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str39 =
  'ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669321699093060&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str40 =
  'ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669321699093060&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let s91 = computeInvariants(str36, REGEXP_9);
let s92 = computeInvariants(str37, REGEXP_9);
let s93 = computeInvariants(str38, REGEXP_9);
function runBlock7(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_9; i++) {
    sum += '0'.replace(re40, '').length;
    sum += '0'.replace(re10, '').length;
    sum += '0'.replace(re51, '').length;
    sum += '0'.replace(re52, '').length;
    sum += '0'.replace(re53, '').length;
    sum += '0'.replace(re39, '').length;
    sum += '0'.replace(re54, '').length;
    sum += 'Lrf'.replace(re40, '').length;
    sum += 'Lrf'.replace(re10, '').length;
    sum += 'Lrf'.replace(re51, '').length;
    sum += 'Lrf'.replace(re52, '').length;
    sum += 'Lrf'.replace(re53, '').length;
    sum += 'Lrf'.replace(re39, '').length;
    sum += 'Lrf'.replace(re54, '').length;
  }
  for (let i = 0; i < REGEXP_8; i++) {
    sum += 'Pybfr {0}'.replace(re63, '').length;
    sum += 'Bcra {0}'.replace(re63, '').length;
    sum += s91[i].split(re32).length;
    sum += s92[i].split(re32).length;
    sum += 'puvyq p1 svefg gnournqref'.replace(re14, '').length;
    sum += 'puvyq p1 svefg gnournqref'.replace(re15, '').length;
    sum += 'uqy_fcb'.replace(re14, '').length;
    sum += 'uqy_fcb'.replace(re15, '').length;
    sum += 'uvag'.replace(re14, '').length;
    sum += 'uvag'.replace(re15, '').length;
    sum += s93[i].replace(re33, '').length;
    sum += 'yvfg'.replace(re14, '').length;
    sum += 'yvfg'.replace(re15, '').length;
    sum += 'at_bhgre'.replace(re30, '').length;
    sum += 'cnerag puebzr5 qbhoyr2 NU'.replace(re14, '').length;
    sum += 'cnerag puebzr5 qbhoyr2 NU'.replace(re15, '').length;
    sum += 'cnerag puebzr5 dhnq5 ps NU osyvax zbarl'.replace(re14, '').length;
    sum += 'cnerag puebzr5 dhnq5 ps NU osyvax zbarl'.replace(re15, '').length;
    sum += 'cnerag puebzr6 fvatyr1'.replace(re14, '').length;
    sum += 'cnerag puebzr6 fvatyr1'.replace(re15, '').length;
    sum += 'cb_qrs'.replace(re14, '').length;
    sum += 'cb_qrs'.replace(re15, '').length;
    sum += 'gnopbagrag'.replace(re14, '').length;
    sum += 'gnopbagrag'.replace(re15, '').length;
    sum += 'iv_svefg_gvzr'.replace(re30, '').length;
    sum += new RegExpBenchmark().execString(
      RegExp(
        '(^|.)(ronl|qri-ehf3.wbg)(|fgberf|zbgbef|yvirnhpgvbaf|jvxv|rkcerff|punggre).(pbz(|.nh|.pa|.ux|.zl|.ft|.oe|.zk)|pb(.hx|.xe|.am)|pn|qr|se|vg|ay|or|ng|pu|vr|va|rf|cy|cu|fr)$',
        'i'
      ),
      'cntrf.ronl.pbz'
    );
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231364074');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231364074.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.2294274870215848400.1231364074.1231364074.1231364074.1');
    sum += new RegExpBenchmark().execString(re8, '4413241q3660');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669357391353591');
    sum += new RegExpBenchmark().execString(re8, str39);
    sum += new RegExpBenchmark().execString(re8, str40);
    sum += new RegExpBenchmark().execString(re8, 'AFP_zp_kkk-gdzogv_80=4413241q3660');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7');
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.2294274870215848400.1231364074.1231364074.1231364074.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231364074');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231364074.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, 'p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7');
    sum += new RegExpBenchmark().execString(re34, s91[i]);
    sum += new RegExpBenchmark().execString(re34, s92[i]);
  }
  return sum;
}
let re64: RegExp = new RegExp('\\b[a-z]', 'g');
let re65: RegExp = new RegExp('^uggc:\\/\\/');
let re66: RegExp = new RegExp('(?:^|s+)qvfnoyrq(?:s+|$)');
let str41 = 'uggc://cebsvyr.zlfcnpr.pbz/Zbqhyrf/Nccyvpngvbaf/Cntrf/Pnainf.nfck';
function runBlock8(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_7; i++) {
    s21[i].match(RegExp('d+', 'g'));
    sum += 'nsgre'.replace(re64, '').length;
    sum += 'orsber'.replace(re64, '').length;
    sum += 'obggbz'.replace(re64, '').length;
    sum += 'ohvygva_jrngure.kzy'.replace(re65, '').length;
    sum += 'ohggba'.replace(re37, '').length;
    sum += 'ohggba'.replace(re18, '').length;

    sum += 'qngrgvzr.kzy'.replace(re65, '').length;
    sum += 'uggc://eff.paa.pbz/eff/paa_gbcfgbevrf.eff'.replace(re65, '').length;
    sum += 'vachg'.replace(re37, '').length;
    sum += 'vachg'.replace(re18, '').length;

    sum += 'vafvqr'.replace(re64, '').length;
    sum += 'cbvagre'.replace(re27, '').length;
    sum += 'cbfvgvba'.replace(RegExp('[A-Z]', 'g'), '').length;
    sum += 'gbc'.replace(re27, '').length;
    sum += 'gbc'.replace(re64, '').length;
    sum += 'hy'.replace(re37, '').length;
    sum += 'hy'.replace(re18, '').length;
    sum += str26.replace(re37, '').length;
    sum += str26.replace(re18, '').length;

    sum += 'lbhghor_vtbbtyr/i2/lbhghor.kzy'.replace(re65, '').length;
    sum += 'm-vaqrk'.replace(re27, '').length;
    sum += new RegExpBenchmark().execString(RegExp('#([w-]+)', 'g'), str26);

    sum += new RegExpBenchmark().execString(re16, 'urvtug');
    sum += new RegExpBenchmark().execString(re16, 'znetvaGbc');

    sum += new RegExpBenchmark().execString(re16, 'jvqgu');
    sum += new RegExpBenchmark().execString(re19, 'gno0 svefg ba');

    sum += new RegExpBenchmark().execString(re19, 'gno0 ba');
    sum += new RegExpBenchmark().execString(re19, 'gno4 ynfg');

    sum += new RegExpBenchmark().execString(re19, 'gno4');
    sum += new RegExpBenchmark().execString(re19, 'gno5');

    sum += new RegExpBenchmark().execString(re19, 'gno6');
    sum += new RegExpBenchmark().execString(re19, 'gno7');

    sum += new RegExpBenchmark().execString(re19, 'gno8');
    sum += new RegExpBenchmark().execString(RegExp('NqborNVE/([^s]*)'), s15[i]);

    sum += new RegExpBenchmark().execString(RegExp('NccyrJroXvg/([^ ]*)'), s15[i]);
    sum += new RegExpBenchmark().execString(RegExp('XUGZY', 'gi'), s15[i]);

    sum += new RegExpBenchmark().execString(RegExp('^(?:obql|ugzy)$', 'i'), 'YV');

    sum += new RegExpBenchmark().execString(re38, 'ohggba');
    sum += new RegExpBenchmark().execString(re38, 'vachg');
    sum += new RegExpBenchmark().execString(re38, 'hy');
    sum += new RegExpBenchmark().execString(re38, str26);
    sum += new RegExpBenchmark().execString(RegExp('^(\\w+|\\*)'), str26);
    sum += new RegExpBenchmark().execString(RegExp('znp|jva|yvahk', 'i'), 'Jva32');
    sum += new RegExpBenchmark().execString(RegExp('eton?([ds,]+)'), 'fgngvp');
  }
  for (let i = 0; i < REGEXP_6; i++) {
    sum += ''.replace(RegExp('\r', 'g'), '').length;
    sum += '/'.replace(re40, '').length;
    sum += '/'.replace(re10, '').length;
    sum += '/'.replace(re51, '').length;
    sum += '/'.replace(re52, '').length;
    sum += '/'.replace(re53, '').length;
    sum += '/'.replace(re39, '').length;
    sum += '/'.replace(re54, '').length;
    sum += 'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/{0}?[NDO]&{1}&{2}&[NDR]'.replace(re63, '').length;
    sum += str41.replace(re12, '').length;
    sum += 'uggc://jjj.snprobbx.pbz/fepu.cuc'.replace(re23, '').length;
    sum += 'freivpr'.replace(re40, '').length;
    sum += 'freivpr'.replace(re41, '').length;
    sum += 'freivpr'.replace(re42, '').length;
    sum += 'freivpr'.replace(re43, '').length;
    sum += 'freivpr'.replace(re44, '').length;
    sum += 'freivpr'.replace(re45, '').length;
    sum += 'freivpr'.replace(re46, '').length;
    sum += 'freivpr'.replace(re47, '').length;
    sum += 'freivpr'.replace(re48, '').length;
    sum += new RegExpBenchmark().execString(RegExp('((ZFVRs+([6-9]|dd).))'), s15[i]);
    sum += new RegExpBenchmark().execString(re66, '');
    sum += new RegExpBenchmark().execString(re50, 'fryrpgrq');
    sum += new RegExpBenchmark().execString(re8, '8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669340386893867');
    sum += new RegExpBenchmark().execString(re8, 'VC=74.125.75.17');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn');
    sum += new RegExpBenchmark().execString(RegExp('Xbadhrebe|Fnsnev|XUGZY'), s15[i]);
    sum += new RegExpBenchmark().execString(re13, str41);
    sum += new RegExpBenchmark().execString(re49, 'unfsbphf');
  }
  return sum;
}
let re67: RegExp = new RegExp('zrah_byq', 'g');
let str42 =
  'FrffvbaQQS2=473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669325184628362&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str43 =
  'FrffvbaQQS2=473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29; __hgzm=144631658.1231364380.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.3931862196947939300.1231364380.1231364380.1231364380.1; __hgzo=144631658.0.10.1231364380; __hgzp=144631658; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669325184628362&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str44 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_vzntrf_wf&qg=1231364373088&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231364373088&punaary=svz_zlfcnpr_hfre-ivrj-pbzzragf%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Spbzzrag.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1158737789.1231364375&tn_fvq=1231364375&tn_uvq=415520832&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str45 =
  'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669325184628362&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str46 =
  'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669325184628362&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let re68: RegExp = new RegExp('^([#.]?)((?:[w\u{0128}-\uffff*_-]|\\.)*)');
let re69: RegExp = new RegExp('\\{1\\}', 'g');
let re70: RegExp = new RegExp('s+', 'g');
let re71: RegExp = new RegExp('(\\$\\{4\\})|(\\$4\\b)', 'g');
let re72: RegExp = new RegExp('(\\${5\\})|(\\$5\\b)', 'g');
let re73: RegExp = new RegExp('\\{2\\}', 'g');
let re74: RegExp = new RegExp('[^+>] [^+>]');
let re75: RegExp = new RegExp('\bucpyvs*=s*([^;]*)', 'i');
let re76: RegExp = new RegExp('\bucuvqrs*=s*([^;]*)', 'i');
let re78: RegExp = new RegExp('\bhfucjrns*=s*([^;]*)', 'i');
let re79: RegExp = new RegExp('\bmvcs*=s*([^;]*)', 'i');
let re80: RegExp = new RegExp('^((?:[w\u{0128}-\uffff*_-]|\\.)+)(#)((?:[w\u{0128}-\uffff*_-]|\\.)+)');
let re81: RegExp = new RegExp('^([>+~])s*(w*)', 'i');
let re82: RegExp = new RegExp('^>s*((?:[w\u{0128}-\uffff*_-]|\\.)+)');
let re83: RegExp = new RegExp('^[s[]?shapgvba');

let re77 = RegExp('\bucfies*=s*([^;]*)', 'i');
let re84: RegExp = new RegExp('v/g.tvs#(.*)', 'i');
let str47 = '#Zbq-Vasb-Vasb-WninFpevcgUvag';
let str48 = ',n.svryqOgaPnapry';
let str49 =
  'FrffvbaQQS2=p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669357391353591&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_kkk-gdzogv_80=4413241q3660';
let str50 =
  'FrffvbaQQS2=p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7; AFP_zp_kkk-gdzogv_80=4413241q3660; AFP_zp_kkk-aowb_80=4413235p3660; __hgzm=144631658.1231367708.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.2770915348920628700.1231367708.1231367708.1231367708.1; __hgzo=144631658.0.10.1231367708; __hgzp=144631658; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669357391353591&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str51 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231367691141&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231367691141&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Sjjj.zlfcnpr.pbz%2S&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=320757904.1231367694&tn_fvq=1231367694&tn_uvq=1758792003&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str52 =
  'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55332979829981?[NDO]&aqu=1&g=7%2S0%2S2009%2014%3N38%3N42%203%20480&af=zfacbegny&cntrAnzr=HF%20UCZFSGJ&t=uggc%3N%2S%2Sjjj.zfa.pbz%2S&f=1024k768&p=24&x=L&oj=994&ou=634&uc=A&{2}&[NDR]';
let str53 = 'cnerag puebzr6 fvatyr1 gno fryrpgrq ovaq qbhoyr2 ps';
let str54 =
  'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669357391353591&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str55 =
  'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669357391353591&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str56 = 'ne;ng;nh;or;oe;pn;pu;py;pa;qr;qx;rf;sv;se;to;ux;vq;vr;va;vg;wc;xe;zk;zl;ay;ab;am;cu;cy;cg;eh;fr;ft;gu;ge;gj;mn;';
let str57 =
  'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886&GHVQ=1';
let str58 =
  'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886';
let str59 =
  'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886; mvc=m:94043|yn:37.4154|yb:-122.0585|p:HF|ue:1';
let str60 =
  'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886; mvc=m:94043|yn:37.4154|yb:-122.0585|p:HF';
let str61 = 'uggc://gx2.fgp.f-zfa.pbz/oe/uc/11/ra-hf/pff/v/g.tvs#uggc://gx2.fgo.f-zfa.pbz/v/29/4RQP4969777N048NPS4RRR3PO2S7S.wct';
let str62 = 'uggc://gx2.fgp.f-zfa.pbz/oe/uc/11/ra-hf/pff/v/g.tvs#uggc://gx2.fgo.f-zfa.pbz/v/OQ/63NP9O94NS5OQP1249Q9S1ROP7NS3.wct';
let str63 = 'zbmvyyn/5.0 (jvaqbjf; h; jvaqbjf ag 5.1; ra-hf) nccyrjroxvg/528.9 (xugzy, yvxr trpxb) puebzr/2.0.157.0 fnsnev/528.9';
let s94 = computeInvariants(str42, REGEXP_5);
let s95 = computeInvariants(str43, REGEXP_5);
let s96 = computeInvariants(str44, REGEXP_5);
let s97 = computeInvariants(str47, REGEXP_5);
let s98 = computeInvariants(str48, REGEXP_5);
let s99 = computeInvariants(str49, REGEXP_5);
let s100 = computeInvariants(str50, REGEXP_5);
let s101 = computeInvariants(str51, REGEXP_5);
let s102 = computeInvariants(str52, REGEXP_5);
let s103 = computeInvariants(str53, REGEXP_5);

function runBlock9(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_5; i++) {
    sum += s94[i].split(re32).length;
    sum += s95[i].split(re32).length;
    sum += 'svz_zlfcnpr_hfre-ivrj-pbzzragf,svz_zlfcnpr_havgrq-fgngrf'.split(re20).length;
    sum += s96[i].replace(re33, '').length;
    sum += 'zrah_arj zrah_arj_gbttyr zrah_gbttyr'.replace(re67, '').length;
    sum += 'zrah_byq zrah_byq_gbttyr zrah_gbttyr'.replace(re67, '').length;
    sum += new RegExpBenchmark().execString(re8, '102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98');
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231364380');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231364380.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.3931862196947939300.1231364380.1231364380.1231364380.1');
    sum += new RegExpBenchmark().execString(re8, '441326q33660');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669341278771470');
    sum += new RegExpBenchmark().execString(re8, str45);
    sum += new RegExpBenchmark().execString(re8, str46);
    sum += new RegExpBenchmark().execString(re8, 'AFP_zp_dfctwzssrwh-aowb_80=441326q33660');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98');
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.3931862196947939300.1231364380.1231364380.1231364380.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231364380');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231364380.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
  }
  for (let i = 0; i < REGEXP_4; i++) {
    sum += ' yvfg1'.replace(re14, '').length;
    sum += ' yvfg1'.replace(re15, '').length;
    sum += ' yvfg2'.replace(re14, '').length;
    sum += ' yvfg2'.replace(re15, '').length;
    sum += ' frneputebhc1'.replace(re14, '').length;
    sum += ' frneputebhc1'.replace(re15, '').length;
    sum += s97[i].replace(re68, '').length;
    sum += s97[i].replace(re18, '').length;
    sum += ''.replace(RegExp('&', 'g'), '').length;
    sum += ''.replace(re35, '').length;
    sum += '(..-{0})(|(d+)|)'.replace(re63, '').length;
    sum += s98[i].replace(re18, '').length;
    sum += '//vzt.jro.qr/vij/FC/${cngu}/${anzr}/${inyhr}?gf=${abj}'.replace(re56, '').length;
    sum += '//vzt.jro.qr/vij/FC/tzk_uc/${anzr}/${inyhr}?gf=${abj}'.replace(RegExp('(${anzr})|($anzr\b)', 'g'), '').length;
    sum += '<fcna pynff="urnq"><o>Jvaqbjf Yvir Ubgznvy</o></fcna><fcna pynff="zft">{1}</fcna>'.replace(re69, '').length;
    sum += '<fcna pynff="urnq"><o>{0}</o></fcna><fcna pynff="zft">{1}</fcna>'.replace(re63, '').length;
    sum += '<fcna pynff="fvtahc"><n uers=uggc://jjj.ubgznvy.pbz><o>{1}</o></n></fcna>'.replace(re69, '').length;
    sum += '<fcna pynff="fvtahc"><n uers={0}><o>{1}</o></n></fcna>'.replace(re63, '').length;
    sum += 'Vzntrf'.replace(re15, '').length;
    sum += 'ZFA'.replace(re15, '').length;
    sum += 'Zncf'.replace(re15, '').length;
    sum += 'Zbq-Vasb-Vasb-WninFpevcgUvag'.replace(re39, '').length;
    sum += 'Arjf'.replace(re15, '').length;
    sum += s99[i].split(re32).length;
    sum += s100[i].split(re32).length;
    sum += 'Ivqrb'.replace(re15, '').length;
    sum += 'Jro'.replace(re15, '').length;
    sum += 'n'.replace(re39, '').length;
    sum += 'nwnkFgneg'.split(re70).length;
    sum += 'nwnkFgbc'.split(re70).length;
    sum += 'ovaq'.replace(re14, '').length;
    sum += 'ovaq'.replace(re15, '').length;
    sum += 'oevatf lbh zber. Zber fcnpr (5TO), zber frphevgl, fgvyy serr.'.replace(re63, '').length;
    sum += 'puvyq p1 svefg qrpx'.replace(re14, '').length;
    sum += 'puvyq p1 svefg qrpx'.replace(re15, '').length;
    sum += 'puvyq p1 svefg qbhoyr2'.replace(re14, '').length;
    sum += 'puvyq p1 svefg qbhoyr2'.replace(re15, '').length;
    sum += 'puvyq p2 ynfg'.replace(re14, '').length;
    sum += 'puvyq p2 ynfg'.replace(re15, '').length;
    sum += 'puvyq p2'.replace(re14, '').length;
    sum += 'puvyq p2'.replace(re15, '').length;
    sum += 'puvyq p3'.replace(re14, '').length;
    sum += 'puvyq p3'.replace(re15, '').length;
    sum += 'puvyq p4 ynfg'.replace(re14, '').length;
    sum += 'puvyq p4 ynfg'.replace(re15, '').length;
    sum += 'pbclevtug'.replace(re14, '').length;
    sum += 'pbclevtug'.replace(re15, '').length;
    sum += 'qZFAZR_1'.replace(re14, '').length;
    sum += 'qZFAZR_1'.replace(re15, '').length;
    sum += 'qbhoyr2 ps'.replace(re14, '').length;
    sum += 'qbhoyr2 ps'.replace(re15, '').length;
    sum += 'qbhoyr2'.replace(re14, '').length;
    sum += 'qbhoyr2'.replace(re15, '').length;
    sum += 'uqy_arj'.replace(re14, '').length;
    sum += 'uqy_arj'.replace(re15, '').length;
    sum += 'uc_fubccvatobk'.replace(re30, '').length;
    sum += 'ugzy%2Rvq'.replace(re29, '').length;
    sum += 'ugzy%2Rvq'.replace(re30, '').length;
    sum += s101[i].replace(re33, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/cebgbglcr.wf${4}${5}'.replace(re71, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/cebgbglcr.wf${5}'.replace(re72, '').length;
    sum += s102[i].replace(re73, '').length;
    sum += 'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55332979829981?[NDO]&{1}&{2}&[NDR]'.replace(re69, '').length;
    sum += 'vztZFSG'.replace(re14, '').length;
    sum += 'vztZFSG'.replace(re15, '').length;
    sum += 'zfasbbg1 ps'.replace(re14, '').length;
    sum += 'zfasbbg1 ps'.replace(re15, '').length;
    sum += s103[i].replace(re14, '').length;
    sum += s103[i].replace(re15, '').length;
    sum += 'cnerag puebzr6 fvatyr1 gno fryrpgrq ovaq'.replace(re14, '').length;
    sum += 'cnerag puebzr6 fvatyr1 gno fryrpgrq ovaq'.replace(re15, '').length;
    sum += 'cevznel'.replace(re14, '').length;
    sum += 'cevznel'.replace(re15, '').length;
    sum += 'erpgnatyr'.replace(re30, '').length;
    sum += 'frpbaqnel'.replace(re14, '').length;
    sum += 'frpbaqnel'.replace(re15, '').length;
    sum += 'haybnq'.split(re70).length;
    sum += '{0}{1}1'.replace(re63, '').length;
    sum += '|{1}1'.replace(re69, '').length;
    sum += new RegExpBenchmark().execString(RegExp('(..-HF)(|(d+)|)', 'i'), 'xb-xe,ra-va,gu-gu');
    sum += new RegExpBenchmark().execString(re4, '/ZlFcnprNccf/NccPnainf,45000012');
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231367708');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231367708.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.2770915348920628700.1231367708.1231367708.1231367708.1');
    sum += new RegExpBenchmark().execString(re8, '4413235p3660');
    sum += new RegExpBenchmark().execString(re8, '441327q73660');
    sum += new RegExpBenchmark().execString(re8, '9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473');
    sum += new RegExpBenchmark().execString(re8, 'SbeprqRkcvengvba=633669350559478880');
    sum += new RegExpBenchmark().execString(re8, str54);
    sum += new RegExpBenchmark().execString(re8, str55);
    sum += new RegExpBenchmark().execString(re8, 'AFP_zp_dfctwzs-aowb_80=441327q73660');
    sum += new RegExpBenchmark().execString(re8, 'AFP_zp_kkk-aowb_80=4413235p3660');
    sum += new RegExpBenchmark().execString(re8, 'FrffvbaQQS2=9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473');
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.2770915348920628700.1231367708.1231367708.1231367708.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231367708');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231367708.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re34, s99[i]);
    sum += new RegExpBenchmark().execString(re34, s100[i]);
    sum += new RegExpBenchmark().execString(RegExp('ZFVRs+5[.]01'), s15[i]);
    sum += new RegExpBenchmark().execString(RegExp('HF(?=;)', 'i'), str56);
    sum += new RegExpBenchmark().execString(re74, s97[i]);
    sum += new RegExpBenchmark().execString(re28, 'svefg npgvir svefgNpgvir');
    sum += new RegExpBenchmark().execString(re28, 'ynfg');
    sum += new RegExpBenchmark().execString(RegExp('\bp:(..)', 'i'), 'm:94043|yn:37.4154|yb:-122.0585|p:HF');
    sum += new RegExpBenchmark().execString(re75, str57);
    sum += new RegExpBenchmark().execString(re75, str58);
    sum += new RegExpBenchmark().execString(re76, str57);
    sum += new RegExpBenchmark().execString(re76, str58);
    sum += new RegExpBenchmark().execString(re77, str57);
    sum += new RegExpBenchmark().execString(re77, str58);
    sum += new RegExpBenchmark().execString(RegExp('\bhfucces*=s*([^;]*)', 'i'), str59);
    sum += new RegExpBenchmark().execString(re78, str57);
    sum += new RegExpBenchmark().execString(re78, str58);
    sum += new RegExpBenchmark().execString(RegExp('\bjcis*=s*([^;]*)', 'i'), str59);
    sum += new RegExpBenchmark().execString(re79, str58);
    sum += new RegExpBenchmark().execString(re79, str60);
    sum += new RegExpBenchmark().execString(re79, str59);
    sum += new RegExpBenchmark().execString(RegExp('|p:([a-z]{2})', 'i'), 'm:94043|yn:37.4154|yb:-122.0585|p:HF|ue:1');
    sum += new RegExpBenchmark().execString(re80, s97[i]);
    sum += new RegExpBenchmark().execString(re61, 'cebgbglcr.wf');
    sum += new RegExpBenchmark().execString(re68, s97[i]);
    sum += new RegExpBenchmark().execString(re81, s97[i]);
    sum += new RegExpBenchmark().execString(re82, s97[i]);
    sum += new RegExpBenchmark().execString(RegExp('^Fubpxjnir Synfu (d)'), s21[i]);
    sum += new RegExpBenchmark().execString(RegExp('^Fubpxjnir Synfu (d+)'), s21[i]);
    sum += new RegExpBenchmark().execString(re83, '[bowrpg tybony]');
    sum += new RegExpBenchmark().execString(re62, s97[i]);
    sum += new RegExpBenchmark().execString(re84, str61);
    sum += new RegExpBenchmark().execString(re84, str62);
    sum += new RegExpBenchmark().execString(RegExp('jroxvg'), str63);
  }
  return sum;
}
let re85: RegExp = new RegExp('eaq_zbqobkva');
let str64 = '1231365729213';
let str65 = '74.125.75.3-1057165600.29978900';
let str66 = '74.125.75.3-1057165600.29978900.1231365730214';
let str67 = 'Frnepu%20Zvpebfbsg.pbz';
let str68 =
  'FrffvbaQQS2=8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn; ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669340386893867&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str69 =
  'FrffvbaQQS2=8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn; __hgzm=144631658.1231365779.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.1877536177953918500.1231365779.1231365779.1231365779.1; __hgzo=144631658.0.10.1231365779; __hgzp=144631658; ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669340386893867&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str70 = 'I=3%26THVQ=757q3ss871q44o7o805n8113n5p72q52';
let str71 = 'I=3&THVQ=757q3ss871q44o7o805n8113n5p72q52';
let str72 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231365765292&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231365765292&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Sohyyrgvaf.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1579793869.1231365768&tn_fvq=1231365768&tn_uvq=2056210897&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str73 = 'frnepu.zvpebfbsg.pbz';
let str74 = 'frnepu.zvpebfbsg.pbz/';
let str75 =
  'ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669340386893867&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str76 =
  'ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669340386893867&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
function runBlock10(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_3; i++) {
    sum += '%3Szxg=ra-HF'.replace(re39, '').length;
    sum += '-8'.replace(re40, '').length;
    sum += '-8'.replace(re10, '').length;
    sum += '-8'.replace(re51, '').length;
    sum += '-8'.replace(re52, '').length;
    sum += '-8'.replace(re53, '').length;
    sum += '-8'.replace(re39, '').length;
    sum += '-8'.replace(re54, '').length;
    sum += '1.5'.replace(re40, '').length;
    sum += '1.5'.replace(re10, '').length;
    sum += '1.5'.replace(re51, '').length;
    sum += '1.5'.replace(re52, '').length;
    sum += '1.5'.replace(re53, '').length;
    sum += '1.5'.replace(re39, '').length;
    sum += '1.5'.replace(re54, '').length;
    sum += '1024k768'.replace(re40, '').length;
    sum += '1024k768'.replace(re10, '').length;
    sum += '1024k768'.replace(re51, '').length;
    sum += '1024k768'.replace(re52, '').length;
    sum += '1024k768'.replace(re53, '').length;
    sum += '1024k768'.replace(re39, '').length;
    sum += '1024k768'.replace(re54, '').length;
    sum += str64.replace(re40, '').length;
    sum += str64.replace(re10, '').length;
    sum += str64.replace(re51, '').length;
    sum += str64.replace(re52, '').length;
    sum += str64.replace(re53, '').length;
    sum += str64.replace(re39, '').length;
    sum += str64.replace(re54, '').length;
    sum += '14'.replace(re40, '').length;
    sum += '14'.replace(re10, '').length;
    sum += '14'.replace(re51, '').length;
    sum += '14'.replace(re52, '').length;
    sum += '14'.replace(re53, '').length;
    sum += '14'.replace(re39, '').length;
    sum += '14'.replace(re54, '').length;
    sum += '24'.replace(re40, '').length;
    sum += '24'.replace(re10, '').length;
    sum += '24'.replace(re51, '').length;
    sum += '24'.replace(re52, '').length;
    sum += '24'.replace(re53, '').length;
    sum += '24'.replace(re39, '').length;
    sum += '24'.replace(re54, '').length;
    sum += str65.replace(re40, '').length;
    sum += str65.replace(re10, '').length;
    sum += str65.replace(re51, '').length;
    sum += str65.replace(re52, '').length;
    sum += str65.replace(re53, '').length;
    sum += str65.replace(re39, '').length;
    sum += str65.replace(re54, '').length;
    sum += str66.replace(re40, '').length;
    sum += str66.replace(re10, '').length;
    sum += str66.replace(re51, '').length;
    sum += str66.replace(re52, '').length;
    sum += str66.replace(re53, '').length;
    sum += str66.replace(re39, '').length;
    sum += str66.replace(re54, '').length;
    sum += '9.0'.replace(re40, '').length;
    sum += '9.0'.replace(re10, '').length;
    sum += '9.0'.replace(re51, '').length;
    sum += '9.0'.replace(re52, '').length;
    sum += '9.0'.replace(re53, '').length;
    sum += '9.0'.replace(re39, '').length;
    sum += '9.0'.replace(re54, '').length;
    sum += '994k634'.replace(re40, '').length;
    sum += '994k634'.replace(re10, '').length;
    sum += '994k634'.replace(re51, '').length;
    sum += '994k634'.replace(re52, '').length;
    sum += '994k634'.replace(re53, '').length;
    sum += '994k634'.replace(re39, '').length;
    sum += '994k634'.replace(re54, '').length;
    sum += '?zxg=ra-HF'.replace(re40, '').length;
    sum += '?zxg=ra-HF'.replace(re10, '').length;
    sum += '?zxg=ra-HF'.replace(re51, '').length;
    sum += '?zxg=ra-HF'.replace(re52, '').length;
    sum += '?zxg=ra-HF'.replace(re53, '').length;
    sum += '?zxg=ra-HF'.replace(re54, '').length;
    sum += 'PAA.pbz'.replace(re25, '').length;
    sum += 'PAA.pbz'.replace(re12, '').length;
    sum += 'PAA.pbz'.replace(re39, '').length;
    sum += 'Qngr & Gvzr'.replace(re25, '').length;
    sum += 'Qngr & Gvzr'.replace(re12, '').length;
    sum += 'Qngr & Gvzr'.replace(re39, '').length;
    sum += 'Frnepu Zvpebfbsg.pbz'.replace(re40, '').length;
    sum += 'Frnepu Zvpebfbsg.pbz'.replace(re54, '').length;
    sum += str67.replace(re10, '').length;
    sum += str67.replace(re51, '').length;
    sum += str67.replace(re52, '').length;
    sum += str67.replace(re53, '').length;
    sum += str67.replace(re39, '').length;
    sum += str68.split(re32).length;
    sum += str69.split(re32).length;
    sum += str70.replace(re52, '').length;
    sum += str70.replace(re53, '').length;
    sum += str70.replace(re39, '').length;
    sum += str71.replace(re40, '').length;
    sum += str71.replace(re10, '').length;
    sum += str71.replace(re51, '').length;
    sum += str71.replace(re54, '').length;
    sum += 'Jrngure'.replace(re25, '').length;
    sum += 'Jrngure'.replace(re12, '').length;
    sum += 'Jrngure'.replace(re39, '').length;
    sum += 'LbhGhor'.replace(re25, '').length;
    sum += 'LbhGhor'.replace(re12, '').length;
    sum += 'LbhGhor'.replace(re39, '').length;
    sum += str72.replace(re33, '').length;
    sum += 'erzbgr_vsenzr_1'.replace(RegExp('^erzbgr_vsenzr_'), '').length;
    sum += str73.replace(re40, '').length;
    sum += str73.replace(re10, '').length;
    sum += str73.replace(re51, '').length;
    sum += str73.replace(re52, '').length;
    sum += str73.replace(re53, '').length;
    sum += str73.replace(re39, '').length;
    sum += str73.replace(re54, '').length;
    sum += str74.replace(re40, '').length;
    sum += str74.replace(re10, '').length;
    sum += str74.replace(re51, '').length;
    sum += str74.replace(re52, '').length;
    sum += str74.replace(re53, '').length;
    sum += str74.replace(re39, '').length;
    sum += str74.replace(re54, '').length;
    sum += 'lhv-h'.replace(RegExp('-', 'g'), '').length;
    sum += new RegExpBenchmark().execString(re9, 'p');
    sum += new RegExpBenchmark().execString(re9, 'qz p');
    sum += new RegExpBenchmark().execString(re9, 'zbqynory');
    sum += new RegExpBenchmark().execString(re9, 'lhv-h svefg');
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231365779');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231365779.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.1877536177953918500.1231365779.1231365779.1231365779.1');
    sum += new RegExpBenchmark().execString(re8, str75);
    sum += new RegExpBenchmark().execString(re8, str76);
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.1877536177953918500.1231365779.1231365779.1231365779.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231365779');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231365779.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re34, str68);
    sum += new RegExpBenchmark().execString(re34, str69);
    sum += new RegExpBenchmark().execString(RegExp('^$'), '');
    sum += new RegExpBenchmark().execString(re31, 'qr');
    sum += new RegExpBenchmark().execString(RegExp('^znkd+$'), '');
    sum += new RegExpBenchmark().execString(RegExp('^zvad+$'), '');
    sum += new RegExpBenchmark().execString(RegExp('^erfgber$'), '');
    sum += new RegExpBenchmark().execString(re85, 'zbqobkva zbqobk_abcnqqvat ');
    sum += new RegExpBenchmark().execString(re85, 'zbqgvgyr');
    sum += new RegExpBenchmark().execString(re85, 'eaq_zbqobkva ');
    sum += new RegExpBenchmark().execString(re85, 'eaq_zbqgvgyr ');
    sum += new RegExpBenchmark().execString(RegExp('frpgvbad+_pbagragf'), 'obggbz_ani');
  }
  return sum;
}
let re86: RegExp = new RegExp(';s*');
let re87: RegExp = new RegExp('(${inyhr})|($inyhr\b)', 'g');
let re88: RegExp = new RegExp('(${abj})|($abj\b)', 'g');
let re89: RegExp = new RegExp('s+$');
let re90: RegExp = new RegExp('^s+');
let re91 = new RegExp(
  '(\\\\"|\\x00-|\\x1f|\\x7f-|\\x9f|\\u{00ad}|\\u{0600}-|\\u{0604}|\\u{070f}|\\u17b4|\\u17b5|\\u200c-|\\u200f|\\u2028-|\\u202f|\\u2060-|\\u206f|\\ufeff|\\ufff0-|\\uffff)',
  'g'
);
let re92: RegExp = new RegExp('^(:)([\\w-]+)\\("?([^\\(]*?)\\)?[^\\(]*?\\)?"');
let re93: RegExp = new RegExp('^([:.#]*)((?:[w\u{0128}-\uffff*_-]|\\.)+)');
let re94: RegExp = new RegExp('^\\[ *@?([\\w-]+) *([!*\\$^~=]*) *(\\\'?"?)(.*?)\\4 *\\]');
let str77 = '#fubhgobk .pybfr';
let str78 =
  'FrffvbaQQS2=102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669341278771470&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_dfctwzssrwh-aowb_80=441326q33660';
let str79 =
  'FrffvbaQQS2=102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98; AFP_zp_dfctwzssrwh-aowb_80=441326q33660; __hgzm=144631658.1231365869.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.1670816052019209000.1231365869.1231365869.1231365869.1; __hgzo=144631658.0.10.1231365869; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669341278771470&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str80 =
  'FrffvbaQQS2=9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669350559478880&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_dfctwzs-aowb_80=441327q73660';
let str81 =
  'FrffvbaQQS2=9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473; AFP_zp_dfctwzs-aowb_80=441327q73660; __hgzm=144631658.1231367054.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.1796080716621419500.1231367054.1231367054.1231367054.1; __hgzo=144631658.0.10.1231367054; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669350559478880&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str82 = '[glcr=fhozvg]';
let str83 = 'n.svryqOga,n.svryqOgaPnapry';
let str84 = 'n.svryqOgaPnapry';
let str85 = 'oyvpxchaxg';
let str86 = 'qvi.bow-nppbeqvba qg';
let str87 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_nccf_wf&qg=1231367052227&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231367052227&punaary=svz_zlfcnpr_nccf-pnainf%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Scebsvyr.zlfcnpr.pbz%2SZbqhyrf%2SNccyvpngvbaf%2SCntrf%2SPnainf.nfck&nq_glcr=grkg&rvq=6083027&rn=0&sez=1&tn_ivq=716357910.1231367056&tn_fvq=1231367056&tn_uvq=1387206491&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str88 =
  'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231365851658&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231365851658&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Scebsvyrrqvg.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1979828129.1231365855&tn_fvq=1231365855&tn_uvq=2085229649&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
let str89 =
  'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55023338617756?[NDO]&aqu=1&g=7%2S0%2S2009%2014%3N12%3N47%203%20480&af=zfacbegny&cntrAnzr=HF%20UCZFSGJ&t=uggc%3N%2S%2Sjjj.zfa.pbz%2S&f=0k0&p=43835816&x=A&oj=994&ou=634&uc=A&{2}&[NDR]';
let str90 = 'zrgn[anzr=nwnkHey]';
let str91 = 'anpuevpugra';
let str92 =
  "b oS={'oT':1.1};x $8n(B){z(B!=o9)};x $S(B){O(!$8n(B))z A;O(B.4L)z'T';b S=7t B;O(S=='2P'&&B.p4){23(B.7f){12 1:z'T';12 3:z/S/.2g(B.8M)?'ox':'oh'}}O(S=='2P'||S=='x'){23(B.nE){12 2V:z'1O';12 7I:z'5a';12 18:z'4B'}O(7t B.I=='4F'){O(B.3u)z'pG';O(B.8e)z'1p'}}z S};x $2p(){b 4E={};Z(b v=0;v<1p.I;v++){Z(b X 1o 1p[v]){b nc=1p[v][X];b 6E=4E[X];O(6E&&$S(nc)=='2P'&&$S(6E)=='2P')4E[X]=$2p(6E,nc);17 4E[X]=nc}}z 4E};b $E=7p.E=x(){b 1d=1p;O(!1d[1])1d=[p,1d[0]];Z(b X 1o 1d[1])1d[0][X]=1d[1][X];z 1d[0]};b $4D=7p.pJ=x(){Z(b v=0,y=1p.I;v<y;v++){1p[v].E=x(1J){Z(b 1I 1o 1J){O(!p.1Y[1I])p.1Y[1I]=1J[1I];O(!p[1I])p[1I]=$4D.6C(1I)}}}};$4D.6C=x(1I){z x(L){z p.1Y[1I].3H(L,2V.1Y.nV.1F(1p,1))}};$4D(7F,2V,6J,nb);b 3l=x(B){B=B||{};B.E=$E;z B};b pK=Y 3l(H);b pZ=Y 3l(C);C.6f=C.35('6f')[0];x $2O(B){z!!(B||B===0)};x $5S(B,n8){z $8n(B)?B:n8};x $7K(3c,1m){z 1q.na(1q.7K()*(1m-3c+1)+3c)};x $3N(){z Y 97().os()};x $4M(1U){pv(1U);pa(1U);z 1S};H.43=!!(C.5Z);O(H.nB)H.31=H[H.7q?'py':'nL']=1r;17 O(C.9N&&!C.om&&!oy.oZ)H.pF=H.4Z=H[H.43?'pt':'65']=1r;17 O(C.po!=1S)H.7J=1r;O(7t 5B=='o9'){b 5B=x(){};O(H.4Z)C.nd(\"pW\");5B.1Y=(H.4Z)?H[\"[[oN.1Y]]\"]:{}}5B.1Y.4L=1r;O(H.nL)5s{C.oX(\"pp\",A,1r)}4K(r){};b 18=x(1X){b 63=x(){z(1p[0]!==1S&&p.1w&&$S(p.1w)=='x')?p.1w.3H(p,1p):p};$E(63,p);63.1Y=1X;63.nE=18;z 63};18.1z=x(){};18.1Y={E:x(1X){b 7x=Y p(1S);Z(b X 1o 1X){b nC=7x[X];7x[X]=18.nY(nC,1X[X])}z Y 18(7x)},3d:x(){Z(b v=0,y=1p.I;v<y;v++)$E(p.1Y,1p[v])}};18.nY=x(2b,2n){O(2b&&2b!=2n){b S=$S(2n);O(S!=$S(2b))z 2n;23(S){12'x':b 7R=x(){p.1e=1p.8e.1e;z 2n.3H(p,1p)};7R.1e=2b;z 7R;12'2P':z $2p(2b,2n)}}z 2n};b 8o=Y 18({oQ:x(J){p.4w=p.4w||[];p.4w.1x(J);z p},7g:x(){O(p.4w&&p.4w.I)p.4w.9J().2x(10,p)},oP:x(){p.4w=[]}});b 2d=Y 18({1V:x(S,J){O(J!=18.1z){p.$19=p.$19||{};p.$19[S]=p.$19[S]||[];p.$19[S].5j(J)}z p},1v:x(S,1d,2x){O(p.$19&&p.$19[S]){p.$19[S].1b(x(J){J.3n({'L':p,'2x':2x,'1p':1d})()},p)}z p},3M:x(S,J){O(p.$19&&p.$19[S])p.$19[S].2U(J);z p}});b 4v=Y 18({2H:x(){p.P=$2p.3H(1S,[p.P].E(1p));O(!p.1V)z p;Z(b 3O 1o p.P){O($S(p.P[3O]=='x')&&3O.2g(/^5P[N-M]/))p.1V(3O,p.P[3O])}z p}});2V.E({7y:x(J,L){Z(b v=0,w=p.I;v<w;v++)J.1F(L,p[v],v,p)},3s:x(J,L){b 54=[];Z(b v=0,w=p.I;v<w;v++){O(J.1F(L,p[v],v,p))54.1x(p[v])}z 54},2X:x(J,L){b 54=[];Z(b v=0,w=p.I;v<w;v++)54[v]=J.1F(L,p[v],v,p);z 54},4i:x(J,L){Z(b v=0,w=p.I;v<w;v++){O(!J.1F(L,p[v],v,p))z A}z 1r},ob:x(J,L){Z(b v=0,w=p.I;v<w;v++){O(J.1F(L,p[v],v,p))z 1r}z A},3F:x(3u,15){b 3A=p.I;Z(b v=(15<0)?1q.1m(0,3A+15):15||0;v<3A;v++){O(p[v]===3u)z v}z-1},8z:x(1u,I){1u=1u||0;O(1u<0)1u=p.I+1u;I=I||(p.I-1u);b 89=[];Z(b v=0;v<I;v++)89[v]=p[1u++];z 89},2U:x(3u){b v=0;b 3A=p.I;6L(v<3A){O(p[v]===3u){p.6l(v,1);3A--}17{v++}}z p},1y:x(3u,15){z p.3F(3u,15)!=-1},oz:x(1C){b B={},I=1q.3c(p.I,1C.I);Z(b v=0;v<I;v++)B[1C[v]]=p[v];z B},E:x(1O){Z(b v=0,w=1O.I;v<w;v++)p.1x(1O[v]);z p},2p:x(1O){Z(b v=0,y=1O.I;v<y;v++)p.5j(1O[v]);z p},5j:x(3u){O(!p.1y(3u))p.1x(3u);z p},oc:x(){z p[$7K(0,p.I-1)]||A},7L:x(){z p[p.I-1]||A}});2V.1Y.1b=2V.1Y.7y;2V.1Y.2g=2V.1Y.1y;x $N(1O){z 2V.8z(1O)};x $1b(3J,J,L){O(3J&&7t 3J.I=='4F'&&$S(3J)!='2P')2V.7y(3J,J,L);17 Z(b 1j 1o 3J)J.1F(L||3J,3J[1j],1j)};6J.E({2g:x(6b,2F){z(($S(6b)=='2R')?Y 7I(6b,2F):6b).2g(p)},3p:x(){z 5K(p,10)},o4:x(){z 69(p)},7A:x(){z p.3y(/-D/t,x(2G){z 2G.7G(1).nW()})},9b:x(){z p.3y(/w[N-M]/t,x(2G){z(2G.7G(0)+'-'+2G.7G(1).5O())})},8V:x(){z p.3y(/\b[n-m]/t,x(2G){z 2G.nW()})},5L:x(){z p.3y(/^s+|s+$/t,'')},7j:x(){z p.3y(/s{2,}/t,' ').5L()},5V:x(1O){b 1i=p.2G(/d{1,3}/t);z(1i)?1i.5V(1O):A},5U:x(1O){b 3P=p.2G(/^#?(w{1,2})(w{1,2})(w{1,2})$/);z(3P)?3P.nV(1).5U(1O):A},1y:x(2R,f){z(f)?(f+p+f).3F(f+2R+f)>-1:p.3F(2R)>-1},nX:x(){z p.3y(/([.*+?^${}()|[]/\\])/t,'\\$1')}});2V.E({5V:x(1O){O(p.I<3)z A;O(p.I==4&&p[3]==0&&!1O)z'p5';b 3P=[];Z(b v=0;v<3;v++){b 52=(p[v]-0).4h(16);3P.1x((52.I==1)?'0'+52:52)}z 1O?3P:'#'+3P.2u('')},5U:x(1O){O(p.I!=3)z A;b 1i=[];Z(b v=0;v<3;v++){1i.1x(5K((p[v].I==1)?p[v]+p[v]:p[v],16))}z 1O?1i:'1i('+1i.2u(',')+')'}});7F.E({3n:x(P){b J=p;P=$2p({'L':J,'V':A,'1p':1S,'2x':A,'4s':A,'6W':A},P);O($2O(P.1p)&&$S(P.1p)!='1O')P.1p=[P.1p];z x(V){b 1d;O(P.V){V=V||H.V;1d=[(P.V===1r)?V:Y P.V(V)];O(P.1p)1d.E(P.1p)}17 1d=P.1p||1p;b 3C=x(){z J.3H($5S(P";
let str93 = 'hagreunyghat';
let str94 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669341278771470&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str95 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669350559478880&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
let str96 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669341278771470&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str97 =
  'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669350559478880&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
let str98 = 'shapgvba (){Cuk.Nccyvpngvba.Frghc.Pber();Cuk.Nccyvpngvba.Frghc.Nwnk();Cuk.Nccyvpngvba.Frghc.Synfu();Cuk.Nccyvpngvba.Frghc.Zbqhyrf()}';
function runBlock11(): number {
  let sum = 0;
  for (let i = 0; i < REGEXP_2; i++) {
    sum += ' .pybfr'.replace(re18, '').length;
    sum += ' n.svryqOgaPnapry'.replace(re18, '').length;
    sum += ' qg'.replace(re18, '').length;
    sum += str77.replace(re68, '').length;
    sum += str77.replace(re18, '').length;
    sum += ''.replace(re39, '').length;
    sum += ''.replace('/^/', '').length;
    sum += ''.split(re86).length;
    sum += '*'.replace(re39, '').length;
    sum += '*'.replace(re68, '').length;
    sum += '*'.replace(re18, '').length;
    sum += '.pybfr'.replace(re68, '').length;
    sum += '.pybfr'.replace(re18, '').length;
    sum += '//vzt.jro.qr/vij/FC/tzk_uc/fperra/${inyhr}?gf=${abj}'.replace(re87, '').length;
    sum += '//vzt.jro.qr/vij/FC/tzk_uc/fperra/1024?gf=${abj}'.replace(re88, '').length;
    sum += '//vzt.jro.qr/vij/FC/tzk_uc/jvafvmr/${inyhr}?gf=${abj}'.replace(re87, '').length;
    sum += '//vzt.jro.qr/vij/FC/tzk_uc/jvafvmr/992/608?gf=${abj}'.replace(re88, '').length;
    sum += '300k120'.replace(re30, '').length;
    sum += '300k250'.replace(re30, '').length;
    sum += '310k120'.replace(re30, '').length;
    sum += '310k170'.replace(re30, '').length;
    sum += '310k250'.replace(re30, '').length;
    sum += '9.0  e115'.replace(RegExp('^.*.(.*)s.*$'), '').length;
    sum += 'Nppbeqvba'.replace(re2, '').length;
    sum += 'Nxghryy\x0a'.replace(re89, '').length;
    sum += 'Nxghryy\x0a'.replace(re90, '').length;
    sum += 'Nccyvpngvba'.replace(re2, '').length;
    sum += 'Oyvpxchaxg\x0a'.replace(re89, '').length;
    sum += 'Oyvpxchaxg\x0a'.replace(re90, '').length;
    sum += 'Svanamra\x0a'.replace(re89, '').length;
    sum += 'Svanamra\x0a'.replace(re90, '').length;
    sum += 'Tnzrf\x0a'.replace(re89, '').length;
    sum += 'Tnzrf\x0a'.replace(re90, '').length;
    sum += 'Ubebfxbc\x0a'.replace(re89, '').length;
    sum += 'Ubebfxbc\x0a'.replace(re90, '').length;
    sum += 'Xvab\x0a'.replace(re89, '').length;
    sum += 'Xvab\x0a'.replace(re90, '').length;
    sum += 'Zbqhyrf'.replace(re2, '').length;
    sum += 'Zhfvx\x0a'.replace(re89, '').length;
    sum += 'Zhfvx\x0a'.replace(re90, '').length;
    sum += 'Anpuevpugra\x0a'.replace(re89, '').length;
    sum += 'Anpuevpugra\x0a'.replace(re90, '').length;
    sum += 'Cuk'.replace(re2, '').length;
    sum += 'ErdhrfgSvavfu'.split(re70).length;
    sum += 'ErdhrfgSvavfu.NWNK.Cuk'.split(re70).length;
    sum += 'Ebhgr\x0a'.replace(re89, '').length;
    sum += 'Ebhgr\x0a'.replace(re90, '').length;
    sum += str78.split(re32).length;
    sum += str79.split(re32).length;
    sum += str80.split(re32).length;
    sum += str81.split(re32).length;
    sum += 'Fcbeg\x0a'.replace(re89, '').length;
    sum += 'Fcbeg\x0a'.replace(re90, '').length;
    sum += 'GI-Fcbg\x0a'.replace(re89, '').length;
    sum += 'GI-Fcbg\x0a'.replace(re90, '').length;
    sum += 'Gbhe\x0a'.replace(re89, '').length;
    sum += 'Gbhe\x0a'.replace(re90, '').length;
    sum += 'Hagreunyghat\x0a'.replace(re89, '').length;
    sum += 'Hagreunyghat\x0a'.replace(re90, '').length;
    sum += 'Ivqrb\x0a'.replace(re89, '').length;
    sum += 'Ivqrb\x0a'.replace(re90, '').length;
    sum += 'Jrggre\x0a'.replace(re89, '').length;
    sum += 'Jrggre\x0a'.replace(re90, '').length;
    sum += str82.replace(re68, '').length;
    sum += str82.replace(re18, '').length;
    sum += str83.replace(re68, '').length;
    sum += str83.replace(re18, '').length;
    sum += str84.replace(re68, '').length;
    sum += str84.replace(re18, '').length;
    sum += 'nqiFreivprObk'.replace(re30, '').length;
    sum += 'nqiFubccvatObk'.replace(re30, '').length;
    sum += 'nwnk'.replace(re39, '').length;
    sum += 'nxghryy'.replace(re40, '').length;
    sum += 'nxghryy'.replace(re41, '').length;
    sum += 'nxghryy'.replace(re42, '').length;
    sum += 'nxghryy'.replace(re43, '').length;
    sum += 'nxghryy'.replace(re44, '').length;
    sum += 'nxghryy'.replace(re45, '').length;
    sum += 'nxghryy'.replace(re46, '').length;
    sum += 'nxghryy'.replace(re47, '').length;
    sum += 'nxghryy'.replace(re48, '').length;
    sum += str85.replace(re40, '').length;
    sum += str85.replace(re41, '').length;
    sum += str85.replace(re42, '').length;
    sum += str85.replace(re43, '').length;
    sum += str85.replace(re44, '').length;
    sum += str85.replace(re45, '').length;
    sum += str85.replace(re46, '').length;
    sum += str85.replace(re47, '').length;
    sum += str85.replace(re48, '').length;
    sum += 'pngrtbel'.replace(re29, '').length;
    sum += 'pngrtbel'.replace(re30, '').length;
    sum += 'pybfr'.replace(re39, '').length;
    sum += 'qvi'.replace(re39, '').length;
    sum += str86.replace(re68, '').length;
    sum += str86.replace(re18, '').length;
    sum += 'qg'.replace(re39, '').length;
    sum += 'qg'.replace(re68, '').length;
    sum += 'qg'.replace(re18, '').length;
    sum += 'rzorq'.replace(re39, '').length;
    sum += 'rzorq'.replace(re68, '').length;
    sum += 'rzorq'.replace(re18, '').length;
    sum += 'svryqOga'.replace(re39, '').length;
    sum += 'svryqOgaPnapry'.replace(re39, '').length;
    sum += 'svz_zlfcnpr_nccf-pnainf,svz_zlfcnpr_havgrq-fgngrf'.split(re20).length;
    sum += 'svanamra'.replace(re40, '').length;
    sum += 'svanamra'.replace(re41, '').length;
    sum += 'svanamra'.replace(re42, '').length;
    sum += 'svanamra'.replace(re43, '').length;
    sum += 'svanamra'.replace(re44, '').length;
    sum += 'svanamra'.replace(re45, '').length;
    sum += 'svanamra'.replace(re46, '').length;
    sum += 'svanamra'.replace(re47, '').length;
    sum += 'svanamra'.replace(re48, '').length;
    sum += 'sbphf'.split(re70).length;
    sum += 'sbphf.gno sbphfva.gno'.split(re70).length;
    sum += 'sbphfva'.split(re70).length;
    sum += 'sbez'.replace(re39, '').length;
    sum += 'sbez.nwnk'.replace(re68, '').length;
    sum += 'sbez.nwnk'.replace(re18, '').length;
    sum += 'tnzrf'.replace(re40, '').length;
    sum += 'tnzrf'.replace(re41, '').length;
    sum += 'tnzrf'.replace(re42, '').length;
    sum += 'tnzrf'.replace(re43, '').length;
    sum += 'tnzrf'.replace(re44, '').length;
    sum += 'tnzrf'.replace(re45, '').length;
    sum += 'tnzrf'.replace(re46, '').length;
    sum += 'tnzrf'.replace(re47, '').length;
    sum += 'tnzrf'.replace(re48, '').length;
    sum += 'ubzrcntr'.replace(re30, '').length;
    sum += 'ubebfxbc'.replace(re40, '').length;
    sum += 'ubebfxbc'.replace(re41, '').length;
    sum += 'ubebfxbc'.replace(re42, '').length;
    sum += 'ubebfxbc'.replace(re43, '').length;
    sum += 'ubebfxbc'.replace(re44, '').length;
    sum += 'ubebfxbc'.replace(re45, '').length;
    sum += 'ubebfxbc'.replace(re46, '').length;
    sum += 'ubebfxbc'.replace(re47, '').length;
    sum += 'ubebfxbc'.replace(re48, '').length;
    sum += 'uc_cebzbobk_ugzy%2Puc_cebzbobk_vzt'.replace(re30, '').length;
    sum += 'uc_erpgnatyr'.replace(re30, '').length;
    sum += str87.replace(re33, '').length;
    sum += str88.replace(re33, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf${4}${5}'.replace(re71, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf${5}'.replace(re72, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/qlaYvo.wf${4}${5}'.replace(re71, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/qlaYvo.wf${5}'.replace(re72, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/rssrpgYvo.wf${4}${5}'.replace(re71, '').length;
    sum += 'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/rssrpgYvo.wf${5}'.replace(re72, '').length;
    sum += str89.replace(re73, '').length;
    sum += 'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55023338617756?[NDO]&{1}&{2}&[NDR]'.replace(re69, '').length;
    sum += str6.replace(re23, '').length;
    sum += 'xvab'.replace(re40, '').length;
    sum += 'xvab'.replace(re41, '').length;
    sum += 'xvab'.replace(re42, '').length;
    sum += 'xvab'.replace(re43, '').length;
    sum += 'xvab'.replace(re44, '').length;
    sum += 'xvab'.replace(re45, '').length;
    sum += 'xvab'.replace(re46, '').length;
    sum += 'xvab'.replace(re47, '').length;
    sum += 'xvab'.replace(re48, '').length;
    sum += 'ybnq'.split(re70).length;
    sum += 'zrqvnzbqgno lhv-anifrg lhv-anifrg-gbc'.replace(re18, '').length;
    sum += 'zrgn'.replace(re39, '').length;
    sum += str90.replace(re68, '').length;
    sum += str90.replace(re18, '').length;
    sum += 'zbhfrzbir'.split(re70).length;
    sum += 'zbhfrzbir.gno'.split(re70).length;
    sum += str63.replace(RegExp('^.*jroxvg/(d+(.d+)?).*$'), '').length;
    sum += 'zhfvx'.replace(re40, '').length;
    sum += 'zhfvx'.replace(re41, '').length;
    sum += 'zhfvx'.replace(re42, '').length;
    sum += 'zhfvx'.replace(re43, '').length;
    sum += 'zhfvx'.replace(re44, '').length;
    sum += 'zhfvx'.replace(re45, '').length;
    sum += 'zhfvx'.replace(re46, '').length;
    sum += 'zhfvx'.replace(re47, '').length;
    sum += 'zhfvx'.replace(re48, '').length;
    sum += 'zlfcnpr_nccf_pnainf'.replace(re52, '').length;
    sum += str91.replace(re40, '').length;
    sum += str91.replace(re41, '').length;
    sum += str91.replace(re42, '').length;
    sum += str91.replace(re43, '').length;
    sum += str91.replace(re44, '').length;
    sum += str91.replace(re45, '').length;
    sum += str91.replace(re46, '').length;
    sum += str91.replace(re47, '').length;
    sum += str91.replace(re48, '').length;
    sum += 'anzr'.replace(re39, '').length;
    sum += str92.replace(RegExp('\bw+\b', 'g'), '').length;
    sum += 'bow-nppbeqvba'.replace(re39, '').length;
    sum += 'bowrpg'.replace(re39, '').length;
    sum += 'bowrpg'.replace(re68, '').length;
    sum += 'bowrpg'.replace(re18, '').length;
    sum += 'cnenzf%2Rfglyrf'.replace(re29, '').length;
    sum += 'cnenzf%2Rfglyrf'.replace(re30, '').length;
    sum += 'cbchc'.replace(re30, '').length;
    sum += 'ebhgr'.replace(re40, '').length;
    sum += 'ebhgr'.replace(re41, '').length;
    sum += 'ebhgr'.replace(re42, '').length;
    sum += 'ebhgr'.replace(re43, '').length;
    sum += 'ebhgr'.replace(re44, '').length;
    sum += 'ebhgr'.replace(re45, '').length;
    sum += 'ebhgr'.replace(re46, '').length;
    sum += 'ebhgr'.replace(re47, '').length;
    sum += 'ebhgr'.replace(re48, '').length;
    sum += 'freivprobk_uc'.replace(re30, '').length;
    sum += 'fubccvatobk_uc'.replace(re30, '').length;
    sum += 'fubhgobk'.replace(re39, '').length;
    sum += 'fcbeg'.replace(re40, '').length;
    sum += 'fcbeg'.replace(re41, '').length;
    sum += 'fcbeg'.replace(re42, '').length;
    sum += 'fcbeg'.replace(re43, '').length;
    sum += 'fcbeg'.replace(re44, '').length;
    sum += 'fcbeg'.replace(re45, '').length;
    sum += 'fcbeg'.replace(re46, '').length;
    sum += 'fcbeg'.replace(re47, '').length;
    sum += 'fcbeg'.replace(re48, '').length;
    sum += 'gbhe'.replace(re40, '').length;
    sum += 'gbhe'.replace(re41, '').length;
    sum += 'gbhe'.replace(re42, '').length;
    sum += 'gbhe'.replace(re43, '').length;
    sum += 'gbhe'.replace(re44, '').length;
    sum += 'gbhe'.replace(re45, '').length;
    sum += 'gbhe'.replace(re46, '').length;
    sum += 'gbhe'.replace(re47, '').length;
    sum += 'gbhe'.replace(re48, '').length;
    sum += 'gi-fcbg'.replace(re40, '').length;
    sum += 'gi-fcbg'.replace(re41, '').length;
    sum += 'gi-fcbg'.replace(re42, '').length;
    sum += 'gi-fcbg'.replace(re43, '').length;
    sum += 'gi-fcbg'.replace(re44, '').length;
    sum += 'gi-fcbg'.replace(re45, '').length;
    sum += 'gi-fcbg'.replace(re46, '').length;
    sum += 'gi-fcbg'.replace(re47, '').length;
    sum += 'gi-fcbg'.replace(re48, '').length;
    sum += 'glcr'.replace(re39, '').length;
    sum += 'haqrsletq'.replace(RegExp('/', 'g'), '').length;
    sum += str93.replace(re40, '').length;
    sum += str93.replace(re41, '').length;
    sum += str93.replace(re42, '').length;
    sum += str93.replace(re43, '').length;
    sum += str93.replace(re44, '').length;
    sum += str93.replace(re45, '').length;
    sum += str93.replace(re46, '').length;
    sum += str93.replace(re47, '').length;
    sum += str93.replace(re48, '').length;
    sum += 'ivqrb'.replace(re40, '').length;
    sum += 'ivqrb'.replace(re41, '').length;
    sum += 'ivqrb'.replace(re42, '').length;
    sum += 'ivqrb'.replace(re43, '').length;
    sum += 'ivqrb'.replace(re44, '').length;
    sum += 'ivqrb'.replace(re45, '').length;
    sum += 'ivqrb'.replace(re46, '').length;
    sum += 'ivqrb'.replace(re47, '').length;
    sum += 'ivqrb'.replace(re48, '').length;
    sum += 'ivfvgf=1'.split(re86).length;
    sum += 'jrggre'.replace(re40, '').length;
    sum += 'jrggre'.replace(re41, '').length;
    sum += 'jrggre'.replace(re42, '').length;
    sum += 'jrggre'.replace(re43, '').length;
    sum += 'jrggre'.replace(re44, '').length;
    sum += 'jrggre'.replace(re45, '').length;
    sum += 'jrggre'.replace(re46, '').length;
    sum += 'jrggre'.replace(re47, '').length;
    sum += 'jrggre'.replace(re48, '').length;
    sum += new RegExpBenchmark().execString(RegExp('#[a-z0-9]+$', 'i'), 'uggc://jjj.fpuhryreim.arg/Qrsnhyg');
    sum += new RegExpBenchmark().execString(re66, 'fryrpgrq');
    sum += new RegExpBenchmark().execString(RegExp('(?:^|s+)lhv-ani(?:s+|$)'), 'sff lhv-ani');
    sum += new RegExpBenchmark().execString(RegExp('(?:^|s+)lhv-anifrg(?:s+|$)'), 'zrqvnzbqgno lhv-anifrg');
    sum += new RegExpBenchmark().execString(RegExp('(?:^|s+)lhv-anifrg-gbc(?:s+|$)'), 'zrqvnzbqgno lhv-anifrg');
    sum += new RegExpBenchmark().execString(re91, 'GnoThvq');
    sum += new RegExpBenchmark().execString(re91, 'thvq');
    sum += new RegExpBenchmark().execString(RegExp('(pbzcngvoyr|jroxvg)'), str63);
    sum += new RegExpBenchmark().execString(RegExp('.+(?:ei|vg|en|vr)[/: ]([d.]+)'), str63);
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231365869');
    sum += new RegExpBenchmark().execString(re8, '144631658.0.10.1231367054');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231365869.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.1231367054.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '144631658.1670816052019209000.1231365869.1231365869.1231365869.1');
    sum += new RegExpBenchmark().execString(re8, '144631658.1796080716621419500.1231367054.1231367054.1231367054.1');
    sum += new RegExpBenchmark().execString(re8, str94);
    sum += new RegExpBenchmark().execString(re8, str95);
    sum += new RegExpBenchmark().execString(re8, str96);
    sum += new RegExpBenchmark().execString(re8, str97);
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.1670816052019209000.1231365869.1231365869.1231365869.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzn=144631658.1796080716621419500.1231367054.1231367054.1231367054.1');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231365869');
    sum += new RegExpBenchmark().execString(re8, '__hgzo=144631658.0.10.1231367054');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231365869.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re8, '__hgzm=144631658.1231367054.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    sum += new RegExpBenchmark().execString(re34, str78);
    sum += new RegExpBenchmark().execString(re34, str79);
    sum += new RegExpBenchmark().execString(re34, str81);
    sum += new RegExpBenchmark().execString(re74, str77);
    sum += new RegExpBenchmark().execString(re74, '*');
    sum += new RegExpBenchmark().execString(re74, str82);
    sum += new RegExpBenchmark().execString(re74, str83);
    sum += new RegExpBenchmark().execString(re74, str86);
    sum += new RegExpBenchmark().execString(re74, 'rzorq');
    sum += new RegExpBenchmark().execString(re74, 'sbez.nwnk');
    sum += new RegExpBenchmark().execString(re74, str90);
    sum += new RegExpBenchmark().execString(re74, 'bowrpg');
    sum += new RegExpBenchmark().execString(RegExp('\\/onfr.wf(\\?.+)?$'), '/uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf');
    sum += new RegExpBenchmark().execString(re28, 'uvag ynfgUvag ynfg');
    sum += new RegExpBenchmark().execString(re75, '');
    sum += new RegExpBenchmark().execString(re76, '');
    sum += new RegExpBenchmark().execString(re77, '');
    sum += new RegExpBenchmark().execString(re78, '');
    sum += new RegExpBenchmark().execString(re80, str77);
    sum += new RegExpBenchmark().execString(re80, '*');
    sum += new RegExpBenchmark().execString(re80, '.pybfr');
    sum += new RegExpBenchmark().execString(re80, str82);
    sum += new RegExpBenchmark().execString(re80, str83);
    sum += new RegExpBenchmark().execString(re80, str84);
    sum += new RegExpBenchmark().execString(re80, str86);
    sum += new RegExpBenchmark().execString(re80, 'qg');
    sum += new RegExpBenchmark().execString(re80, 'rzorq');
    sum += new RegExpBenchmark().execString(re80, 'sbez.nwnk');
    sum += new RegExpBenchmark().execString(re80, str90);
    sum += new RegExpBenchmark().execString(re80, 'bowrpg');
    sum += new RegExpBenchmark().execString(re61, 'qlaYvo.wf');
    sum += new RegExpBenchmark().execString(re61, 'rssrpgYvo.wf');
    sum += new RegExpBenchmark().execString(re61, 'uggc://jjj.tzk.arg/qr/?fgnghf=uvajrvf');
    sum += new RegExpBenchmark().execString(re92, ' .pybfr');
    sum += new RegExpBenchmark().execString(re92, ' n.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re92, ' qg');
    sum += new RegExpBenchmark().execString(re92, str48);
    sum += new RegExpBenchmark().execString(re92, '.nwnk');
    sum += new RegExpBenchmark().execString(re92, '.svryqOga,n.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re92, '.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re92, '.bow-nppbeqvba qg');
    sum += new RegExpBenchmark().execString(re68, str77);
    sum += new RegExpBenchmark().execString(re68, '*');
    sum += new RegExpBenchmark().execString(re68, '.pybfr');
    sum += new RegExpBenchmark().execString(re68, str82);
    sum += new RegExpBenchmark().execString(re68, str83);
    sum += new RegExpBenchmark().execString(re68, str84);
    sum += new RegExpBenchmark().execString(re68, str86);
    sum += new RegExpBenchmark().execString(re68, 'qg');
    sum += new RegExpBenchmark().execString(re68, 'rzorq');
    sum += new RegExpBenchmark().execString(re68, 'sbez.nwnk');
    sum += new RegExpBenchmark().execString(re68, str90);
    sum += new RegExpBenchmark().execString(re68, 'bowrpg');
    sum += new RegExpBenchmark().execString(re93, ' .pybfr');
    sum += new RegExpBenchmark().execString(re93, ' n.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re93, ' qg');
    sum += new RegExpBenchmark().execString(re93, str48);
    sum += new RegExpBenchmark().execString(re93, '.nwnk');
    sum += new RegExpBenchmark().execString(re93, '.svryqOga,n.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re93, '.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re93, '.bow-nppbeqvba qg');
    sum += new RegExpBenchmark().execString(re81, str77);
    sum += new RegExpBenchmark().execString(re81, '*');
    sum += new RegExpBenchmark().execString(re81, str48);
    sum += new RegExpBenchmark().execString(re81, '.pybfr');
    sum += new RegExpBenchmark().execString(re81, str82);
    sum += new RegExpBenchmark().execString(re81, str83);
    sum += new RegExpBenchmark().execString(re81, str84);
    sum += new RegExpBenchmark().execString(re81, str86);
    sum += new RegExpBenchmark().execString(re81, 'qg');
    sum += new RegExpBenchmark().execString(re81, 'rzorq');
    sum += new RegExpBenchmark().execString(re81, 'sbez.nwnk');
    sum += new RegExpBenchmark().execString(re81, str90);
    sum += new RegExpBenchmark().execString(re81, 'bowrpg');
    sum += new RegExpBenchmark().execString(re94, ' .pybfr');
    sum += new RegExpBenchmark().execString(re94, ' n.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re94, ' qg');
    sum += new RegExpBenchmark().execString(re94, str48);
    sum += new RegExpBenchmark().execString(re94, '.nwnk');
    sum += new RegExpBenchmark().execString(re94, '.svryqOga,n.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re94, '.svryqOgaPnapry');
    sum += new RegExpBenchmark().execString(re94, '.bow-nppbeqvba qg');
    sum += new RegExpBenchmark().execString(re94, '[anzr=nwnkHey]');
    sum += new RegExpBenchmark().execString(re94, str82);
    sum += new RegExpBenchmark().execString(re31, 'rf');
    sum += new RegExpBenchmark().execString(re31, 'wn');
    sum += new RegExpBenchmark().execString(re82, str77);
    sum += new RegExpBenchmark().execString(re82, '*');
    sum += new RegExpBenchmark().execString(re82, str48);
    sum += new RegExpBenchmark().execString(re82, '.pybfr');
    sum += new RegExpBenchmark().execString(re82, str82);
    sum += new RegExpBenchmark().execString(re82, str83);
    sum += new RegExpBenchmark().execString(re82, str84);
    sum += new RegExpBenchmark().execString(re82, str86);
    sum += new RegExpBenchmark().execString(re82, 'qg');
    sum += new RegExpBenchmark().execString(re82, 'rzorq');
    sum += new RegExpBenchmark().execString(re82, 'sbez.nwnk');
    sum += new RegExpBenchmark().execString(re82, str90);
    sum += new RegExpBenchmark().execString(re82, 'bowrpg');
    sum += new RegExpBenchmark().execString(re83, str98);
    sum += new RegExpBenchmark().execString(re83, 'shapgvba sbphf() { [angvir pbqr] }');
    sum += new RegExpBenchmark().execString(re62, '#Ybtva');
    sum += new RegExpBenchmark().execString(re62, '#Ybtva_cnffjbeq');
    sum += new RegExpBenchmark().execString(re62, str77);
    sum += new RegExpBenchmark().execString(re62, '#fubhgobkWf');
    sum += new RegExpBenchmark().execString(re62, '#fubhgobkWfReebe');
    sum += new RegExpBenchmark().execString(re62, '#fubhgobkWfFhpprff');
    sum += new RegExpBenchmark().execString(re62, '*');
    sum += new RegExpBenchmark().execString(re62, str82);
    sum += new RegExpBenchmark().execString(re62, str83);
    sum += new RegExpBenchmark().execString(re62, str86);
    sum += new RegExpBenchmark().execString(re62, 'rzorq');
    sum += new RegExpBenchmark().execString(re62, 'sbez.nwnk');
    sum += new RegExpBenchmark().execString(re62, str90);
    sum += new RegExpBenchmark().execString(re62, 'bowrpg');
    sum += new RegExpBenchmark().execString(re49, 'pbagrag');
    sum += new RegExpBenchmark().execString(re24, str6);
    sum += new RegExpBenchmark().execString(RegExp('xbadhrebe'), str63);
    sum += new RegExpBenchmark().execString(RegExp('znp'), 'jva32');
    sum += new RegExpBenchmark().execString(RegExp('zbmvyyn'), str63);
    sum += new RegExpBenchmark().execString(RegExp('zfvr'), str63);
    sum += new RegExpBenchmark().execString(RegExp('ags5.1'), str63);
    sum += new RegExpBenchmark().execString(RegExp('bcren'), str63);
    sum += new RegExpBenchmark().execString(RegExp('fnsnev'), str63);
    sum += new RegExpBenchmark().execString(RegExp('jva'), 'jva32');
    sum += new RegExpBenchmark().execString(RegExp('jvaqbjf'), str63);
  }
  return sum;
}


let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  let test2 = new RegExpBenchmark();
  test2.run();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

let test2 = new RegExpBenchmark();
test2.run();
