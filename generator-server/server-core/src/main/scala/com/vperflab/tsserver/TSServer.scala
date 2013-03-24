package com.vperflab.tsserver

import java.net.{Proxy => NetworkProxy, _}
import java.io._

import java.lang.reflect.{Array => _, _}
import java.lang.annotation._

import java.nio.channels.ServerSocketChannel

import scala.collection.immutable._
import scala.collection.mutable.{Map => MutableMap}

import scala.reflect.Manifest

/**
 * TSClientInvocationHandler - proxies requests to client 
 *
 * Requests defined in trait derived from TSClientInterface and passed as 
 * type argument
 */
class TSClientInvocationHandler[CI <: TSClientInterface](client: TSClient[CI])
  extends InvocationHandler {

  def doTrace(msg: String) {
    System.out.println(msg)
  }

  /**
   * @return  TSClientMethod annotation for method
   * */
  def getMethodInfo(method: Method) : TSClientMethod = {
    val annotation = method.getAnnotation(classOf[TSClientMethod])

    if(annotation == null) {
      throw new TSClientError("Method " + method + " has no TSClientMethod annotation")
    }

    annotation
  }

  /**
   * Invoke client's method
   *
   * 1. Serialize arguments
   * 2. Call client.invoke
   * 3. Deserialize and return response
   *
   * @return response
   */
  def invoke(proxy: Any, method: Method, args: Array[Object]) : Object = {
    val retClass = method.getReturnType
    val methodInfo = getMethodInfo(method)

    val argNamesList = methodInfo.argNames.toList
    val classList = method.getParameterTypes.toList

    val argList = args match {
      case null => Nil
      case _ => args.toList
    }

    var argMap = (argNamesList zip (classList zip argList)).toMap

    var msg = MutableMap[String, Any]()

    for((argName, (argClass, argValue)) <- argMap) {
      msg += argName -> TSObjectSerializer.doSerialize(argValue, argClass)
    }

    doTrace("Invoking %s(%s)".format(methodInfo.name, msg.toMap))

    val ret = client.invoke(methodInfo.name, msg.toMap)

    if(methodInfo.noReturn) {
      Map.empty
    }
    else {
      TSObjectDeserializer.doDeserialize(ret, retClass).asInstanceOf[AnyRef]
    }
  }
}

/**
 * TSServer - multiplexes connections on socket from clients
 *
 * Uses reflection to find appropriate method for incoming commands
 *
 * There are two possible clients: 
 * - tsloadd - TS Loader daemon (handled by TSLoadServer) 
 * - tsmond - TS Monitor daemon (handled by TSMonitorServer)
 */
abstract class TSServer[CI <: TSClientInterface](port: Int)(implicit ciManifest: Manifest[CI])
  extends Runnable {

  var finished = false

  var tsServerSocket = ServerSocketChannel.open()

  val clientsList = List[TSClient[CI]]()

  def doTrace(msg: String) {
    System.out.println(msg)
  }

  /**
   * Main thread method
   *
   * Creates listen socket on port
   * Accepts connections from it and routes it to TSClient
   */
  def run(){
    tsServerSocket.socket().bind(new InetSocketAddress(port))

    while(!finished) {
      val clientSocket = tsServerSocket.accept()
      val tsClient = new TSClient[CI](clientSocket, this)

      createClientInterface(tsClient)

      tsClient.start()

    }
  }

  def createClientInterface(client: TSClient[CI]) {
    val handler = new TSClientInvocationHandler[CI](client)
    val interface = Proxy.newProxyInstance(ciManifest.erasure.getClassLoader,
      Array(ciManifest.erasure), handler).asInstanceOf[TSClientInterface]

    client.setInterface(interface)
  }

  /**
   * Searches over methods annotated with TSServerMethod
   * Select appropriate method or throws TSCommandNotFound exception
   *
   * @param cmd - command name
   * @return pair (java.reflect.Method, TSServerMethod annotation)
   * */
  def findCommand(cmd: String) : (Method, TSServerMethod) = {
    val methods = this.getClass.getMethods

    val methodsWithAnnotations = for {
      method <- methods
      tsAnnotation = method.getAnnotation(classOf[TSServerMethod])
      if tsAnnotation != null && tsAnnotation.name() == cmd
    } yield (method, tsAnnotation)

    methodsWithAnnotations.headOption match {
      case Some(p) => p
      case _ => throw new TSClientCommandNotFound("Not found command %s".format(cmd))
    }
  }

  /**
   * Coverts arguments for method from incoming message according
   * to method's arguments and TSServerMethod specification
   *
   * @param client - client that invoked command (always passed as first argument of server method)
   * @param msg - CommandMessage arguments
   * @param argNamesList - list of argument's names according to JSON protocol
   * @param classList - list of argument's types (from reflection)
   *
   * @return array of arguments
   */
  def convertArguments(client: TSClient[CI],
                       msg: Map[String, Any],
                       argNamesList: List[String],
                       classList: List[Class[_]]) : Array[AnyRef] = {
    var args: List[AnyRef] = List(client)
    val argClassMap = (argNamesList zip classList).toMap

    for((argName, klass) <- argClassMap) {
      try {
        val argMap = TSObjectSerializer.jsonMapToMap(msg(argName))
        val arg = TSObjectDeserializer.doDeserialize(argMap, klass)

        args = args ::: List(arg.asInstanceOf[AnyRef])
      }
      catch {
        case e: NoSuchElementException => throw new TSClientMessageFormatError("Missing argument " + argName)
      }
    }

    args.toArray
  }

  /**
   * Processes incoming command from client:
   *
   * 1. Select appropriate TSServer method
   * 2. Convert arguments from JSON according to their classes
   * 3. Invoke method using reflection
   *
   * @param client invoking client
   * @param cmd command name
   * @param msg command message (contains arguments)
   *
   * @return Map.empty in case of error or serialized response
   */
  def processCommand(client: TSClient[CI], cmd: String, msg: Map[String, Any]) : Map[String, Any] = {
    doTrace("Processing %s(%s)".format(cmd, msg))

    val (method, tsAnnotation) = findCommand(cmd)

    val argNamesList = tsAnnotation.argNames.toList
    /* Remove first argument for arguments list because it is always TSClient */
    val classList = method.getParameterTypes.toList.drop(1)

    val args = convertArguments(client, msg, argNamesList, classList)

    /* doTrace("Got method %s and args %s".format(method, args)) */

    val ret = method.invoke(this, args:_*)

    if(tsAnnotation.noReturn) {
      doTrace("Returning {}")
      Map.empty
    }
    else {
      doTrace("Returning" + ret)
      TSObjectSerializer.doSerializeObject(ret)
    }
  }

  def processClientDisconnect(client: TSClient[CI])

}