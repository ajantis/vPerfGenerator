package com.vperflab.tsserver

import java.net._
import java.io._

import java.nio.ByteBuffer
import java.nio.charset.Charset
import java.nio.channels.SocketChannel

import com.codahale.jerkson.Json._

import scala.collection.mutable.{Map => MutableMap}

import scala.actors.Actor
import scala.actors.Actor._

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
	extends Exception(msg) {}

case class TSClientError(msg: String)
	extends Exception(msg) {}

/**
 * MessageParser - factory that parses incoming json message
 * and creates one of Message instances
 */
object MessageParser {
  /**
   * Parse incoming JSON message and create Message object for it.
   * 
   * returns: 
   * 	CommandMessage for messages
   * 	ResponseMessage for responses
   * 	ErrorMessage for errors
   * 
   * Note: all messages are parsed as Map[String, Any], where values 
   * are LinkHashMap's for JSON nodes, ArrayLists for JSON arrays or
   * primitive types (Int, Long, Double, String).
   * 
   * All futher deserialization to TSObjects are provided by @see TSObjectSerdes 
   * 
   * @param json json message (String)
   * @return  Message instance
   *  */
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

class MessageHandler(srcId: Int) {
  var response: Message = _
  
  def getResponse : Message = response
  
  def setResponse(newResponse: Message) {
    response = newResponse
  }
}

/**
 * TSClientSender - sends messages into SocketChannel
 */
class TSClientSender(var channel: SocketChannel) {
  var msgId = 0
  var msgHandlers = MutableMap[Int, MessageHandler]()
  
  /**
   * Assigns message id (incrementing counter)
   * 
   * TODO: Thread-safe
   * 
   * @return message id
   */
  def getMsgId() : Int = {
    msgId += 1
    
    return msgId
  }
  
  /**
   * Send message to channel. Formats message as JSON, copies
   * it to ByteBuffer than synchronously writes it into channel
   * 
   * May block on channel
   * 
   * @param message - Message instance
   */
  def send(message: Message) = {
    val msgString = message.toJson()
    val msgByteArray = msgString.getBytes()
    
    var buffer = ByteBuffer.allocate(msgByteArray.length + 16)
    
    buffer.put(msgByteArray)
    buffer.putInt(0)
    
    this.channel.synchronized {
      this.channel.write(buffer)
    }
  }
  
  /**
   * Invoke remote agent's command
   * 
   * @param cmd 	command to be invoked
   * @param msg 	command's arguments
   * 
   * @return ResponseMessage or throws Exception
   * */
  def invoke(cmd: String, msg: Map[String, Any]) : Map[String, Any] = {
    val msgId = getMsgId()
    
    val message = new CommandMessage(msgId, cmd, msg)
    val msgHandler = new MessageHandler(msgId)
    
    this.msgHandlers.synchronized {
      this.msgHandlers += msgId -> msgHandler
    }
    
    send(message)
    
    /*Wait until response arrives*/
    msgHandler.synchronized {
    	msgHandler.wait
    }
    
    return msgHandler.getResponse match {
      case ResponseMessage(id, response) => response
	  case ErrorMessage(id, errorMsg) => throw new TSClientError(errorMsg)
    }
  }  
  
  /**
   * (private) Wake up thread that invoked something on agent
   * and respond normal message or error to it. Called from receiver
   * 
   * @param srcId: id of sent CommandMessage
   * @param response: incoming response 
   */
  def addResponse(response: Message) = {
    this.msgHandlers.synchronized {
      val srcId = response.id
      val msgHandler = this.msgHandlers(srcId) 
      this.msgHandlers -= srcId
      
      msgHandler.setResponse(response)
      
      /* Wake up handler */
      msgHandler.synchronized {
      	msgHandler.notify
      }
    }
  }
  
  /**
   * Respond to agent with normal response
   * 
   * @param srcId: id of incoming CommandMessage
   * @param response: outgoing response
   */
  def respond(srcId: Int, response: Map[String, Any]) = {
    send(new ResponseMessage(srcId, response))
  }
  
  /**
   * Report an error occured in server during processing of command message
   * 
   * @param srcId: id of incoming CommandMessage
   * @param e: catched exception
   */
  def reportError(srcId: Int, e: Exception) = {
    val errorMsg = e.toString()
    
    System.out.println(e.toString())
    e.printStackTrace()
    
    send(new ErrorMessage(srcId, errorMsg))
  }
}

class TSClientProcessor[CI <: TSClientInterface](var client: TSClient[CI])
  	extends Actor {
  var finished: Boolean = false
  
  /**
   * Process incoming command from agent
   * 
   * 1. Dispatch command to server via server.process.Command
   * 2. Respond if possible via sender.respond
   * 3. Report an error via sender.reportError
   * */
  def processCommand(srcId: Int, cmd: String, msg: Map[String, Any]) = {
    try {
      val response = client.server.processCommand(client, cmd, msg)
      client.sender.respond(srcId, response)
    }
    catch {
      case e: Exception => client.sender.reportError(srcId, e)
    }
  }
  
  /**
   * Process incoming message
   */
  def act() {
    while(!finished) {
      receive {
	    case CommandMessage(id, cmd, msg) => 
	      processCommand(id, cmd, msg)
	    case message: ResponseMessage => 
	      client.sender.addResponse(message)
	    case message: ErrorMessage => 
	      client.sender.addResponse(message)
	  }
    }
  }
}

/**
 * TSClientReceiver - receiver thread. Processes incoming messages from
 * agent and sents a response/errors or reports it to agent
 * 
 * See also libtscommon -> clnt_recv_thread
 * */
class TSClientReceiver[CI <: TSClientInterface](var channel: SocketChannel, 
    var client: TSClient[CI]) extends Runnable {
  
  /** If set to true, thread will finish after next receive */
  var finished: Boolean = false
  
  var processor = new TSClientProcessor[CI](client)
  
  /**
   * Helper to decode ByteBuffer into JSON string
   * 
   * @note Uses UTF-8 as default encoding
   */
  def decodeByteBuffer(buffer: ByteBuffer) : String = {
    val decoder = Charset.forName("UTF-8").newDecoder()
    
    buffer.rewind()
    val data = decoder.decode(buffer).toString()
    
    return data
  }
  
  /**
   * Main thread method. Receives and processes message from channel
   * 
   * NOTE: each processing is synchronous, so if agent invokes two commands simultaneously,
   * they will be processed sequentally
   * 
   * XXX: constant ByteBuffer size
   */
  def run() = {
    processor.start
    
    while(!finished) {
      val buffer = ByteBuffer.allocate(2048)
      
      channel.read(buffer)
      
      val msgStr = decodeByteBuffer(buffer)
      var message = MessageParser.fromJson(msgStr)
      
      processor ! message
    }
    
    processor.finished = true
  }
}

trait TSClientInterface 

/**
 * TSClient - client for connecting agent
 */
class TSClient[CI <: TSClientInterface](var channel: SocketChannel, var server: TSServer[CI])  {
  var id: Int = -1
  
  var sender = new TSClientSender(channel)
  
  var receiver = new TSClientReceiver[CI](channel, this)
  var recvThread = new Thread(receiver)
  
  var interface: TSClientInterface = _
  
  def invoke = sender.invoke(_, _)
  
  def setId(id: Int) {
    this.id = id
  }
  
  def start() {
    recvThread.start()
  }
  
  def stop() {
    /*XXX: close socket*/
     
    receiver.finished = true;
  }
  
  def getInterface: CI = this.interface.asInstanceOf[CI]
  
  def setInterface(newInterface: TSClientInterface) = {
    interface = newInterface
  }
}