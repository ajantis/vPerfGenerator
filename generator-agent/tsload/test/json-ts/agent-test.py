#!/usr/bin/python

import SocketServer
import time

from jsonts import *

NUM_SENDS = 20
PORT = 9090

class RequestHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        hello = self.request.recv(1024)
        
        print "Received hello msg:", hello
        
        for i in range(NUM_SENDS):
            msg = createCommandMsg("get_modules_info", {})            
            self.request.sendall(serializeMsg(msg))
            
            print 'Sent command #%d' % i

class TSServer(SocketServer.TCPServer):
    allow_reuse_address = True

server = TSServer(("localhost", PORT), RequestHandler)
server.serve_forever()