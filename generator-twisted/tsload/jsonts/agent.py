'''
Created on 07.05.2013

@author: myaut
'''

import sys

from tsload.jsonts import JSONTS
from tsload.jsonts.interface import RootAgent

from twisted.internet.protocol import Factory
from twisted.internet.endpoints import TCP4ClientEndpoint
from twisted.internet import reactor

import uuid
import inspect

class CallContext:
    def __init__(self, client, msgId):
        self.client = client
        self.agentId = self.client.getId()
        self.msgId = msgId

class TSAgentClient(JSONTS):
    uuid = None
    
    def __init__(self, factory, agentId):
        self.agentId = agentId
        
        JSONTS.__init__(self, factory)
        
    def getId(self):
        return self.agentId

class TSAgent(Factory):
    uuid = None
    agentType = 'generic'
    
    @classmethod
    def createAgent(klass, agentType, host, port):
        agent = klass()
        
        if agent.uuid is None:
            agent.uuid = str(uuid.uuid1())
        
        agent.agentType = agentType
        agent.agents = {}
        agent.calls = {}
        
        agent.endpoint = TCP4ClientEndpoint(reactor, host, port)
        d = agent.endpoint.connect(agent)
        d.addCallback(agent.gotClient)
        d.addErrback(agent.gotConnectionError)
        
        return agent
    
    def doTrace(self, fmt, *args):
        print fmt % args 
    
    def buildProtocol(self, addr):
        return TSAgentClient(self, -1)
    
    def createRemoteAgent(self, agentId, agentKlass):
        agent = agentKlass(self.client, agentId)
        self.agents[agentId] = agent
        return agent
        
    def processMessage(self, srcClient, message):
        srcAgentId = srcClient.getId()
        srcMsgId = message['id']
        
        dstAgentId = message['agentId']
        
        self.doTrace('Process message %s(%d, %d)', srcClient, srcAgentId, srcMsgId)
        
        if 'cmd' in message:
            command = message['cmd']
            cmdArgs = message['msg']
            
            if dstAgentId != self.client.getId():
                raise JSONTS.Error(JSONTS.AE_INVALID_AGENT,
                                   'Invalid destination agent: ours is %d, received is %d' % (self.agentId, dstAgentId))
            
            if type(cmdArgs) != dict:
                raise JSONTS.Error(JSONTS.AE_MESSAGE_FORMAT, 'Message should be dictionary, not a %s' % type(cmdArgs))
            
            try:
                method = getattr(self, command)
            except AttributeError as ae:
                raise JSONTS.Error(JSONTS.AE_COMMAND_NOT_FOUND, str(ae))
            
            # Check if argument names matching
            args = inspect.getargspec(method)
            args = filter(lambda a: a not in ['self', 'context'], args.args)
            if set(args) != set(cmdArgs.keys()):
                raise JSONTS.Error(JSONTS.AE_MESSAGE_FORMAT,
                                   'Missing or extra arguments: needed %s, got %s' % (args, cmdArgs.keys()))
            
            context = CallContext(srcClient, srcMsgId)
            
            response = method(context = context, **cmdArgs)
            
            self.doTrace('Got response %s', response)
            
            srcClient.sendResponse(srcClient.getId(), srcMsgId, response)
        elif 'response' in message:
            response = message['response'] 
            self.processResponse(srcMsgId, response)
        elif 'error' in message:
            error = message['error']
            code = message['code']
            self.processError(srcMsgId, code, error)
    
    def call(self, call, callback, errback):
        self.calls[call.msgId] = call
        call.setCallbacks(callback, errback)
    
    def processResponse(self, msgId, response):        
        call = self.calls[msgId]
        call.callback(response)
        del self.calls[msgId]
        
    def processError(self, msgId, code, error):        
        call = self.calls[msgId]
        call.errback(code, error)
        del self.calls[msgId]
    
    def gotClient(self, client):
        # Connection was successful
        self.client = client
        self.rootAgent = self.createRemoteAgent(0, RootAgent)
        
        self.call(self.rootAgent.hello(agentType = self.agentType, agentUuid = self.uuid), 
                  self.gotHello, self.gotError)
    
    def gotHello(self, response):        
        self.agentId = response['agentId']
        
        self.doTrace('Got hello response %s', response)
        
        self.gotAgent()
    
    def gotConnectionError(self, failure):
        self.gotError(JSONTS.AE_CONNECTION_ERROR, str(failure))
    
    def gotError(self, code, error):
        print >> sys.stderr, 'ERROR %d: %s' % (code, error)
        
    def disconnect(self):
        self.endpoint.disconnect()    
        