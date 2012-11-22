package com.vperflab.tsserver

import java.net._
import java.io._

import java.nio.channels.SocketChannel
import java.nio.ByteBuffer

import com.codahale.jerkson.Json._

abstract class Message {
  def id: Int
  
  def toJson() : String = {
    return generate(this)
  }
}

case class CommandMessage(id: Int, cmd: String, msg: Map[String, Any]) extends Message;
case class ResponseMessage(id: Int, response: Map[String, Any]) extends Message;
case class ErrorMessage(id: Int, error: String) extends Message;

case class MessageParserException(msg: String)
	extends Exception {}

object MessageParser {
  def fromJson(json: String) : Message = {
    val jsonMap = parse[Map[String, Any]](json) 
    var message: Message = null
    
    if((jsonMap contains "cmd") && (jsonMap contains "msg")) {
      message = parse[CommandMessage](json)
    }
    else if (jsonMap contains "response") {
      message = parse[ResponseMessage](json)
    }
    else if (jsonMap contains "error") {
      message = parse[ErrorMessage](json)
    }
    else {
      throw new MessageParserException("Received invalid JSON message")
    }
    
    return message
  }
}

class MessageHandler {
  
}

class TSAgentSender(var channel: SocketChannel) {
  var msgId = 0
  var msgHandlers: Map[Int, MessageHandler] = Map.empty
  
  def getMsgId() : Int = {
    msgId += 1
    
    return msgId
  }
  
  def send(message: Message) = {
    val msgString = message.toJson()
    var buffer = ByteBuffer.allocate(msgString.length + 4)
    
    buffer.put(msgString.getBytes())
    buffer.putInt(0)
    
    this.channel.synchronized {
      this.channel.write(buffer)
    }
  }
  
  def invoke(cmd: String, msg: Map[String, Any]) = {
    val message = new CommandMessage(getMsgId(), cmd, msg)
    
    send(message)
    
    /*Wait until response arrives*/
  }  
  
  def addResponse(srcId: Int, msg: Map[String, Any]) = {
    
  }
  
  def addError(srcId: Int, err: String) {
    
  }
  
  def respond(srcId: Int, response: Map[String, Any]) = {
    send(new ResponseMessage(srcId, response))
  }
  
  def reportError(srcId: Int, e: Exception) = {
    val errorMsg = e.toString()
    
    send(new ErrorMessage(srcId, errorMsg))
  }
}

class TSAgentReceiver(var channel: SocketChannel, 
    var sender: TSAgentSender,
    var server: TSServer) extends Runnable {
  var finished: Boolean = false
  
  def processCommand(srcId: Int, cmd: String, msg: Map[String, Any]) = {
    try {
      val response = server.processCommand(cmd, msg)
      sender.respond(srcId, response)
    }
    catch {
      case e: Exception => sender.reportError(srcId, e)
    }
    		
  }
  
  def processMessage(message: Message) = message match {
    case CommandMessage(id, cmd, msg) => 
      processCommand(id, cmd, msg)
    case ResponseMessage(id, response) => 
      sender.addResponse(id, response)
    case ErrorMessage(id, errorMsg) => 
      sender.addError(id, errorMsg)
  }
  
  def run() = {
    while(!finished) {
      val buffer = ByteBuffer.allocate(2048)
      
      channel.read(buffer)
      
      var byteArr : Array[Byte] = new Array[Byte](buffer.remaining());
	  buffer.get(byteArr);
	  val msgStr = new String(byteArr); 
      
      var message = MessageParser.fromJson(msgStr)
      this.processMessage(message)
      
      System.out.println(message)
    }
  }
}

class TSAgent(var socket: Socket, var server: TSServer) {
  var id: Int = -1
  
  var channel = socket.getChannel()
  
  var sender = new TSAgentSender(channel)
  
  var receiver = new TSAgentReceiver(channel, sender, server)
  var recvThread = new Thread(receiver)
  
  def start() {
    recvThread.start()
  }
  
  def stop() {
    /*XXX: close socket*/
     
    receiver.finished = true;
  }
  
  def register(id: Int) {
    this.id = id
  }
}