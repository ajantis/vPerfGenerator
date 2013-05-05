'''
Created on 05.05.2013

@author: myaut
'''

import logging
logging.basicConfig()

from tsload.jsonts import TSClient
from tsload.admin import TSAdminAgentImpl
from twisted.internet import reactor

import socket

client = TSClient()
client.setAgentType('admin', TSAdminAgentImpl)

client.connect("localhost", 9090)
reactor.run()