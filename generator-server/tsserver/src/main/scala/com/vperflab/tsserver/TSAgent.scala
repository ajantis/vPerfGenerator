import java.net._
import java.io._

import com.codahale.jerkson.Json._

case class Message(val cmd: String, val msg: Map[String, Any])

class TSAgent(var socket: Socket) {
  var in = new BufferedReader(new InputStreamReader(socket.getInputStream()))

  var out = new PrintWriter(new OutputStreamWriter(socket.getOutputStream()))
  
  var id = -1
  
  def register(id: Int) {
    this.id = id
    
    val regMessage = in.readLine()
    System.out.println(regMessage)
    System.out.println(parse[Message](regMessage))
    
    out.write("{\"agentId\": %d}".format(id))
  } 
}