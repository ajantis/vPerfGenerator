from tsload.jsonts import JSONTS
from tsload.jsonts.agent import TSAgent

HOST = 'localhost'
PORT = 9090

class TSWebAgent(TSAgent):
    connected = False
    
    def setEventListener(self, el):
        self.eventListener = el
    
    def gotAgent(self):
        self.connected = True
        self.eventListener.callback(self)
    
    def gotConnectionError(self, error):
        self.connectionError = error.getErrorMessage()
        self.eventListener.callback(self)
        
    