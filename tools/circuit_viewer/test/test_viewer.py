#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2023 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

import os
import shutil
from http.server import SimpleHTTPRequestHandler
import socketserver
import threading
from selenium import webdriver
from selenium.webdriver.common.action_chains import ActionChains
import time
import easyocr
import nltk
import cv2
import tempfile
import pytest
import subprocess

web_path = os.path.realpath("../dist")


class MyHttpRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=web_path, **kwargs)


httpd = socketserver.TCPServer(("", 9999), MyHttpRequestHandler)
driver = webdriver.Chrome()
actions = ActionChains(driver)
reader = easyocr.Reader(['ch_sim', 'en'], verbose=False)
window_width = -1
window_height = -1
temp_image_file = os.path.join(tempfile.gettempdir(), "test.png")


def cut_image(image, x, y, w, h):
    x = int(x)
    y = int(y)
    return image[y:y + h, x:x + w]


oldx = 0
oldy = 0


def click_on_page(x, y):
    global oldx, oldy
    actions.move_by_offset(x - oldx, y - oldy).click().perform()
    oldx = x
    oldy = y
    time.sleep(1)


def setup():
    os.chdir("..")

    print("setup : 编译项目")
    subprocess.run(["npm", "run", "dist"])

    print("setup : 拷贝测试文件")
    shutil.copy("examples/log_loop.txt", "dist/test.txt")

    print("setup : 启动web服务")
    threading.Thread(target=httpd.serve_forever).start()

    print("setup : selenium打开测试页面")
    driver.implicitly_wait(10)
    driver.get("http://127.0.0.1:9999")
    global window_width, window_height
    window_width = driver.execute_script("return document.body.clientWidth")
    window_height = driver.execute_script("return document.body.clientHeight")


def teardown():
    print("teardown : 关闭selenium")
    driver.close()

    print("teardown : 关闭web服务")
    httpd.shutdown()


def test_loading():  # 验证载入中画面
    driver.get_screenshot_as_file(temp_image_file)
    image = cv2.imread(temp_image_file)
    result = reader.readtext(cut_image(image, window_width / 2 - 100, window_height / 2 - 20, 200, 40))
    assert result[0][1][:7] == "Loading"


def test_start():  # 验证主画面显示
    time.sleep(5)
    driver.get_screenshot_as_file(temp_image_file)
    image = cv2.imread(temp_image_file)
    result = reader.readtext(cut_image(image, 10, 100, 60, 20))
    assert result[0][1] == "隐藏选中"


def find_string_in_result(s, result):
    dis = 999
    p = -1
    for i, r in enumerate(result):
        d = nltk.edit_distance(r[1], s)
        if d < dis:
            dis = d
            p = i
    if dis < len(s) / 2:
        return result[p]
    return False


def test_selectfunc():  # 点击优化类型切换下拉菜单，选择优化类型
    click_on_page(420, 50)
    click_on_page(420, 150)

    driver.get_screenshot_as_file(temp_image_file)
    image = cv2.imread(temp_image_file)
    result = reader.readtext(image)
    ret = find_string_in_result("0,CIRCUIT_ROOT", result)
    assert ret != False


def test_hide():  # 点击state和root按钮，隐藏0,CIRCUIT_ROOT
    click_on_page(40, 80)
    click_on_page(350, 80)

    driver.get_screenshot_as_file(temp_image_file)
    image = cv2.imread(temp_image_file)
    result = reader.readtext(image)
    ret = find_string_in_result("0,CIRCUIT_ROOT", result)
    assert ret == False


def test_wait():
    time.sleep(10)
