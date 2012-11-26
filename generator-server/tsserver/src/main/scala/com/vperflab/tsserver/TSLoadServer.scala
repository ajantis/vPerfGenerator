package com.vperflab.tsserver

/* @hello() */
class TSHostInfo extends TSObject {
  var hostName: String = _
}

class TSHostResponse(agentId: Int) extends TSObject 

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

/* createWorkload() */
class TSWorkload extends TSObject {
  var module: String = _
  
  @TSObjContainer(elementClass = classOf[AnyRef])
  var params: Map[String, AnyRef] = _
}

trait TSLoadClient extends TSClientInterface{
  @TSClientMethod(name = "get_modules_info")
  def getModulesInfo() : TSModulesInfo 
}

class TSLoadServer(portNumber: Int) extends TSServer[TSLoadClient](portNumber) {
	/* Commands invoked by tsload */
	@TSServerMethod(name = "hello", argNames = Array("info"))
	def hello(client: TSClient[TSLoadClient], info: TSHostInfo) : TSHostResponse = {
	  System.out.println("Say hello from %s!".format(info.hostName))
	  
	  /*TODO: get agentId for it*/
	  
	  client.getInterface.getModulesInfo()
	  
	  return new TSHostResponse(0)
	}
}