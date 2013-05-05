'''
Created on 05.05.2013

@author: myaut
'''

import logging
logging.basicConfig()

from tsload.jsonts import TSServerFactory
from tsload.load import TSLoadAgent
from tsload.admin import TSAdminAgent

from twisted.internet import reactor

factory = TSServerFactory()
factory.registerAgentType('load', TSLoadAgent)
factory.registerAgentType('admin', TSAdminAgent)

reactor.listenTCP(9090, factory)
reactor.run()