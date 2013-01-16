package com.vperflab.agent

import com.foursquare.rogue.Rogue._
import net.liftweb.common._
import org.springframework.stereotype.Component
import com.vperflab.tsserver.{TSClient, TSLoadServer, TSLoadClient, TSClientError}
import com.vperflab.model.Agent
import scala.collection.mutable.{Map => MutableMap}
import org.bson.types.ObjectId

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
      case Empty =>
        /*No such agent, create a new one*/
        logger.info("Registering new agent " + hostName)
        Agent.createRecord.hostName(hostName).isActive(true).save.id.get

      case Failure(message, exception, chain) =>
        throw new TSClientError(message)

    }
    agentId.toString
  }

  def listAgents : List[Agent] = Agent.findAll
}

case class ActiveAgent(hostName: String, loadClient: TSClient[TSLoadClient])
