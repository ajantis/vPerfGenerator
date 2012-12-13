package com.vperflab.agent

import net.liftweb.mapper.By
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
	def registerAgent(hostName: String) : Long = {
      val agentList: Box[Agent] = Agent.find(By(Agent.hostName, hostName))
      
      return agentList match {
        case Full(agent: Agent) => agent.id
        case Empty => {
          /*No such agent, create a new one*/
          val agent = Agent.create
          val agentId = agent.id
          
          agent.hostName(hostName)
          
          val saved: Boolean = agent.save
          
          agentId
        }
        case Failure(message, exception, chain) => {
          throw new TSClientError(message)
        } 
      }
    }
    
    def registerLoadAgent(hostName: String, client: TSClient[TSLoadClient]) : Long = {
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
      
      return agentId
    }
    
    def listAgents : List[AgentStatus] = {
      println(Agent.findAll())
      println(activeAgents)
      
      return Agent.findAll.map( agent => 
        AgentStatus(agent.hostName, activeAgents contains agent.hostName))
    }
}