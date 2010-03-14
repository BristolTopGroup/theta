# -*- coding: utf-8 -*-

import sys, os, sqlite3

def fail(s):
    print "FAIL: %s" % s
    sys.exit(1)

def warn(s):
    print "WARNING: %s" % s
    sys.exit(1)

def passed(s):
    print "PASS: %s" % s


def execute_checked(command):
    i=os.system(command)
    if i>0: fail("executing %s returned error" % command)

def exec_theta(s):
    execute_checked("../../bin/theta -q %s" % s)

def sql(filename, query):
    conn = sqlite3.connect(filename)
    c = conn.cursor()
    c.execute(query)
    result = c.fetchall()
    c.close()
    conn.close()
    return result
