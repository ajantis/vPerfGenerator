package com.vperflab.agent

import com.foursquare.rogue.Rogue._
import net.liftweb.common._
import org.springframework.stereotype.Component
import com.vperflab.tsserver._
import com.vperflab.model.{WorkloadParamInfo, WLTypeInfo, Agent}
import org.bson.types.ObjectId
import net.liftweb.common.Full
import scala.Some
import com.vperflab.tsserver.TSClientError
import scala.reflect.Manifest

@Component
class AgentService extends Loggable{
  val tsLoadServer = new TSLoadServer(port = 9090, agentService = this)
  val tsLoadSrvThread = new Thread(tsLoadServer)

  {
    logger.info("Starting TS Load server ...")
    tsLoadSrvThread.start()
  }

  def removeLoadAgent(agentId: String){
    Agent.find("_id", new ObjectId(agentId)) match {
      case Full(agent) if agent.isActive.is => {
        logger.info("Agent " + agent.hostName.asString + " is set as inactive.")
        agent.isActive(false).save
      }
      case Full(agent) if(!agent.isActive.is) =>
        logger.info("Agent " + agent.hostName.asString + " is already inactive. Skipping...")
      case _ => {
        logger.error("Agent with id " + agentId + " is not found.")
      }
    }
  }

  // Registers agent if necessary and returns it's id
  def addLoadAgent(hostName: String, client: TSClient[TSLoadClient]): String = {
    val existingAgent: Box[Agent] = Agent.where(_.hostName eqs hostName).fetch().headOption

    val agentId = existingAgent match {
      case Full(agent: Agent) => {
        logger.info("Existing agent " + hostName + " is active again...")
        agent.isActive(true).save.id.value
      }
      case Empty => {
        /*No such agent, create a new one*/
        logger.info("Registering new agent " + hostName)
        val newAgent = Agent.createRecord.hostName(hostName).isActive(true).save

        newAgent.id.get
      }
      case Failure(message, exception, chain) =>
        throw new TSClientError(message)

    }
    agentId.toString
  }

  def listAgents: List[Agent] = Agent.findAll

  def prefetchAgentInfo(agent: Agent) = {
    tsLoadServer.fetchWorkloadTypes(agent.id.get.toString) match {
      case Some(info) => {
	    def nullableToMap(name: String, param: Any): Map[String, String] = param match {
	      case x: Any => Map(name -> x.toString)
	      case _ => Map()
	    }
        
        logger info "Module info is fetched for agent " + agent.hostName.is + ". Updating..."
        val wltypes = info.wltypes
        val wltypesInfo = for {
          wltype <- wltypes
          params: List[WorkloadParamInfo] = (wltype._2.params.map {
            case (name: String, paramData: TSWorkloadParamInfo) => {
              val paramsInfo: Map[String, String] =
                paramData match {
                  case i: TSWLParamBooleanInfo => nullableToMap("default", i.default)
                  case i: TSWLParamIntegerInfo => nullableToMap("min", i.min) ++ nullableToMap("max", i.max) ++ nullableToMap("default", i.default)
                  case i: TSWLParamFloatInfo => nullableToMap("min", i.min) ++ nullableToMap("max", i.max) ++ nullableToMap("default", i.default)
                  case i: TSWLParamSizeInfo => nullableToMap("min", i.min) ++ nullableToMap("max", i.max) ++ nullableToMap("default", i.default)
                  case i: TSWLParamStringInfo => Map("length" -> i.len.toString) ++ nullableToMap("default", i.default)
                  case i: TSWLParamStringSetInfo => Map(("values" -> i.strset.mkString(",")))
                  case _ => Map()
                }
              WorkloadParamInfo.createRecord.
              	name(name).
                description(paramData.description).
                additionalData(paramsInfo)
            }
          }).toList
        } yield WLTypeInfo.createRecord.name(wltype._1).module(wltype._2.module).path(wltype._2.path).params(params)

        agent.wltypes(wltypesInfo.toList).save
      }
      case _ => logger.error("Modules info is not fetched for agent " + agent.hostName.is)
    }

  }
}

case class ActiveAgent(hostName: String, loadClient: TSClient[TSLoadClient])
