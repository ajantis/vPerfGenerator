import java.net._
import java.io._

class TSAgent(var socket: Socket) {
  var in = new BufferedReader(new InputStreamReader(socket.getInputStream()))

  var out = new PrintWriter(new OutputStreamWriter(socket.getOutputStream()))
  
  var id = -1
  
  def register(id: Int) {
    this.id = id
    
    val regMessage = in.readLine()
    System.out.println(regMessage)
    
    out.write("{\"agentId\": %d}".format(id))
  } 
}