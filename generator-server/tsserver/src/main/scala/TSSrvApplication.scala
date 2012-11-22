import com.vperflab.tsserver.TSServer

object TSSrvApplication extends App {
    var tsServer = new TSServer(9090)
  
	System.out.println("Starting tsserver...")
	
	tsServer.start()
}