#!/usr/bin/python

sanger = True

if sanger:
    for i in xrange(128):
        if i >= 33:
            print '%-2d, /* %-3d -- %c */' % (i - 33 , i, i)
        else:
            print '0 , /* %-3d */' % i
else:
    for i in xrange(128):
        if i >= 64:
            print '%-2d, /* %-3d -- %c */' % (i - 64 , i, i)
        else:
            print '0 , /* %-3d */' % i
