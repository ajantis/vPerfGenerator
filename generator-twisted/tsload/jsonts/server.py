'''
Created on 07.05.2013

@author: myaut
'''

import sys
import traceback

from tsload.jsonts import AccessRule, Flow, JSONTS
from tsload.jsonts.agent import TSAgent, TSAgentClient, CallContext

from twisted.internet.protocol import Factory
from twisted.internet import reactor, task

import uuid

rootAgentUUID = '{14f498da-a689-4341-8869-e4a292b143b6}'
rootAgentType = 'root'

class TSServerClient(JSONTS):
    uuid = None
    
    def __init__(self, factory, agentId):
        self.agentId = agentId
        
        self.flows = {}
        self.acl = []
        
        self.master = False
        
        JSONTS.__init__(self, factory)
        
    def addAccessRule(self, ar):
        self.acl.append(ar)
    
    def getId(self):
        return self.agentId
    
    def setId(self, agentId):
        self.agentId = agentId
    
    def setAgentInfo(self, agentType, agentUuid):
        self.agentType = agentType
        self.agentUuid = agentUuid
        
    def serializeAgentInfo(self):
        return {'uuid': self.agentUuid, 
                'class': self.agentType,
                'state': self.state}
    
    def checkACL(self, flow):
        if self.master:
            return True
        
        for ar in self.acl:
            if ar == flow:
                return True
        
        return False
    
    def addFlow(self, flow):
        self.factory.doTrace('Added flow %s [%d -> %d] to client %d', flow, flow.srcMsgId, flow.dstMsgId, self.agentId)
        
        self.flows[flow.dstAgentId, flow.dstMsgId] = flow
    
    def findFlow(self, agentId, msgId):
        try:
            flow = self.flows[agentId, msgId]
            del self.flows[agentId, msgId]
            return flow
        except KeyError:
            return None

class TSRootProxy(JSONTS):
    def __init__(self, server, client):
        self.server = server
        self.client = client
    
    def sendMessage(self, msg):
        self.server.processMessage(self.server.rootAgent.client, msg)
        
    def __getattr__(self, name):
        return getattr(self.client, name)

class TSRootAgent(TSAgent):
    uuid = rootAgentUUID
    agentType = rootAgentType
    
    def __init__(self, server):
        self.server = server
        self.client = TSServerClient(server, 0)
        
        self.client.setAgentInfo(self.agentType, self.uuid)
    
    def hello(self, context, agentType, agentUuid):
        agentId = context.client.getId()
        client = self.server.clients[agentId]
        
        context.client.setAgentInfo(agentType, agentUuid)
        
        return {'agentId': context.client.getId()}
    
    def authMasterKey(self, context, masterKey):
        client = context.client
        
        self.server.doTrace('Authentificating client %s with key %s', client, masterKey)
        
        if str(self.server.masterKey) == masterKey:
            client.master = True
            return {}
        
        raise JSONTS.Error(JSONTS.AE_INVALID_DATA, 'Master key invalid')
    
    def listClients(self, context):
        clients = {}
        
        for agentId in self.server.clients:
            client = self.server.clients[agentId]
            clients[agentId] = client.serializeAgentInfo()
            
        return clients

class TSServer(Factory):
    def __init__(self):        
        self.rootAgent = TSRootAgent(self)
        self.clients = {0: self.rootAgent.client}
        
        self.listenerFlows = []
        for command in ['hello', 'authMasterKey', 'listClients']:
            self.listenerFlows.append(Flow(dstAgentId = 0, command = command))
        
        self.msgCounter = iter(xrange(1, sys.maxint))
        self.agentIdGenerator = iter(xrange(1, sys.maxint))
        
        self.deadCleaner = task.LoopingCall(self.cleanDeadAgents)
        self.deadCleaner.start(60, False)
        
        self.generateMasterKey()
        
    def generateMasterKey(self):
        self.masterKey = uuid.uuid1()
        self.doTrace('Master key is %s', self.masterKey)
        
        # FIXME: correct directory
        masterFile = file('master.key', 'w')
        masterFile.write(str(self.masterKey))
        masterFile.close()
    
    def doTrace(self, fmt, *args):
        print fmt % args 
    
    @classmethod
    def createServer(klass, port):
        server = klass()
        
        reactor.listenTCP(port, server)
    
    def buildProtocol(self, addr):
        agentId = next(self.agentIdGenerator)
        client = TSServerClient(self, agentId)
        
        self.clients[agentId] = client
        
        return client
    
    def cleanDeadAgents(self):
        deadAgentIds = []
        
        # FIXME: should process routes
        
        for agentId, client in self.clients.items():
            if client.state == JSONTS.STATE_DISCONNECTED:
                deadAgentIds.append(agentId)
                
        for agentId in deadAgentIds:
            self.doTrace('Cleaned agent %d', agentId)
            del self.clients[agentId]
    
    def processMessage(self, srcClient, message):
        srcAgentId = srcClient.getId()
        srcMsgId = message['id']
        
        dstAgentId = message['agentId']
        dstMsgId = next(self.msgCounter)
        
        self.doTrace('Process message %s from %d', message, srcAgentId)
        
        try:
            dstClient = self.clients[dstAgentId]
        except KeyError:
            if 'cmd' in message:
                raise JSONTS.Error(JSONTS.AE_INVALID_AGENT, 
                                   'Invalid agent #%d' % dstAgentId)
            return
        
        self.doTrace('Process message %s(%d, %d) -> %s(%d, %d)',
                     srcClient, srcAgentId, srcMsgId, 
                     dstClient, dstAgentId, dstMsgId)
        
        if 'cmd' in message:
            command = message['cmd']
            
            flow = Flow(srcAgentId = srcAgentId,
                        dstAgentId = dstAgentId, 
                        command = command)
            
            if dstAgentId == srcAgentId or dstAgentId == 0:
                for listener in self.listenerFlows:
                    self.doTrace('Checking flow %s against listener rule %s', flow, listener)
                    
                    if listener == flow:
                        dstMsgId = next(self.msgCounter)
                        flowClone = flow.clone(srcMsgId, dstMsgId)
                        dstClient.addFlow(flowClone)
                        
                        message['id'] = dstMsgId
                        message['agentId'] = listener.dstAgentId
                        self.deliverMessage(srcClient, message)
            else:
                flow.setMsgIds(srcMsgId, dstMsgId)
                
                if not srcClient.checkACL(flow):
                    raise JSONTS.Error(JSONTS.AE_ACCESS_DENIED, 'Access is denied')
                
                dstClient.addFlow(flow)
                
                message['id'] = dstMsgId
                self.deliverMessage(srcClient, message)
        else:
            flow = srcClient.findFlow(srcAgentId, srcMsgId)
            
            if flow is None:
                return
            
            self.doTrace('Found flow %s [%d -> %d]', flow, flow.srcMsgId, flow.dstMsgId)
            
            message['id'] = flow.srcMsgId
            message['agentId'] = flow.srcAgentId
            
            self.deliverMessage(srcClient, message)
      
    def deliverMessage(self, srcClient, message):  
        dstAgentId = message['agentId']
        
        self.doTrace('Deliver message -> %d', dstAgentId)
        
        if dstAgentId == 0:
            self.rootAgent.processMessage(TSRootProxy(self, srcClient), message)
        else:
            dstClient = self.clients[dstAgentId]
            dstClient.sendMessage(message)
            
            