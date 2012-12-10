import com.vperflab.tsserver.TSLoadServer

object TSSrvApplication extends App {
    var tsLoadServer = new TSLoadServer(9090)
  
	System.out.println("Starting tsserver...")
	
	var tsLoadSrvThread = new Thread(tsLoadServer)
	tsLoadSrvThread.start()
}