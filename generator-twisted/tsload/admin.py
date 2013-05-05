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
    
class TSAdminAgentImpl(TSAdminAgent):
    def gotAgent(self):
        self.factory.call(self.listAgents(),
                          self.listAgentsResponse, self.gotError)
     
    def listWorkloadTypes(self, response):
        print response 
        
    def listAgentsResponse(self, response):
        for agentId, agentDescription in response.items():
            if agentDescription['class'] == 'TSLoadAgent':
                self.factory.call(self.getWorkloadTypes(int(agentId)),
                                  self.listWorkloadTypes, self.gotError)
        
    def gotError(self, error, code):
        print >> sys.stderr, 'ERROR %d: %s' % (error, code)
        reactor.stop()