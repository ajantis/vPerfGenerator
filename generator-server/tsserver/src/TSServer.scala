import java.net._
import java.io._

object TSServer {
	val tsServerSocket = new ServerSocket(9090)
	
	val agentsList = List[TSAgent]()
	
	var agentId = 0
	
	def start() {
	  while(true) {
	    val clientSocket = tsServerSocket.accept()
	    val tsAgent = new TSAgent(clientSocket)
	    
	    tsAgent.register(agentId)
	    	    
	    agentId += 1
	  }
	}
}