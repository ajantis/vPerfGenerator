package com.vperflab.agent

import net.liftweb.mapper.By
import net.liftweb.common._

import org.springframework.stereotype.Component

import com.vperflab.tsserver.{TSLoadServer, TSClientError}
import com.vperflab.model.Agent

@Component
class AgentService {
    var tsLoadServer = new TSLoadServer(9090)	
	var tsLoadSrvThread = new Thread(tsLoadServer)
    
	tsLoadSrvThread.start()
}

object AgentService {
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
          agent.id
        }
        case Failure(message, exception, chain) => {
          throw new TSClientError(message)
        } 
      }
    }
    
    def registerLoadAgent(hostName: String) : Long = {
      return registerAgent(hostName: String);
    }
}