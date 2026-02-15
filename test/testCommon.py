#!/usr/bin/python3

import os
import sys
import signal
import shlex

os.environ['LANG'] = 'C'

from dogtail.config import config
config.logDebugToStdOut = True
config.logDebugToFile = False

import dogtail.procedural as dt
from dogtail.procedural import focus
from dogtail.utils import run as dogtail_run

def run_app(file=None):
    global pid

    if file is not None:
        arguments = [os.path.join(os.path.dirname(__file__), file)]
    else:
        arguments = []

    cmd = " ".join([shlex.quote(sys.argv[1]), *[shlex.quote(arg) for arg in arguments]])
    pid = dogtail_run(cmd, appName='xreader')
    focus.application('xreader')

def bail():
    try:
        os.kill(pid, signal.SIGTERM)
    except Exception:
        pass
    sys.exit(1)
