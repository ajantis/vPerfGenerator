package com.vperflab.tsserver

import java.util.Date
import java.lang.{Boolean => JBoolean, Integer => JInteger, Double => JDouble}

import com.vperflab.agent.AgentService
import collection.mutable

/* @hello() */
class TSHostInfo extends TSObject {
  var hostName: String = _
}

class TSHelloResponse(agentId: String) extends TSObject {
  val agent_id: String = agentId
}

/* getModulesInfo() */
object TSWorkloadParamFlag {
  val OPTIONAL = 0x01
}


@TSObjAbstract(fieldName = "type",
    classMap = Array("integer", "TSWLParamIntegerInfo",
     "bool", "TSWLParamBooleanInfo",
     "float", "TSWLParamFloatInfo",
     "size", "TSWLParamSizeInfo",
     "string", "TSWLParamStringInfo",
     "strset", "TSWLParamStringSetInfo"))
abstract class TSWorkloadParamInfo extends TSObject {
  var flags: Long = _
  var description: String = _
}

class TSWLParamBooleanInfo extends TSWorkloadParamInfo {
  @TSOptional
  var default: Boolean = _
}

class TSWLParamIntegerInfo extends TSWorkloadParamInfo {
  @TSOptional
  var min: Long = _
  
  @TSOptional
  var max: Long = _
  
  @TSOptional
  var default: Long = _
}

class TSWLParamFloatInfo extends TSWorkloadParamInfo {
  @TSOptional
  var min: Double = _
  
  @TSOptional
  var max: Double = _
  
  @TSOptional
  var default: Double = _
}

class TSWLParamSizeInfo extends TSWorkloadParamInfo {
  @TSOptional
  var min: Long = _
  
  @TSOptional
  var max: Long = _
  
  @TSOptional
  var default: Long = _
}

class TSWLParamStringInfo extends TSWorkloadParamInfo {
  var len: Long = _
  
  @TSOptional
  var default: String = _
}
      
class TSWLParamStringSetInfo extends TSWorkloadParamInfo {
  @TSObjContainer(elementClass = classOf[String])
  var strset: List[String] = _
  
  @TSOptional
  var default: Long = _
}

class TSWorkloadType extends TSObject {
  var module: String = _
  var path: String = _
  
  @TSObjContainer(elementClass = classOf[TSWorkloadParamInfo])
  var params: Map[String, TSWorkloadParamInfo] = _
}

class TSWorkloadTypeList extends TSObject {
  @TSObjContainer(elementClass = classOf[TSWorkloadType])
  var wltypes: Map[String, TSWorkloadType] = _
}

/* configureWorkload() */
class TSWorkload extends TSObject {
  var wltype: String = _
  var threadpool: String = _
  
  @TSObjContainer(elementClass = classOf[AnyRef])
  var params: Map[String, AnyRef] = _
}

/* @workloadStatus() */

/*Sibling to wl_status_t*/
object TSWorkloadStatusCode {
  val CONFIGURING = 1
  val SUCCESS = 2
  val FAIL = 3
  val FINISHED = 4
  val RUNNING = 5
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
  var workload_name: String = _
  
  var step: Long = _
  var request: Long = _
  var thread: Long = _
  
  var start: Long = _
  var end: Long = _
  
  var flags: Long = _
  
  override def toString: String = "<RQ %d/%d@%d>".format(step, request, thread)
}

class TSRequestReport extends TSObject {
  @TSObjContainer(elementClass = classOf[TSRequest])
  var requests: List[TSRequest] = _
}

/* getThreadPools() */

class TSThreadPool extends TSObject {
  var num_threads: Long = _
  var quantum: Long = _
  
  var wl_count: Long = _
  
  @TSObjContainer(elementClass = classOf[String])
  var wl_list: List[String] = _
}

class TSThreadPoolList extends TSObject {
  @TSObjContainer(elementClass = classOf[String])
  var dispatchers: List[String] = _
  
  @TSObjContainer(elementClass = classOf[TSThreadPool])
  var threadpools: Map[String, TSThreadPool] = _
}

/* END OF TYPES */

trait TSLoadClient extends TSClientInterface{
  @TSClientMethod(name = "get_workload_types")
  def getWorkloadTypes : TSWorkloadTypeList
  
  @TSClientMethod(name = "configure_workload", 
		  		  argNames = Array("workload_name", "workload_params"), 
		  		  noReturn = true)
  def configureWorkload(workloadName: String, workloadParams: TSWorkload)
  
  @TSClientMethod(name = "start_workload", 
                  argNames = Array("workload_name", "start_time"), 
                  noReturn = true)
  def startWorkload(workloadName: String, startTime: Long) 
  
  @TSClientMethod(name = "provide_step", 
                  argNames = Array("workload_name", "step_id", "num_requests"))
  def provideStep(workloadName: String, stepId: Long, numRequests: Long) : TSProvideStepResult
  
  @TSClientMethod(name = "get_threadpools")
  def getThreadPools : TSThreadPoolList
  
  @TSClientMethod(name = "create_threadpool", 
                  argNames = Array("tp_name", "num_requests", "quantum", "disp_name"),
                  noReturn = true)
  def createThreadPool(threadpoolName: String, numThreads: Long, quantum: Long, dispName: String)
  
  @TSClientMethod(name = "destroy_threadpool", 
                  argNames = Array("tp_name"),
                  noReturn = true)
  def destroyThreadPool(threadpoolName: String)
}

class TSLoadServer(port: Int, agentService: AgentService) extends TSServer[TSLoadClient](port) {

  val clients = new mutable.HashMap[String, TSClient[TSLoadClient]]()

	@TSServerMethod(name = "hello", 
	                argNames = Array("info"))
	def hello(client: TSClient[TSLoadClient], info: TSHostInfo) : TSHelloResponse = {
	  val agentId = agentService.addLoadAgent(info.hostName, client)
    client.id = agentId

	  clients.synchronized {
      clients += agentId -> client
    }
	  new TSHelloResponse(agentId)
	}
	
	@TSServerMethod(name = "workload_status", 
				    argNames = Array("workload_name", "status", "progress", "message"), 
				    noReturn = true) 
	def workloadStatus(client: TSClient[TSLoadClient], workload: String, status: Long, 
	    progress: Long, message: String) {
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

  def fetchWorkloadTypes(clientId: String): Option[TSWorkloadTypeList] = {
    clientForId(clientId).map(_.getInterface.getWorkloadTypes)
  }
  
  def fetchThreadPools(clientId: String): Option[TSThreadPoolList] = {
    clientForId(clientId).map(_.getInterface.getThreadPools)
  }

  private def clientForId(clientId: String): Option[TSClient[TSLoadClient]] = clients.get(clientId)

  override def processClientDisconnect(client: TSClient[TSLoadClient]) {
    clients.synchronized {
      clients -= client.id
    }

    agentService.removeLoadAgent(client.id)
  }
}