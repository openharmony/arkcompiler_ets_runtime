"""
Copyright (c) 2023 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: Use ark to execute workload test suite
"""

import argparse
import datetime
import logging
import os
import shutil
from collections import namedtuple
from openpyxl import Workbook, load_workbook
from openpyxl.styles import PatternFill


def get_logger(logger_name, log_file_path, level=logging.INFO):
    formatter = logging.Formatter(fmt='[%(asctime)s]  [%(levelname)s]   %(message)s',
                                  datefmt='%Y-%b-%d  %H:%M:%S')

    fh = logging.FileHandler(encoding='utf-8', mode='a', filename=log_file_path)
    fh.setFormatter(formatter)
    fh.setLevel(logging.DEBUG)
    # 进行显示屏输出
    ch = logging.StreamHandler()
    ch.setFormatter(formatter)
    ch.setLevel(logging.DEBUG)
    log = logging.getLogger(logger_name)
    log.addHandler(fh)
    log.addHandler(ch)
    log.setLevel(level)

    return log


logger = None
CUR_PATH = os.path.abspath(os.path.dirname(__file__))
TMP_PATH = os.path.join(os.getcwd(), "tmp")
RET_OK = 0
RET_FAILED = 1
BINARY_PATH = ""
OUTPUT_PATH = ""
LOG_PATH = ""
TODAY_EXCEL_PATH = ""
YESTERDAY_EXCEL_PATH = ""
DETERIORATION_BOUNDARY_VALUE = 0
PASS = 'pass'
FAIL = 'fail'


def update_excel_cell(report_file, cell_data):
    try:
        wb = load_workbook(report_file)
        ws = wb.worksheets[0]

    except FileNotFoundError:
        headers_row = ['用例名称', '执行状态', '用例执行耗时(ns)', '昨日用例执行耗时(ns)', '是否劣化', '执行日志', '备注']
        wb = Workbook()
        ws = wb.active

        ws.column_dimensions['A'].width = 35.0
        ws.column_dimensions['B'].width = 15.0
        ws.column_dimensions['C'].width = 15.0
        ws.column_dimensions['D'].width = 15.0
        ws.column_dimensions['E'].width = 25.0
        ws.column_dimensions['F'].width = 50.0
        ws.column_dimensions['G'].width = 10.0
        ws.append(headers_row)
        ws.freeze_panes = 'A2'
        wb.save(report_file)

    is_degraded = False
    is_existed_data = False
    target_row = 0
    for row_num in range(2, ws.max_row + 1):
        js_case_name_tmp = ws.cell(row=row_num, column=1).value

        if cell_data.js_file_name == js_case_name_tmp:
            is_existed_data = True
            target_row = row_num
            break

    if is_existed_data is False:
        new_row = [cell_data.js_file_name, cell_data.exec_status, cell_data.exec_time, ' ', 'False',
                   cell_data.stack_info, cell_data.notes]
        ws.append(new_row)
        wb.save(report_file)
    else:
        last_day_excute_time = str(ws.cell(row=target_row, column=4).value)
        exec_time = 0 if cell_data.exec_time == ' ' else int(cell_data.exec_time)
        if last_day_excute_time != ' ':
            if exec_time / int(last_day_excute_time) >= (1 + cell_data.deterioration_upper_limit):
                is_degraded = True

        ws.cell(row=target_row, column=2).value = str(cell_data.exec_status)
        ws.cell(row=target_row, column=3).value = str(cell_data.exec_time)
        ws.cell(row=target_row, column=5).value = str(is_degraded)
        if is_degraded:
            ws.cell(row=target_row, column=5).value = str(is_degraded)
            ws.cell(row=target_row, column=5).fill = PatternFill(start_color='FF0000', end_color='FF0000',
                                                                 fill_type='solid')
            ws.cell(row=target_row, column=6).value = str(cell_data.stack_info)
            ws.cell(row=target_row,
                    column=7).value = f"last day excute time(ns): {ws.cell(row=target_row, column=4).value}"
        wb.save(report_file)


def run_js_case(binary_path, js_file_path, class_name, api_name, excel_path):
    js_file_name = class_name + '/' + api_name + '.js'
    fangzhou_test_path = os.path.join(TMP_PATH, "fangzhou_test")  # for abc file
    if os.path.exists(fangzhou_test_path):
        shutil.rmtree(fangzhou_test_path)
    os.makedirs(fangzhou_test_path)

    class_folder_path = os.path.join(fangzhou_test_path, class_name)
    api_path = os.path.join(class_folder_path, api_name)
    if not os.path.exists(class_folder_path):
        os.makedirs(class_folder_path)
    abc_file_path = api_path + ".abc"
    cur_abc_file = os.path.join(CUR_PATH, api_name + ".abc")
    api_log_path = os.path.join(class_folder_path, api_name + ".log")
    Cell_data = namedtuple('cell', ['js_file_name', 'exec_status', 'exec_time', 'stack_info', 'notes',
                                    'deterioration_upper_limit'])
    es2abc_path = "{}/out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc".format(BINARY_PATH)
    # tranmit abc
    cmd = f"{es2abc_path} {js_file_path};cp {cur_abc_file} {abc_file_path} "
    logger.info("run cmd: %s", cmd)
    ret = os.system(cmd)
    if ret != 0:
        err_str = f"{js_file_name} generate abc file failed. cmd: {cmd} "
        logger.error(err_str)
        cell_data = Cell_data(js_file_name, FAIL, ' ', err_str, ' ', DETERIORATION_BOUNDARY_VALUE)
        update_excel_cell(excel_path, cell_data)
        return RET_FAILED
    # execute abc
    ark_js_vm_path = "{}/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm".format(binary_path)
    cmd = f"{ark_js_vm_path} --log-level=info --enable-runtime-stat=true {cur_abc_file} > {api_log_path}"
    logger.info("run cmd: %s", cmd)
    ret = os.system(cmd)
    # grep duration
    ns_str = ""
    is_api_log_path_valid = False
    if os.path.exists(api_log_path):
        is_api_log_path_valid = True
        with open(api_log_path, 'r') as f:
            for line in f:
                if "Runtime State duration" in line:
                    ns_str = line.split(':')[1].split("(")[0]
                    break
        with open(api_log_path, 'r') as f:
            content = f.read()

    if ret != 0:
        err_info = f"{js_file_name} execute abc file failed. cmd: {cmd}"
        logger.error(err_info)
        stack_info = content if is_api_log_path_valid else ' '
        cell_data = Cell_data(js_file_name, FAIL, ' ', stack_info, err_info, DETERIORATION_BOUNDARY_VALUE)
        update_excel_cell(excel_path, cell_data)
        if is_api_log_path_valid:
            os.remove(api_log_path)

        return RET_FAILED

    if len(ns_str) == 0:
        logger.error("grep executing time failed. log path: %s", api_log_path)
        notes = "grep executing time failed.log is saved in previous column, please check it!"

        cell_data = Cell_data(js_file_name, FAIL, ' ', content, notes, DETERIORATION_BOUNDARY_VALUE)
        update_excel_cell(excel_path, cell_data)

        return RET_FAILED
    else:
        cell_data = Cell_data(js_file_name, PASS, ns_str, content, ' ', DETERIORATION_BOUNDARY_VALUE)
        update_excel_cell(excel_path, cell_data)

    os.remove(cur_abc_file)

    return RET_OK


def run(jspath, excel_path):
    if not os.path.exists(jspath):
        logger.error("js perf cases path is not exist. jspath: %s", jspath)
    logger.info("begin to run js perf test. js perf cases path: %s", jspath)
    for root, _, files in os.walk(jspath):
        for file in files:
            if file == '.keep':
                continue
            file_path = os.path.join(root, file)
            results = file_path.split("/")
            class_name = results[-2]
            api_name = results[-1].split(".")[0]

            run_js_case(BINARY_PATH, file_path, class_name, api_name, excel_path)


def append_summary_info(report_file, total_cost_time):
    """
        summary info:
            pass count:
            fail count:
            totle count:
            degraded count:
            total excute time is(s) :
            degraded percentage upper limit:
    """
    try:
        wb = load_workbook(report_file)
        ws = wb.worksheets[0]

    except FileNotFoundError:
        headers_row = ['用例名称', '执行状态', '用例执行耗时(ns)', '昨日用例执行耗时(ns)', '是否劣化', '执行日志', '备注']
        wb = Workbook()
        ws = wb.active

        ws.column_dimensions['A'].width = 35.0
        ws.column_dimensions['B'].width = 15.0
        ws.column_dimensions['C'].width = 15.0
        ws.column_dimensions['D'].width = 15.0
        ws.column_dimensions['E'].width = 25.0
        ws.column_dimensions['F'].width = 50.0
        ws.column_dimensions['G'].width = 10.0
        ws.append(headers_row)
        ws.freeze_panes = 'A2'
        wb.save(report_file)

    # append 2 blank
    blank_num = 2
    ws.insert_rows(ws.max_row + 1, blank_num)

    totle_num = 0
    degraded_upper_limit = DETERIORATION_BOUNDARY_VALUE
    pass_num = 0
    failed_num = 0
    degraded_num = 0

    for row_num in range(2, ws.max_row + 1):
        excu_status = ws.cell(row=row_num, column=2).value
        is_degraded = ws.cell(row=row_num, column=5).value
        if excu_status == PASS:
            pass_num += 1
            totle_num += 1
        elif excu_status == 'FAIL':
            failed_num += 1
            totle_num += 1

        if is_degraded == 'True':
            degraded_num += 1

    count = 3
    for _ in range(count):
        new_row = [' ', ' ', ' ', ' ', ' ', ' ', ' ']
        ws.append(new_row)
    new_row = ['Degraded percentage upper limit', degraded_upper_limit, ' ', ' ', ' ', ' ', ' ']
    ws.append(new_row)
    new_row = ['Totle js case count', totle_num, ' ', ' ', ' ', ' ', ' ']
    ws.append(new_row)
    new_row = ['Pass count', pass_num, ' ', ' ', ' ', ' ', ' ']
    ws.append(new_row)
    new_row = ['Fail count', failed_num, ' ', ' ', ' ', ' ', ' ']
    ws.append(new_row)
    new_row = ['Degraded count', degraded_num, ' ', ' ', ' ', ' ', ' ']
    ws.append(new_row)
    new_row = ['Total excute time(时:分:秒.微妙)', total_cost_time, ' ', ' ', ' ', ' ', ' ']
    ws.append(new_row)

    ws.column_dimensions.group('D', hidden=True)
    wb.save(report_file)
    return RET_OK


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--binarypath",
        "-bp",
        required=True,
        help="path of binary folder. refer to harmony compile out folder path",
    )
    parser.add_argument(
        "--jspath",
        "-p",
        required=True,
        help="path of js scripts, support folder and file",
    )
    parser.add_argument(
        "--deterioration_boundary_value",
        "-d",
        default=0.05,
        help="deterioration boundary value, default 0.05",
    )
    parser.add_argument(
        "--output_folder_path",
        "-o",
        default=None,
        help="output folder for executing js cases, default current folder",
    )
    args = parser.parse_args()

    if not os.path.exists(args.binarypath):
        logger.error("parameter --binarypath is not exit. Please check it! binary path: %s", args.binarypath)
        raise RuntimeError("error bad  parameters  --binarypath")

    if args.output_folder_path is None:
        args.output_folder_path = os.getcwd()

    if not os.path.isabs(args.output_folder_path):
        args.output_folder_path = os.path.abspath(args.output_folder_path)

    return args


def init_report(report_file, yestoday_report):
    try:
        today_wb = load_workbook(report_file)
        today_ws = today_wb.worksheets[0]
    except FileNotFoundError:
        headers_row = ['用例名称', '执行状态', '用例执行耗时(ns)', '昨日用例执行耗时(ns)', '是否劣化', '执行日志', '备注']
        today_wb = Workbook()
        today_ws = today_wb.active

        today_ws.column_dimensions['A'].width = 35.0
        today_ws.column_dimensions['B'].width = 15.0
        today_ws.column_dimensions['C'].width = 15.0
        today_ws.column_dimensions['D'].width = 15.0
        today_ws.column_dimensions['E'].width = 25.0
        today_ws.column_dimensions['F'].width = 50.0
        today_ws.column_dimensions['G'].width = 10.0
        today_ws.append(headers_row)
        today_ws.freeze_panes = 'A2'
        today_wb.save(report_file)

    if not os.path.exists(yestoday_report):
        logger.warning("last day js perf test report path is not valid. so today's JS perf test\
                        cannot perform degradation comparison")
        return

    wb = load_workbook(yestoday_report)
    ws = wb.worksheets[0]
    yesterday_map = {}
    # open yesterday report and add cost time to D column
    for row_num in range(2, ws.max_row + 1):
        js_file_name = str(ws.cell(row=row_num, column=1).value)
        cost_time = ws.cell(row=row_num, column=3).value
        if cost_time is None:
            cost_time = " "
        if '.js' in js_file_name:
            yesterday_map[js_file_name] = cost_time

    wb.close()

    for key in yesterday_map:
        new_row = [key, ' ', ' ', yesterday_map[key], ' ', ' ', ' ']
        today_ws.append(new_row)

    today_wb.save(report_file)


def append_date_label(target_str, date_input):
    formatted_date = date_input.strftime('%Y%m%d')
    new_str = target_str + "_{}".format(formatted_date)

    return new_str


def get_given_date_report_name(date_input):
    report_name_head = "js_perf_test_result"
    report_name_head = append_date_label(report_name_head, date_input)
    return report_name_head + ".xlsx"


def get_given_date_report_path(date_input):
    report_file_name = get_given_date_report_name(date_input)
    report_file_path = os.path.join(OUTPUT_PATH, report_file_name)
    return report_file_path


if __name__ == "__main__":
    """
        command format: python3  run_js_test.py  -bp /home/out -p /home/arkjs-perf-test/js-perf-test -o output_path
        notes: all paths must be absolute path
    """
    LOG_PATH = os.path.join(TMP_PATH, "test.log")
    if os.path.exists(LOG_PATH):
        os.remove(LOG_PATH)
    logger = get_logger("jstest", LOG_PATH)

    paras = get_args()
    logger.info("execute arguments: %s", paras)

    DETERIORATION_BOUNDARY_VALUE = paras.deterioration_boundary_value
    BINARY_PATH = paras.binarypath
    OUTPUT_PATH = CUR_PATH
    if paras.output_folder_path is not None:
        OUTPUT_PATH = paras.output_folder_path

    if not os.path.exists(OUTPUT_PATH):
        os.makedirs(OUTPUT_PATH)

    today = datetime.date.today()
    yesterday = today - datetime.timedelta(days=1)
    TODAY_EXCEL_PATH = get_given_date_report_path(today)
    YESTERDAY_EXCEL_PATH = get_given_date_report_path(yesterday)

    if os.path.exists(TODAY_EXCEL_PATH):
        os.remove(TODAY_EXCEL_PATH)

    init_report(TODAY_EXCEL_PATH, YESTERDAY_EXCEL_PATH)
    start_time = datetime.datetime.now(tz=datetime.timezone.utc)
    run(paras.jspath, TODAY_EXCEL_PATH)
    end_time = datetime.datetime.now(tz=datetime.timezone.utc)

    totol_time = u"%s" % (end_time - start_time)
    append_summary_info(TODAY_EXCEL_PATH, totol_time)
    logger.info("run js perf test finished. Please check details in report.")
    shutil.rmtree(TMP_PATH)