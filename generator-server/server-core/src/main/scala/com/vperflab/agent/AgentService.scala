package com.vperflab.agent

import com.foursquare.rogue.Rogue._
import net.liftweb.common._
import org.springframework.stereotype.Component
import com.vperflab.tsserver.{TSClient, TSLoadServer, TSLoadClient, TSClientError}
import com.vperflab.model.Agent
import scala.collection.mutable.{Map => MutableMap}

@Component
class AgentService {
  var tsLoadServer = new TSLoadServer(9090)
  var tsLoadSrvThread = new Thread(tsLoadServer)

  tsLoadSrvThread.start()
}

class ActiveAgent(hostName: String) {
  var loadClient: TSClient[TSLoadClient] = _

  def getLoadAgent =
    loadClient.getInterface

  def setLoadClient(client: TSClient[TSLoadClient]) =
    loadClient = client
}

case class AgentStatus(hostName: String, isActive: Boolean)

object AgentService {
  var activeAgents = MutableMap[String, ActiveAgent]()

  /**
   * Registers agent and returns it's id
   */
  def registerAgent(hostName: String) : String = {

    val existingAgent: Box[Agent] = Agent.where(_.hostName eqs hostName).fetch().headOption

    existingAgent match {
      case Full(agent: Agent) => agent.id.asString
      case Empty =>
        /*No such agent, create a new one*/
        Agent.createRecord.hostName(hostName).save.id.asString

      case Failure(message, exception, chain) =>
        throw new TSClientError(message)

    }
  }

  def registerLoadAgent(hostName: String, client: TSClient[TSLoadClient]) : String = {
    val agentId = registerAgent(hostName: String)

    activeAgents.synchronized {
      if(!(activeAgents contains hostName)) {
        var agent = new ActiveAgent(hostName)
        agent.setLoadClient(client)

        activeAgents += hostName -> agent
      }
      else {
        /*Already registered?*/
      }
    }
    agentId
  }

  def listAgents : List[Agent] = {
    println(Agent.findAll)
    println(activeAgents)

    Agent.findAll.map {
      // this is just for example.
      // TODO Make status update async
      case agent: Agent => agent.isActive(activeAgents contains agent.hostName.asString).save
    }
  }
}