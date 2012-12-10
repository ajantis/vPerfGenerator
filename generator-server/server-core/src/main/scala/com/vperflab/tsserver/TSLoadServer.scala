package com.vperflab.tsserver

import java.util.Date
import java.lang.{Boolean => JBoolean, Integer => JInteger, Double => JDouble}

import com.vperflab.agent.AgentService

/* @hello() */
class TSHostInfo extends TSObject {
  var hostName: String = _
}

class TSHelloResponse(agentId: Long) extends TSObject {
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
  var threadpool: String = _
  
  @TSObjContainer(elementClass = classOf[AnyRef])
  var params: Map[String, AnyRef] = _
}

class TSWorkloadList(wl_list: Map[String, TSWorkload]) extends TSObject {
  
  @TSObjContainer(elementClass = classOf[TSWorkload])
  val workloads = wl_list 
}

/* @workloadStatus() */

/*Sibling to wl_status_t*/
object TSWorkloadStatusCode {
  val CONFIGURING = 1
  val SUCCESS = 2
  val FAIL = 3
  val FINISHED = 4
}

class TSWorkloadStatus extends TSObject {
  var workload: String = _
  var status: Long = _
  var done: Long = _
  var message: String = _
}

/* provideStep() */

class TSProvideStepResult extends TSObject {
  var result: Long = _
}

/* @requestsReport() */

object TSRequestFlag {
  val STARTED  = 0x01
  val SUCCESS  = 0x02
  val ONTIME   = 0x04
  val FINISHED = 0x08
}

class TSRequest extends TSObject {
  var step: Long = _
  var request: Long = _
  var thread: Long = _
  
  var start: Long = _
  var end: Long = _
  
  var flags: Long = _
  
  override def toString(): String = "<RQ %d/%d@%d>".format(step, request, thread)
}

class TSRequestReport extends TSObject {
  @TSObjContainer(elementClass = classOf[TSRequest])
  var requests: List[TSRequest] = _
}
/* END OF TYPES */

trait TSLoadClient extends TSClientInterface{
  @TSClientMethod(name = "get_modules_info")
  def getModulesInfo() : TSModulesInfo
  
  @TSClientMethod(name = "configure_workloads", 
		  		  argNames = Array("workloads"), 
		  		  noReturn = true)
  def configureWorkloads(workloads: TSWorkloadList)
  
  @TSClientMethod(name = "start_workload", 
                  argNames = Array("workload_name", "start_time"), 
                  noReturn = true)
  def startWorkload(workloadName: String, startTime: Long) 
  
  @TSClientMethod(name = "provide_step", 
                  argNames = Array("workload_name", "step_id", "num_requests"))
  def provideStep(workloadName: String, stepId: Long, numRequests: Long) : TSProvideStepResult
}

class TSLoadServer(portNumber: Int) extends TSServer[TSLoadClient](portNumber) {
	@TSServerMethod(name = "hello", 
	                argNames = Array("info"))
	def hello(client: TSClient[TSLoadClient], info: TSHostInfo) : TSHelloResponse = {
	  val agentId = AgentService.registerLoadAgent(info.hostName)
	  
	  return new TSHelloResponse(agentId)
	}
	
	@TSServerMethod(name = "workload_status", 
				    argNames = Array("status"), 
				    noReturn = true) 
	def workloadStatus(client: TSClient[TSLoadClient], status: TSWorkloadStatus) = {
	  /* TODO: Workload status*/
	}
	
	@TSServerMethod(name = "requests_report", 
                    argNames = Array("requests"),
                    noReturn = true)
    def requestsReport(client: TSClient[TSLoadClient], report: TSRequestReport) {
	  for(rq <- report.requests) {
	    /* TODO: report requests */
	  }
	}
}