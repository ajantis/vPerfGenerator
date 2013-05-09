#!/usr/bin/python 

import sys
import getopt

from tsload.cli.main import TSAdminCLIAgent

from twisted.internet import reactor

usageStr = '''tscli.py [-h host] [-p port] {-u user|-m master-key}'''

def usage(error):
    print >> sys.stderr, error
    print >> sys.stderr, usageStr
    sys.exit(2) 

HOST = 'localhost'
PORT = 9090

authUser = None
authMasterKey = None

try:
    opts, args = getopt.getopt(sys.argv[1:], 's:p:u:m:',
                               ['server=', 'port=', 'user=', 'master='])
except getopt.GetoptError as goe:
    usage(str(oge))

for opt, optarg in opts:
    if opt in ['-s', '--server']:
        HOST = optarg
    elif opt in ['-p', '--port']:
        PORT = int(optarg)
    elif opt in ['-u', '--user']:
        authUser = optarg
    elif opt in ['-m', '--master']:
        authMasterKey = optarg
    else:
        usage('Unknown option %s' % opt)
        
if authUser is None and authMasterKey is None:
    usage('Choose authentification method')

if authUser is not None and authMasterKey is not None:
    usage('-u and -m are mutually exclusive')

agent = TSAdminCLIAgent.createAgent('admin', HOST, PORT)
agent.setAuthType(authUser, authMasterKey)

reactor.run()