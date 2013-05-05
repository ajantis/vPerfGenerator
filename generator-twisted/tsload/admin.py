'''
Created on 05.05.2013

@author: myaut
'''

from tsload.jsonts import TSAgent, TSAgentRoute
from tsload.load import TSLoadAgent
from twisted.internet import reactor

import sys

class TSAdminAgent(TSAgent):
    getWorkloadTypes = TSAgentRoute(TSLoadAgent, TSLoadAgent.getWorkloadTypes)