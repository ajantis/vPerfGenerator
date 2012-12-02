package com.vperflab.tsserver

import java.lang.{Boolean => JBoolean, Integer => JInteger, Double => JDouble}

/* @hello() */
class TSHostInfo extends TSObject {
  var hostName: String = _
}

class TSHelloResponse(agentId: Int) extends TSObject {
  val agent_id: Long = agentId 
}

/* getModulesInfo() */
@TSObjAbstract(fieldName = "type",
    classMap = Array("integer", "TSWLParamIntegerInfo",
     "bool", "TSWLParamBooleanInfo",
     "float", "TSWLParamFloatInfo",
     "size", "TSWLParamSizeInfo",
     "string", "TSWLParamStringInfo",
     "strset", "TSWLParamStringSetInfo"))
abstract class TSWorkloadParamInfo extends TSObject {
  var description: String = _
};

class TSWLParamBooleanInfo extends TSWorkloadParamInfo

class TSWLParamIntegerInfo extends TSWorkloadParamInfo {
  var min: Long = _
  var max: Long = _
}

class TSWLParamFloatInfo extends TSWorkloadParamInfo {
  var min: Double = _
  var max: Double = _
}

class TSWLParamSizeInfo extends TSWorkloadParamInfo {
  var min: Long = _
  var max: Long = _
}

class TSWLParamStringInfo extends TSWorkloadParamInfo {
  var len: Long = _
}
      
class TSWLParamStringSetInfo extends TSWorkloadParamInfo {
  @TSObjContainer(elementClass = classOf[String])
  var strset: List[String] = _
}

class TSSingleModuleInfo extends TSObject {
  var path: String = _
  
  @TSObjContainer(elementClass = classOf[TSWorkloadParamInfo])
  var params: Map[String, TSWorkloadParamInfo] = _
}

class TSModulesInfo extends TSObject {
  @TSObjContainer(elementClass = classOf[TSSingleModuleInfo])
  var modules: Map[String, TSSingleModuleInfo] = _
}

/* configureWorkload() */
class TSWorkload extends TSObject {
  var module: String = _
  
  @TSObjContainer(elementClass = classOf[AnyRef])
  var params: Map[String, AnyRef] = _
}

class TSWorkloadList(wl_list: Map[String, TSWorkload]) extends TSObject {
  
  @TSObjContainer(elementClass = classOf[TSWorkload])
  val workloads = wl_list 
}

/* workloadStatus() */
class TSWorkloadStatus extends TSObject {
  var workload: String = _
  var status: String = _
  var done: Long = _
  var message: String = _
}

trait TSLoadClient extends TSClientInterface{
  @TSClientMethod(name = "get_modules_info")
  def getModulesInfo() : TSModulesInfo
  
  @TSClientMethod(name = "configure_workloads", argNames = Array("workloads"), noReturn = true)
  def configureWorkloads(workloads: TSWorkloadList) 
}


class TSLoadServer(portNumber: Int) extends TSServer[TSLoadClient](portNumber) {
	@TSServerMethod(name = "hello", argNames = Array("info"))
	def hello(client: TSClient[TSLoadClient], info: TSHostInfo) : TSHelloResponse = {
	  System.out.println("Say hello from %s!".format(info.hostName))
	  
	  /*TODO: get agentId for it*/
	  
	  val modules = client.getInterface.getModulesInfo()
	  
	  System.out.println(modules)
	  
	  startTestWorkload(client)
	  
	  return new TSHelloResponse(0)
	}
	
	@TSServerMethod(name = "workload_status", argNames = Array("status"), noReturn = true) 
	def workloadStatus(client: TSClient[TSLoadClient], status: TSWorkloadStatus) = {
	  System.out.println("Workload %s status %s,%d: %s".format(
	      status.workload, status.status, status.done, status.message))
	}
	
	def startTestWorkload(client: TSClient[TSLoadClient]) {
	  var workload = new TSWorkload()
	  
	  workload.module = "dummy"
	  workload.params = Map("filesize" -> (16777216 : JInteger),
	        "blocksize" -> (4096 : JInteger),
	        "path" -> "/tmp/testfile",
	        "test" -> "read",
	        "sparse" -> (false : JBoolean))
	  
	  var workloads = new TSWorkloadList(Map("test" -> workload))
	        
	  client.getInterface.configureWorkloads(workloads)
	}
}