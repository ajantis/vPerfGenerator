'''
Created on 05.05.2013

@author: myaut
'''

from jsonts import TSAgent, TSAgentMethod

class TSLoadAgent(TSAgent):
    getWorkloadTypes = TSAgentMethod('get_workload_types')