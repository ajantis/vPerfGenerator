import java.net._
import java.io._

import com.codahale.jerkson.Json._

abstract class Message {
  var id: Int = -1
  
  def fromJsonMap(jsonMap: Map[String, Any])
  
  def toJson() : String 
}

class CommandMessage extends Message {
  var command: String = ""
  var message: Map[String, Any] = Map.empty
  
  def fromJsonMap(jsonMap: Map[String, Any]) = {
    this.id = jsonMap("id").asInstanceOf[Int]
    this.command = jsonMap("cmd").asInstanceOf[String]
    this.message = jsonMap("msg").asInstanceOf[Map[String, Any]]
  }
  
  def toJson() : String = {
    return generate(this)
  }
}

case class MessageParserException(msg: String)
	extends Exception {}

object MessageParser {
  def fromJson(json: String) : Message = {
    val jsonMap = parse[Map[String, Any]](json)
    
    if(jsonMap contains "cmd") {
      val message = new CommandMessage()
      message.fromJsonMap(jsonMap)
      
      return message
    }
    
    throw new MessageParserException("Received invalid JSON message")
  }
}

class TSAgent(var socket: Socket) {
  var in = new BufferedReader(new InputStreamReader(socket.getInputStream()))

  var out = new PrintWriter(new OutputStreamWriter(socket.getOutputStream()))
  
  var id = -1
  
  def register(id: Int) {
    this.id = id
    
    val regMessage = in.readLine()
    System.out.println(MessageParser.fromJson(regMessage))
    
    out.write("{\"agentId\": %d}".format(id))
  } 
}