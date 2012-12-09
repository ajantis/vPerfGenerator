package com.vperflab.tsserver

import java.net._
import java.io._

import java.nio.ByteBuffer
import java.nio.charset.Charset
import java.nio.channels.SocketChannel

import com.codahale.jerkson.Json._

import scala.collection.mutable.{Map => MutableMap, ArrayBuffer}

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
case class ErrorMessage(id: Int, code: Int, error: String) extends Message;

case class Stop

case class MessageParserException(msg: String)
	extends Exception(msg) 

case class TSClientError(msg: String)
	extends Exception(msg) 
case class TSClientCommandNotFound(cmsg: String)
	extends TSClientError(cmsg)
case class TSClientMessageFormatError(cmsg: String)
	extends TSClientError(cmsg)
case class TSClientInvalidData(cmsg: String)
	extends TSClientError(cmsg)

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
    else if ((jsonMap contains "error") && (jsonMap contains "code")) {
      message = parse[ErrorMessage](json)
    }
    else {
      throw new MessageParserException("Received invalid JSON message")
    }
    
    return message
  }
}

object ErrorFactory {
	val AE_COMMAND_NOT_FOUND = 100
	val AE_MESSAGE_FORMAT	 = 101
	val AE_INVALID_DATA		 = 102
	val AE_INTERNAL_ERROR	 = 200
	
	def createException(msg: ErrorMessage): TSClientError = {
	  return msg.code match {
	    case AE_COMMAND_NOT_FOUND => new TSClientCommandNotFound(msg.error)
	    case AE_MESSAGE_FORMAT => new TSClientMessageFormatError(msg.error)
	    case AE_INVALID_DATA => new TSClientInvalidData(msg.error)
	    
	    case _ => new TSClientError(msg.error)
	  }
	}
	
	def createErrorMessage(msgId: Int, e: Exception): ErrorMessage = {
	  return e match {
	    case TSClientCommandNotFound(msg) => new ErrorMessage(msgId, AE_COMMAND_NOT_FOUND, msg)
	    case TSClientMessageFormatError(msg) => new ErrorMessage(msgId, AE_MESSAGE_FORMAT, msg)
	    case TSClientInvalidData(msg) => new ErrorMessage(msgId, AE_INVALID_DATA, msg)
	    
	    case _ => new ErrorMessage(msgId, AE_INTERNAL_ERROR, e.toString())
	  }
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
    val nullTerm: Byte = 0
    
    var buffer = ByteBuffer.allocate(msgByteArray.length + 1)
    
    buffer.put(msgByteArray)
    /* All messages are null-terminated */
    buffer.put(nullTerm)
    
    buffer.rewind()
    
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
	  case message: ErrorMessage => throw ErrorFactory.createException(message)
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
    
    send(ErrorFactory.createErrorMessage(srcId, e))
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
	      /* Command invokation may be cascade,
	       * so create new actor for it */
	      actor {
	      	processCommand(id, cmd, msg)
	      }
	    case message: ResponseMessage => 
	      client.sender.addResponse(message)
	    case message: ErrorMessage => 
	      client.sender.addResponse(message)
	    case Stop =>
	      finished = true
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
  
  var CHUNK_SIZE = 2048
  
  /** If set to true, thread will finish after next receive */
  var finished: Boolean = false
  
  var bufBytes = new ArrayBuffer[Byte]()
  
  var processor = new TSClientProcessor[CI](client)
  
  val traceDecode = false
  
  /**
   * Helper to decode ByteBuffer into JSON string
   * 
   * @note Uses UTF-8 as default encoding
   * 
   * XXX: does we really need decoding?
   */
  private def decodeMessage(buffer: ByteBuffer) = {
    val decoder = Charset.forName("UTF-8").newDecoder()
    
    buffer.rewind()
    val msgStr = decoder.decode(buffer).toString
    
    if(traceDecode)
      System.out.println("Message: " + msgStr)
    
    var message = MessageParser.fromJson(msgStr)
	processor ! message
  }
  
  /**
   * Decode messages from bytes sequence
   * */
  private def decodeBytes(bytes: ArrayBuffer[Byte]) {
    var position = 0
    var lastPosition = 0

    while(position < bytes.length) {
      /*Found null-terminator - this is end of message*/
      if(bytes(position) == 0) {
        val len = position - lastPosition
        
        /* Discard zero-length message (if two null-terminator
         * follows null-terminator) */
        if(len > 0) {
          /* Put data into ByteBuffer to use java.nio again */
          val msgBytes = bufBytes.slice(lastPosition, position).toArray
          var msgBuffer = ByteBuffer.wrap(msgBytes) 
          
          if(traceDecode)
          	System.out.println("Decoding message in range [" + lastPosition + ";" + position)
          
          decodeMessage(msgBuffer)
        }
        
        lastPosition = position + 1
      } 
      
      position += 1
    }
  }
  
  /**
   * Decode messages from buffer
   * */
  private def decodeByteBuffer(buffer: ByteBuffer, length: Int) {
    /* Read bytes from buffer */
    var chunkBytes = new Array[Byte](length)
    
    buffer.rewind()
    buffer.get(chunkBytes)
  
    /* Accumulate bytes in bufBytes*/
    bufBytes = bufBytes ++ chunkBytes
    
    /* If there was last message (socket closed) 
     * or chunk ended  */
    if(length == 0 || bufBytes.last == 0) {
      decodeBytes(bufBytes)
      
      /*Reset bufBytes*/
      bufBytes = new ArrayBuffer[Byte]()
  	}
  }
  
  /**
   * Main thread method. Receives and processes message from channel
   * 
   * NOTE: each processing is synchronous, so if agent invokes two commands simultaneously,
   * they will be processed sequentally
   * 
   * Message decoding is a bit tricky, so it requires some comments. JSON messages 
   * are split by null-terminators, so here is path of message decoding:
   * 
   * 1. Receiver reads chunk of CHUNK_SIZE bytes from socket
   *    If it reads -1 bytes, that means that socket disconnected
   * 2. decodeByteBuffer converts buffer into array of bytes
   *    If length of array is zero, or it is ended with null-terminator, 
   *    we found end of message, and can decode them (go to step 3)
   *    Otherwise, we accumulate bytes in bufBytes
   * 3. Decoding messages. Loop over bufBytes. When found null-terminator,
   *    slice bufBytes, convert slice into ByteBuffer (because we are using
   *    java.nio again), decode it in decodeMessages and send it to processor actor. 
   */
  def run() = {
    processor.start
    
    while(!finished) {
      val buffer = ByteBuffer.allocate(CHUNK_SIZE)
      val readNum = channel.read(buffer)
      
      if(traceDecode)
        System.out.println("Read " + readNum + " bytes to " + buffer)
      
      decodeByteBuffer(buffer, readNum match {
          case -1 => 0
          case _ => readNum
        });
      
      if(readNum == -1) {
        /*Disconnect*/
        finished = true
      }
    }
    
    processor ! Stop
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