/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
// import { ignore, observed, state} from '../obx'

class ContactDetails {
  //welink_id
 public welink_id:string= '';
 public chineseName :string = '';
 public englishName :string = '';
 public deptCode :string = '';
 public deptName :string = '';
 public deptL1Code :string = '';
 public deptL1Name :string = '';
 public deptL2Code :string = '';
 public deptL2Name :string = '';
 public deptL3Code :string = '';
 public deptL3Name :string = '';
 public deptL4Code :string = '';
 public deptL5Code :string = '';
 public deptL6Code :string = '';
 public deptL7Code:string  = '';
 public deptL8Code :string = '';
 public deptLevel  :string= '';
  //主管工号;
 public currManagerNumber :string = '';
  //主管;
 public managerLastName:string  = '';
  //任职，比如 4级  技术族-消费者云技术类-消费者云SDET;
 public competence :string = '';
  //职位，比如主任工程师，18级;
 public postsRank :string = '';
 public employeeNumber :string = '';
 public w3account :string = '';
 public formerEmployeeNumber  :string= '';
  //邮箱;
 public personMail :string = '';
  //电话，多个以/拼接;
 public personMobileCode :string = '';
 public personAddress :string = '';
 public personAssistantAll :string = '';
 public personRoom :string = '';
 public personDifferenceTime :string = '';
 public personFaxCodeAll :string = '';
 public personInternalCodeAll :string = '';
  // 地址;
 public personLocation  :string= '';
 public personOther :string = '';
 public personPhoneCode  :string= '';
 public personTravelCode :string = '';
 public phoneCodeAll :string = '';
 public pinyinName :string = '';
 public personType  = '';
  //取值 F/M;
 public sex  :string= '';
 public module_code  :string= '';
 public module_name :string = '';
 public module_tlempnum :string = '';
 public module_tlname :string = '';
 public notesChmName  :string= '';
 public notesEngName :string = '';
 public language  :string= '';
 public localCountryCity :string = '';
  //当地时间;
 public timeByZone  :string= '';
 public lastUpdateDate :string = '';
 public uid :string = '';
 public upperDeptName :string = '';
  //邮政编码;
 public zipCode:string  = '';
  //签名;
 public singed :string = '';
  //备注;
 public remark :string = '';
  //关注;
 public followed :boolean = false;
  //-----------------自定义字段----------------;
  //常用联系人，与服务端返回字段对应;
 public movement :boolean = false;
  //好友;
 public friends :boolean= false;
  //普通联系人;
 public common :boolean = false;
  //1,标识为本人;
 public personalType :string = '';

  constructor(i) {
    this.welink_id = i.toString()
  }
}

function main() {
  let r= []
  // let s = new Date().getTime()
  var start = Date.now();
  for(let i = 0; i < 1000; i++) {
      r.push(new ContactDetails(i))
  }
  let sum = 0
  for (let j of r) {
      sum += Number(j.welink_id)
  }
  // let e = new Date().getTime()
  // this.msg = (e - s) + "  " + sum

  // console.log(e-s)

  var end = Date.now();
  // console.log(cnt)
  // console.log(end - start)
  print("welink_index:\t" + String(end - start) + "\tms");
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
    main()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

main();