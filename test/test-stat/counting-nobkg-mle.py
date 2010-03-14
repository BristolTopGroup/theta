#!/usr/bin/python
# -*- coding: utf-8 -*-

execfile("../lib.py")

for Theta in (5.0, 10000.0):
    execute_checked("sed \"s/__THETA__/%s/g\" counting-nobkg.cfg.tpl > counting-nobkg.cfg" % Theta)
    exec_theta("counting-nobkg-mle.cfg")
    rows = sql("counting-nobkg-mle.db", "SELECT COUNT(*) FROM products WHERE abs(mle__Theta - writer__n_events_o) > %g" % (Theta/10000) )
    num_fail = rows[0][0]
    if num_fail > 0: fail("maximum likelihood estimate off too much in %d cases" % num_fail)
    else: passed("Theta = %f" % Theta)
    if Theta > 5000:
        rows = sql("counting-nobkg-mle.db", "SELECT COUNT(*) FROM products WHERE abs(mle__Theta_error * mle__Theta_error - mle__Theta) > 1.0")
        num_fail = rows[0][0]
        if num_fail > 0: fail("asymptotic error estimate in mle off too much in %d cases" % num_fail)
        else: passed("Theta = %f asymptotic error" % Theta)
