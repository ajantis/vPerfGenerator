'''
Created on 05.05.2013

@author: myaut
'''

import sys
import shlex

import logging
logging.basicConfig()

from tsload.jsonts import TSClient
from tsload.admin import TSAdminAgent
from twisted.internet import reactor

import socket

from tscli import CLIContext, NextContext
from tscli.agent import AgentRootContext

class RootContext(CLIContext):
    name = ''
    operations = ['agent']
    
    @NextContext(AgentRootContext)
    def agent(self, args):
        '''Agent administration'''
        return args


class AdminCLI(TSAdminAgent):
    def __init__(self, factory, agentId):
        self.context = RootContext(None, self)
        TSAdminAgent.__init__(self, factory, agentId)
    
    def gotAgent(self):
        self.ask()
    
    def _read_args(self):
        prompt = 'TS %s> ' % self.context.path()
        command = ''
        done = False
        
        interrupts = 0
        
        sys.stdout.flush()
        
        while not done:
            try:
                sys.stdout.write(prompt)
                sys.stdout.flush()
                input = sys.stdin.readline()
                
                command += input.strip()
            
                args = shlex.split(command)
                done = True
            except ValueError:
                prompt = '> '
            except IOError:
                interrupts += 1
                if interrupts == 2:
                    reactor.stop()
                    raise KeyboardInterrupt()
                
                print 'Interrupted'
                
                command = ''
                prompt = 'TS %s> ' % self.context.path()
                
        return args
    
    def ask(self):
        args = []
        
        while not self.context.async:
            args = self._read_args()
            
            if not args:
                continue
            
            if args[0] == 'exit':
                reactor.stop()
                return
            
            while args:
                try:
                    self.context, args = self.context.call(args)
                except Exception as e:
                    print >> sys.stderr, str(e)
                    args = []
    
    def call(self, callId):
        self.factory.call(callId, self.gotResponse, self.gotError)
    
    def gotResponse(self, response):
        self.context, args = self.context.doResponse(response)
        self.ask()
    
    def gotError(self, error, code):
        print >> sys.stderr, 'ERROR %d: %s' % (error, code)
        self.ask()
    
    def help(self, args):
        pass
    
client = TSClient()
client.setAgentType('admin', AdminCLI)

client.connect("localhost", 9090)
reactor.run()