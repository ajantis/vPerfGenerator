package com.vperflab.tsserver

import java.net._
import java.io._

import java.lang.reflect.{Array => _, _}
import java.lang.annotation._

import java.nio.channels.ServerSocketChannel

import scala.collection.immutable._
import scala.collection.mutable.{Map => MutableMap}

import java.util.{LinkedHashMap}
import scala.collection.JavaConversions._

case class TSCommandNotFound(msg: String)
	extends Exception(msg) {}

case class TSMissingArgument(msg: String) 
	extends Exception(msg) {}

case class TSMissingField(msg: String) 
	extends Exception(msg) {}

class TSServer(portNumber: Int) {
	var tsServerSocket = ServerSocketChannel.open()
	
	val agentsList = List[TSAgent]()
	
	var agentId = 0
	
	def start() = {
	  tsServerSocket.socket().bind(new InetSocketAddress(portNumber))
	  
	  while(true) {
	    val clientSocket = tsServerSocket.accept()
	    val tsAgent = new TSAgent(clientSocket, this)
	    
	    tsAgent.setId(agentId)
	    tsAgent.start()
	    	    
	    agentId += 1
	  }
	}
	
	def findCommand(cmd: String) : (Method, TSServerMethod) = {
	  val methods = classOf[TSServer].getMethods()
	  
	  for(method <- classOf[TSServer].getMethods()) {
	    val annotation = method.getAnnotation(classOf[TSServerMethod])
	    
	    if(annotation != null) {
	      val tsAnnotation = annotation.asInstanceOf[TSServerMethod]
	      
	      if(tsAnnotation.name == cmd)
	        return (method, tsAnnotation);
	    }
	  }
	  
	  throw new TSCommandNotFound("Not found command %s".format(cmd))
	}
	
	def convertSingleArgument(argMap: Map[String, Any], klass: Class[_]) : AnyRef = {
	  var arg = klass.newInstance()
	  
	  for(field <- klass.getDeclaredFields) {
	    val name = field.getName
	    
	    field.setAccessible(true)
	    field.set(arg, argMap(name))
	  }
	  
	  return arg.asInstanceOf[AnyRef]
	}
	
	def convertArgMap(anyArgMap: Any) : Map[String, Any] = {
	  var argHashMap = anyArgMap.asInstanceOf[LinkedHashMap[String, Any]]
	  var argMap : Map[String, Any] = mapAsScalaMap(argHashMap).toMap
	  
      return argMap
	}
	
	def convertArguments(agent: TSAgent, msg: Map[String, Any], argList: List[String], classList: List[Class[_]]) : Array[AnyRef] = {
	  var args: List[AnyRef] = List(agent)
	  var argClassMap = (argList zip classList).toMap
	   
	  for((argName, klass) <- argClassMap) {	
	    val argMap = convertArgMap(msg(argName))
	    val arg = convertSingleArgument(argMap, klass)
	    
	    args = args ::: List(arg)
	  }
	  
	  System.out.println(args)
	  
	  return args.toArray
	}
	
	def convertReturnValue(ret: Any) : Map[String, Any] = {
	  var retMap = MutableMap[String, Any]()
	  
	  for(field <- ret.getClass.getDeclaredFields) {
	    val name = field.getName
	    
	    retMap += name -> field.get()
	  }
	  
	  return retMap.toMap
	}
	
	def processCommand(agent: TSAgent, cmd: String, msg: Map[String, Any]) : Map[String, Any] = {
	  System.out.println("Processing %s(%s)".format(cmd, msg))	  
	  val (method, tsAnnotation) = findCommand(cmd)
	  
	  val argList = tsAnnotation.argArray.toList
	  val classList = method.getParameterTypes().toList.drop(1)
	  
	  var args = convertArguments(agent, msg, argList, classList)
	  
	  System.out.println("Got method %s and args %s".format(method, args))
	  
	  val ret = method.invoke(this, args:_*)
	  
	  if(tsAnnotation.noReturn) {
		return Map.empty
	  }
	  else {
	    return convertReturnValue(ret)
	  }
	}
	
	@TSServerMethod(name = "hello", argArray = Array("info"), noReturn = true)
	def hello(agent: TSAgent, info: TSHostInfo) = {
	  System.out.println("Say hello from %s!".format(info.getHostName()))
	}
}