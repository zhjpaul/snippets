#!/usr/bin/env python

import re
import urllib

def getHtml(url):
    page = urllib.urlopen(url)
    html = page.read()
    #print html
    return html

def getImg(html):
    #reg = r'http://girlatlas.b0.upaiyun.com'
    reg = r'http://girlatlas.b0.upaiyun.com/\d+/\d+/\w+.jpg!mid'
    imgre = re.compile(reg)
    imglist = imgre.findall(html)
    global x
    for imgurl in imglist:
        print imgurl
        urllib.urlretrieve(imgurl, './pic/%s.jpg' %x)
        x = x + 1

def getPage(url):
    ## get all the page first
    html = getHtml(url)
    reg = r'http://girl-atlas.com/a/\d+'
    page_re = re.compile(reg)
    page_list = page_re.findall(html)
    
    pre = ""
    for page in page_list:
        if page != pre:
            print page
            getImg(getHtml(page))
        pre = page

    ## get the next page url
    reg = r'/index.action?id=\d+&last=\d+'
    next_re = re.compile(reg)
    next_list = next_re.findall(html)
    for nextp in next_list:
        print nextp
        getPage(base + nextp)

if __name__ == "__main__":
    x = 0
    base = r"http://girl-atlas.com"
    getPage(base)
