'''
Created on 09.05.2013

@author: myaut
'''

from tsload.jsonts.server import TSServer
from twisted.internet import reactor

server = TSServer.createServer(9090)
reactor.run()