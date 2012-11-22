package com.vperflab.tsserver

import java.net._
import java.io._

class TSServer(portId: Int) {
	val tsServerSocket = new ServerSocket(portId)
	
	val agentsList = List[TSAgent]()
	
	var agentId = 0
	
	def start() = {
	  while(true) {
	    val clientSocket = tsServerSocket.accept()
	    val tsAgent = new TSAgent(clientSocket, this)
	    
	    tsAgent.register(agentId)
	    	    
	    agentId += 1
	  }
	}
	
	def processCommand(cmd: String, msg: Map[String, Any]) : Map[String, Any] = {
	  return Map.empty
	}
}