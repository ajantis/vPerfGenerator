'''
Created on 05.05.2013

@author: myaut
'''

import json
import sys

import logging
import copy

import socket

from twisted.internet.protocol import Protocol, Factory
from twisted.internet.endpoints import TCP4ClientEndpoint
from twisted.internet import reactor, task

class TSAgentMethod:
    def __init__(self, name, args = []):
        self.name = name
        self.args = args
    
    def setAgentId(self, agentId):
        self.agentId = agentId
    
    def setFactory(self, factory):
        self.factory = factory
    
    def __call__(self, *args):
        msg = dict(zip(self.args, args))
        
        return self.factory.sendCommand(self.agentId, self.name, msg)

class TSAgentRoute(TSAgentMethod):
    def __init__(self, agentKlass, agentMethod):
        self.agentKlass  = agentKlass
        self.agentMethod = agentMethod
        
        TSAgentMethod.__init__(self, agentMethod.name, agentMethod.args)
    
    def __call__(self, agentId, *args):
        self.agentId = agentId 
        return TSAgentMethod.__call__(self, *args)

class TSAgent:
    listAgents = TSAgentMethod('list_agents')
    
    def __init__(self, factory, agentId):
        self.agentId = agentId
        self.factory = factory
        
        self.methods = {}
        
        for mname in dir(self):
            method = getattr(self, mname)
            
            if isinstance(method, TSAgentMethod):
                method = copy.copy(method)
                setattr(self, mname, method)
                
                method.setAgentId(agentId)
                method.setFactory(factory)
                
                self.methods[method.name] = method
    
    def invoke(self, agentId, command, msgBody):
        method = self.methods[command]
        
        method(**msgBody)

class JSONTS(Protocol):
    STATE_NEW          = 0
    STATE_CONNECTED    = 1
    STATE_ESTABLISHED  = 2
    STATE_DISCONNECTED = 3
    
    log = logging.getLogger('JSONTS')
    log.setLevel(logging.DEBUG)
    
    def __init__(self, factory):
        self.factory = factory
        self.state = JSONTS.STATE_NEW
        self.agentId = -1
        self.messageId = -1
        self.commandId = iter(xrange(sys.maxint))
        
        self.buffer = ''
    
    def connectionMade(self):
        self.state = JSONTS.STATE_CONNECTED
    
    def connectionLost(self, reason):
        if self.state == JSONTS.STATE_ESTABLISHED:
            # remove TSClient
            pass
        self.state = JSONTS.STATE_DISCONNECTED
    
    def dataReceived(self, data):
        # Split data into chunks
        self.log.debug('Received: %s', repr(data))
        
        data = self.buffer + data
        while data:
            chunk, sep, data = data.partition('\0')
            
            if not sep:
                # Received data was incomplete
                self.buffer = chunk
                break
            
            self.processMessage(chunk)
    
    def sendMessage(self, msg):
        if self.state not in [JSONTS.STATE_CONNECTED, JSONTS.STATE_ESTABLISHED]:
            return
        
        rawMsg = json.dumps(msg) + '\0'
        
        self.log.debug('Sending: %s', repr(rawMsg))
        
        self.transport.write(rawMsg)
    
    def sendCommand(self, agentId, command, msgBody):
        commandId = next(self.commandId)
        self.sendMessage({'agentId': agentId, 
                          'id': commandId, 
                          'cmd': command, 
                          'msg': msgBody})
        
        return commandId
        
    def sendResponse(self, agentId, msgId, response):
        self.sendMessage({'agentId': agentId, 
                          'id': msgId, 
                          'response': response})
        
    def sendError(self, agentId, msgId, error, code):
        self.sendMessage({'agentId': agentId, 
                          'id': msgId, 
                          'error': error, 
                          'code': code})
    
    def processMessage(self, rawMsg):
        msg = json.loads(rawMsg)
        
        self.log.debug('Received chunk: %s', repr(rawMsg))
        
        try:
            msgId = msg['id']
            agentId = msg['agentId']
            if 'cmd' in msg:
                command = msg['cmd']
                msgBody = msg['msg']
                self.factory.processCommand(self, agentId, msgId, command, msgBody)
            elif 'response' in msg:
                response = msg['response']
                self.factory.processResponse(self, agentId, msgId, response)
            elif 'error' in msg:
                error = msg['error']
                code = msg['code']
                self.factory.processError(self, agentId, msgId, error, code)
            else:
                # Protocol error
                pass
        except KeyError as ke:
            # protocol error
            raise
        
class TSServerFactory(Factory):
    '''TSServerFactory - Factory for twisted server that routes requests between JSONTS clients
    
    TSServer implements only two calls:
    1. hello() - allocates agentId and returns it to agent
    2. listAgents() - lists all agents
    
    Other commands/responses would be forwarded to desired client, which could be:
    * Listener 
        Not all clients may call for listAgents and determine to which agents they should 
        deliver their message. Instead of that, desired agent, called listener, registers 
        itself by calling addListener(), than all commands would be forwarded to that listener.
    * Route
        After command is sent, server creates a temporarily route to know, which was 
        original message id and agent id. When response arrives, server substitutes message id
        and agent id with original ones, from route, to do not confuse sender side by
        server generated ids.
    '''
    log = logging.getLogger('TSServerFactory')
    log.setLevel(logging.DEBUG)
    
    def __init__(self):
        self.agentTypes = {}
        self.clients = {}
        self.agents = {}
        
        self.routes = {}
        self.routeMsgs = {}
        
        self.agentid = iter(xrange(1, sys.maxint))
        
        self.deadCleaner = task.LoopingCall(self.cleanDeadAgents)
        self.deadCleaner.start(60, False)
    
    def getAgent(self, agentId):
        return self.clients[agentId]
    
    def registerAgentType(self, name, klass):
        self.agentTypes[name] = klass
    
    def registerAgent(self, client):
        agentId = next(self.agentid)
        agentKlass = self.agentTypes[client.agentType]
        agent = agentKlass(self, agentId)
        
        self.clients[agentId] = client
        self.agents[agentId] = agent
        
        client.agentId = agentId
        
        self.log.info('Agent hostName: %s registered', client.hostName)
        
        return agentId
    
    def cleanDeadAgents(self):
        deadAgentIds = []
        
        # FIXME: should process routes
        
        for agentId, client in self.clients.items():
            if client.state == JSONTS.STATE_DISCONNECTED:
                deadAgentIds.append(agentId)
                
        for agentId in deadAgentIds:
            del self.clients[agentId]
            del self.agents[agentId]
    
    def buildProtocol(self, addr):
        self.log.info('Client addr: %s connected', addr)
        
        return JSONTS(self)
    
    def sendCommand(self, agentId, command, msgBody):
        return self.clients[agentId].sendCommand(command, msgBody)
    
    def processCommand(self, client, agentId, msgId, command, msgBody):
        self.messageId = msgId
        
        if command == 'hello':
            hostInfo = msgBody['info']
            
            client.agentType = hostInfo['agentType']
            client.hostName = hostInfo['hostName']
            client.state = JSONTS.STATE_ESTABLISHED
            
            newAgentId = self.registerAgent(client)
            
            client.sendResponse(-1, msgId, {'agentId': newAgentId})
        elif command == 'list_agents':
            agents = {}
            for agentId, agent in self.agents.items():
                agents[agentId] = {'class': agent.__class__.__name__,
                                   'hostName': self.clients[agentId].hostName,
                                   'state': self.clients[agentId].state}
            client.sendResponse(0, msgId, agents)
        elif agentId != 0:
            srcAgentId = client.agentId
            
            self.routes[agentId] = srcAgentId
            self.routeMsgs[agentId] = msgId
            
            self.log.debug('Forwarding command %s from %d to %d', 
                           command, srcAgentId, agentId)
            
            agent = self.agents[agentId]
            agent.invoke(agentId, command, msgBody)
    
    def getRoute(self, agentId):
        dstAgentId = self.routes[agentId]
        dstMsgId = self.routeMsgs[agentId]
        
        del self.routes[agentId]
        del self.routeMsgs[agentId]
        
        return dstAgentId, dstMsgId
    
    def processResponse(self, client, agentId, msgId, response):
        dstAgentId, dstMsgId = self.getRoute(agentId)
        
        self.log.debug('Forwarding response #%d->%d from %d to %d', 
                           msgId, dstMsgId, agentId, dstAgentId)
        
        client = self.clients[dstAgentId]
        client.sendResponse(agentId, dstMsgId, response)
    
    def processError(self, client, agentId, msgId, error, code):
        dstAgentId, dstMsgId = self.getRoute(agentId)
        
        self.log.debug('Forwarding error #%d->%d from %d to %d', 
                           msgId, dstMsgId, agentId, dstAgentId)
        
        client = self.clients[agentId]
        client.sendError(agentId, dstMsgId, error, code)
    
class TSClientFactory(Factory):
    '''TSClientFactory - factory for twisted clients'''
    log = logging.getLogger('TSClientFactory')
    log.setLevel(logging.DEBUG)
    
    def buildProtocol(self, addr):
        self.log.info('Connected to %s', addr)
        
        return JSONTS(self)
    
    def __init__(self):
        self.calls = {}
    
    def call(self, msgId, callback, errback):
        ''' Register callbacks for message id msgId.
        When response or error arrives,  '''
        self.calls[msgId] = (callback, errback)
    
    def sendCommand(self, agentId, command, msgBody):
        return self.client.sendCommand(agentId, command, msgBody)
    
    def processCommand(self, client, agentId, msgId, command, msgBody):
        pass
    
    def processResponse(self, client, agentId, msgId, response):
        call = self.calls[msgId]
        call[0](response)
        del self.calls[msgId]
    
    def processError(self, client, agentId, msgId, error, code):
        call = self.calls[msgId]
        call[1](error, code)
        del self.calls[msgId]
    
class TSClient:
    def __init__(self):
        self.factory = TSClientFactory()
        self.hostName = socket.gethostname()
    
    def setAgentType(self, agentType, agentKlass):
        self.agentType = agentType
        self.agentKlass = agentKlass
    
    def _gotClient(self, client):
        self.factory.client = client
        info = {'agentType': self.agentType, 
                'hostName': self.hostName}
        
        self.factory.call(client.sendCommand(0, 'hello', {'info': info}),
                          self._gotAgent, self._gotAgentError)
    
    def _gotAgentError(self, error, code):
        print >> sys.stderr, 'ERROR %d: %s' % (code, error)
        reactor.stop()
    
    def _gotAgent(self, response):
        agentId = response['agentId']
        
        self.agent = self.agentKlass(self.factory, agentId)
        self.agent.gotAgent()        
    
    def connect(self, host, port):        
        self.endpoint = TCP4ClientEndpoint(reactor, "localhost", 9090)
        
        self.connection = self.endpoint.connect(self.factory)
        self.connection.addCallback(self._gotClient)